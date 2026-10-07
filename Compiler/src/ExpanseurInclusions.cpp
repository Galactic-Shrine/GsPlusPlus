#include "GsPP/ExpanseurInclusions.hpp"

#include "GsPP/ChargeurGsE.hpp"
#include "GsPP/FichiersSource.hpp"
#include "GsPP/VerificateurGsE.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#endif

#if defined(__GNUC__) && defined(__x86_64__) && !defined(_WIN32)
#define GS_ABI_EXPANSION __attribute__((ms_abi))
#else
#define GS_ABI_EXPANSION
#endif

namespace GsPP
{
    namespace
    {
        std::uint8_t* GS_ABI_EXPANSION Allouer(std::uint64_t taille) noexcept
        {
            if (taille > std::numeric_limits<std::size_t>::max()) return nullptr;
            return static_cast<std::uint8_t*>(std::malloc(static_cast<std::size_t>(taille)));
        }

        void GS_ABI_EXPANSION Liberer(std::uint8_t* adresse) noexcept { std::free(adresse); }

        std::uint64_t Lire64(const std::vector<std::uint8_t>& contenu, std::size_t position)
        {
            std::uint64_t valeur = 0;
            for (unsigned i = 0; i < 8; ++i) valeur |= std::uint64_t(contenu.at(position + i)) << (8 * i);
            return valeur;
        }
    }

    struct ExpanseurInclusionsCharge::Implementation
    {
        void* Adresse = nullptr;
        std::size_t Taille = 0;
        ExpanseurInclusionsAvecRepriseHote Fonction = nullptr;
        std::uint64_t Base = 0;
        std::vector<ExportChargeGsE> Exports;
        std::vector<SegmentChargeGsE> Segments;

        std::uint64_t ChercherFonction(std::string_view francais, std::string_view anglais) const
        {
            auto chercher = [&](std::string_view nom)
            { return std::find_if(Exports.begin(), Exports.end(), [&](const auto& e) { return e.Nom == nom; }); };
            const auto fr = chercher(francais), en = chercher(anglais);
            if (fr == Exports.end() || en == Exports.end() || !fr->Adresse || fr->Adresse != en->Adresse)
                throw std::runtime_error("exports FR/EN absents ou différents dans l'image GsE : " + std::string(francais));
            for (const auto* exporte : {&*fr, &*en})
                if (exporte->Type != 1 || exporte->Section != 0 || !exporte->Taille || exporte->Adresse < Base
                    || !std::any_of(Segments.begin(), Segments.end(), [&](const auto& segment)
                    {
                        const auto rva = exporte->Adresse - Base;
                        return (segment.Drapeaux & 4U) && rva >= segment.Rva && rva - segment.Rva < segment.Taille
                            && exporte->Taille <= segment.Taille - (rva - segment.Rva);
                    }))
                    throw std::runtime_error("export non exécutable dans l'image GsE : " + exporte->Nom);
            return fr->Adresse;
        }

        ~Implementation()
        {
#if defined(_WIN32)
            if (Adresse) VirtualFree(Adresse, 0, MEM_RELEASE);
#elif defined(__linux__)
            if (Adresse) munmap(Adresse, Taille);
#endif
        }

        void Proteger(std::size_t debut, std::size_t taille, std::uint32_t drapeaux)
        {
#if defined(_WIN32)
            DWORD protection = PAGE_NOACCESS, ancien = 0;
            if (drapeaux & 4U) protection = PAGE_EXECUTE_READ;
            else if (drapeaux & 2U) protection = PAGE_READWRITE;
            else if (drapeaux & 1U) protection = PAGE_READONLY;
            if (!VirtualProtect(static_cast<std::uint8_t*>(Adresse) + debut, taille, protection, &ancien))
                throw std::runtime_error("protection mémoire de l'expanseur GsE échouée");
#elif defined(__linux__)
            int protection = PROT_NONE;
            if (drapeaux & 1U) protection |= PROT_READ;
            if (drapeaux & 2U) protection |= PROT_WRITE;
            if (drapeaux & 4U) protection |= PROT_EXEC;
            if (mprotect(static_cast<std::uint8_t*>(Adresse) + debut, taille, protection) != 0)
                throw std::runtime_error("protection mémoire de l'expanseur GsE échouée");
#else
            (void)debut; (void)taille; (void)drapeaux;
            throw std::runtime_error("chargement exécutable de l'expanseur non pris en charge");
#endif
        }
    };

    ExpanseurInclusionsCharge::ExpanseurInclusionsCharge(const std::filesystem::path& chemin)
        : implementation_(std::make_unique<Implementation>())
    {
#if !(defined(_M_X64) || defined(__x86_64__)) || !(defined(_WIN32) || defined(__linux__))
        (void)chemin;
        throw std::runtime_error("l'expanseur GsE exige un hôte Windows/Linux x86-64");
#else
        const auto texte = LireFichierSource(chemin, 64 * 1024 * 1024);
        const std::vector<std::uint8_t> contenu(texte.begin(), texte.end());
        const auto rapport = VerificateurGsE().Verifier(contenu);
        if (!rapport.Valide)
            throw std::runtime_error("image d'expansion GsE invalide : "
                + (rapport.Erreurs.empty() ? chemin.string() : rapport.Erreurs.front()));
        std::size_t page = 0;
#if defined(_WIN32)
        SYSTEM_INFO informations{}; GetSystemInfo(&informations); page = informations.dwPageSize;
#else
        const auto valeurPage = sysconf(_SC_PAGESIZE);
        if (valeurPage > 0) page = static_cast<std::size_t>(valeurPage);
#endif
        if (!page || (page & (page - 1)) || page < 32)
            throw std::runtime_error("taille de page de l'expanseur incompatible");
        const auto tailleImage = Lire64(contenu, 48);
        // Borne avant allocation et avant tout calcul de trampoline relatif32.
        if (!tailleImage || tailleImage > 1024ULL * 1024 * 1024
            || page > 1024 * 1024 || tailleImage > std::numeric_limits<std::size_t>::max() - 2 * page)
            throw std::length_error("image d'expansion GsE trop grande");
        const auto trampolines = (static_cast<std::size_t>(tailleImage) + page - 1) & ~(page - 1);
        auto& zone = *implementation_;
        zone.Taille = trampolines + page;
#if defined(_WIN32)
        zone.Adresse = VirtualAlloc(nullptr, zone.Taille, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
        zone.Adresse = mmap(nullptr, zone.Taille, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (zone.Adresse == MAP_FAILED) zone.Adresse = nullptr;
#endif
        if (!zone.Adresse) throw std::runtime_error("allocation mémoire de l'expanseur GsE échouée");
        const auto base = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(zone.Adresse));
        const auto image = ChargeurGsE().Charger(contenu, base,
            [&](std::string_view nom) -> std::optional<std::uint64_t>
        {
            if (nom == "GalacticShrine::GsPP::Hote::AllouerMemoire") return base + trampolines;
            if (nom == "GalacticShrine::GsPP::Hote::LibererMemoire") return base + trampolines + 16;
            throw std::runtime_error("import interdit dans l'expanseur GsE : " + std::string(nom));
        });
        for (const auto& import : image.Imports)
            if (!import.Resolu || import.Type != 1 || import.Abi != 1)
                throw std::runtime_error("import incompatible dans l'expanseur GsE");
        zone.Base = base; zone.Exports = image.Exports; zone.Segments = image.Segments;
        const auto francais = zone.ChercherFonction("GalacticShrine::GsPP::Autohebergement::DevelopperInclusionsAvecReprise",
            "GalacticShrine::GsPP::Autohebergement::ResumeDeclarationIncludes");
        auto segments = image.Segments;
        std::sort(segments.begin(), segments.end(), [](const auto& a, const auto& b) { return a.Rva < b.Rva; });
        std::uint64_t finPrecedente = 0;
        for (const auto& segment : segments)
        {
            const auto fin = (segment.Rva + segment.Taille + page - 1) & ~std::uint64_t(page - 1);
            if (!segment.Taille || segment.Rva % page || segment.Rva < finPrecedente || fin > trampolines
                || (segment.Drapeaux & ~7U) || (segment.Drapeaux & 6U) == 6U)
                throw std::runtime_error("segments de l'expanseur incompatibles avec les protections W^X");
            finPrecedente = fin;
        }
        std::memcpy(zone.Adresse, image.Memoire.data(), image.Memoire.size());
        auto trampoline = [&](std::size_t offset, std::uintptr_t destination)
        {
            auto* debut = static_cast<std::uint8_t*>(zone.Adresse) + offset;
            debut[0] = 0x48; debut[1] = 0xB8;
            std::memcpy(debut + 2, &destination, 8);
            debut[10] = 0xFF; debut[11] = 0xE0;
        };
        trampoline(trampolines, reinterpret_cast<std::uintptr_t>(&Allouer));
        trampoline(trampolines + 16, reinterpret_cast<std::uintptr_t>(&Liberer));
        zone.Proteger(0, zone.Taille, 0);
        for (const auto& segment : segments)
            zone.Proteger(static_cast<std::size_t>(segment.Rva), static_cast<std::size_t>(segment.Taille), segment.Drapeaux);
        zone.Proteger(trampolines, page, 5);
#if defined(_WIN32)
        if (!FlushInstructionCache(GetCurrentProcess(), zone.Adresse, zone.Taille))
            throw std::runtime_error("synchronisation du code de l'expanseur échouée");
#else
        auto* debut = static_cast<char*>(zone.Adresse);
        __builtin___clear_cache(debut, debut + zone.Taille);
#endif
        zone.Fonction = reinterpret_cast<ExpanseurInclusionsAvecRepriseHote>(francais);
#endif
    }

    ExpanseurInclusionsCharge::~ExpanseurInclusionsCharge() = default;
    ExpanseurInclusionsAvecRepriseHote ExpanseurInclusionsCharge::Developper() const noexcept
    {
        return implementation_->Fonction;
    }

    AnalyseurDeclarationsAvecOriginesHote ExpanseurInclusionsCharge::AnalyseurDeclarations() const
    {
        return reinterpret_cast<AnalyseurDeclarationsAvecOriginesHote>(implementation_->ChercherFonction(
            "GalacticShrine::GsPP::Autohebergement::AnalyserDeclarationsAvecOrigines",
            "GalacticShrine::GsPP::Autohebergement::AnalyzeOriginAwareDeclarations"));
    }
}
