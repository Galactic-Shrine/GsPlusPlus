#include "GsPP/ChargeurGsE.hpp"

#include "GsPP/ContexteDemarrage.hpp"
#include "GsPP/VerificateurGsE.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace
{
    struct PageMemoireTest
    {
        void* SuivantePhysique;
        void* SuivanteTas;
        std::int32_t NombrePages;
        std::int32_t TailleDemandee;
        std::uint32_t PremierMot;
        std::int32_t Etat;
    };

    static_assert(sizeof(PageMemoireTest) == 32);

    std::uint64_t Lire64(const std::vector<std::uint8_t>& contenu, std::size_t position)
    {
        std::uint64_t valeur = 0;
        for (int index = 0; index < 8; ++index)
            valeur |= static_cast<std::uint64_t>(contenu[position + index]) << (index * 8);
        return valeur;
    }

    std::uint64_t LireAdresse(const std::string& texte)
    {
        std::size_t fin = 0;
        const auto valeur = std::stoull(texte, &fin, 0);
        if (fin != texte.size()) throw std::runtime_error("adresse invalide : " + texte);
        return valeur;
    }

    std::vector<std::uint8_t> LireFichier(const std::string& chemin)
    {
        std::ifstream flux(chemin, std::ios::binary);
        if (!flux) throw std::runtime_error("impossible d’ouvrir l’image GsE : " + chemin);
        return {(std::istreambuf_iterator<char>(flux)), std::istreambuf_iterator<char>()};
    }

    class ZoneExecutable final
    {
    public:
        explicit ZoneExecutable(std::size_t taille) : Taille(taille)
        {
#if defined(_WIN32)
            Adresse = VirtualAlloc(nullptr, Taille, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#elif defined(__unix__) || defined(__APPLE__)
            Adresse = mmap(nullptr, Taille, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (Adresse == MAP_FAILED) Adresse = nullptr;
#else
            (void)Taille;
#endif
            if (Adresse == nullptr)
                throw std::runtime_error("allocation de mémoire exécutable indisponible");
        }

        ZoneExecutable(const ZoneExecutable&) = delete;
        ZoneExecutable& operator=(const ZoneExecutable&) = delete;

        ~ZoneExecutable()
        {
#if defined(_WIN32)
            if (Adresse != nullptr) VirtualFree(Adresse, 0, MEM_RELEASE);
#elif defined(__unix__) || defined(__APPLE__)
            if (Adresse != nullptr) munmap(Adresse, Taille);
#endif
        }

        [[nodiscard]] void* Obtenir() const { return Adresse; }

        void Copier(const std::vector<std::uint8_t>& contenu)
        {
            if (contenu.size() > Taille) throw std::runtime_error("image trop grande pour la zone allouée");
            std::memcpy(Adresse, contenu.data(), contenu.size());
        }

        void AppliquerProtections(const std::vector<GsPP::SegmentChargeGsE>& segments)
        {
#if defined(_WIN32)
            DWORD ancien = 0;
            if (!VirtualProtect(Adresse, Taille, PAGE_NOACCESS, &ancien))
                throw std::runtime_error("échec de la protection globale de l’image");
            for (const auto& segment : segments)
            {
                DWORD protection = PAGE_NOACCESS;
                const bool lecture = (segment.Drapeaux & 1) != 0;
                const bool ecriture = (segment.Drapeaux & 2) != 0;
                const bool execution = (segment.Drapeaux & 4) != 0;
                if (execution) protection = ecriture ? PAGE_EXECUTE_READWRITE
                    : lecture ? PAGE_EXECUTE_READ : PAGE_EXECUTE;
                else if (ecriture) protection = PAGE_READWRITE;
                else if (lecture) protection = PAGE_READONLY;
                auto* debut = static_cast<std::uint8_t*>(Adresse) + segment.Rva;
                if (!VirtualProtect(debut, static_cast<SIZE_T>(segment.Taille), protection, &ancien))
                    throw std::runtime_error("échec de la protection d’un segment GsE");
            }
            FlushInstructionCache(GetCurrentProcess(), Adresse, Taille);
#elif defined(__unix__) || defined(__APPLE__)
            const auto taillePageSysteme = sysconf(_SC_PAGESIZE);
            if (taillePageSysteme <= 0)
                throw std::runtime_error("taille de page mémoire indisponible");
            const auto taillePage = static_cast<std::uint64_t>(taillePageSysteme);
            const auto alignerBas = [&](std::uint64_t valeur) { return valeur & ~(taillePage - 1); };
            const auto alignerHaut = [&](std::uint64_t valeur)
            { return (valeur + taillePage - 1) & ~(taillePage - 1); };
            if (mprotect(Adresse, Taille, PROT_NONE) != 0)
                throw std::runtime_error("échec de la protection globale de l’image");
            for (const auto& segment : segments)
            {
                int protection = PROT_NONE;
                if (segment.Drapeaux & 1) protection |= PROT_READ;
                if (segment.Drapeaux & 2) protection |= PROT_WRITE;
                if (segment.Drapeaux & 4) protection |= PROT_EXEC;
                const auto debut = alignerBas(segment.Rva);
                const auto fin = alignerHaut(segment.Rva + segment.Taille);
                auto* adresse = static_cast<std::uint8_t*>(Adresse) + debut;
                if (mprotect(adresse, static_cast<std::size_t>(fin - debut), protection) != 0)
                    throw std::runtime_error("échec de la protection d’un segment GsE");
            }
            auto* debut = static_cast<char*>(Adresse);
            __builtin___clear_cache(debut, debut + Taille);
#else
            (void)segments;
            throw std::runtime_error("exécution GsE non prise en charge sur cette plateforme");
#endif
        }

    private:
        void* Adresse = nullptr;
        std::size_t Taille = 0;
    };

    void AfficherAide()
    {
        std::cout
            << "Chargeur GsE 0.27.0-alpha.4\n\n"
            << "Utilisation : gsechargeur <image.GsE> [options]\n"
            << "Alias anglais : gseload\n\n"
            << "Options :\n"
            << "  --base <adresse>                 base logique de chargement\n"
            << "  --resoudre <nom=adresse>         résoudre un import\n"
            << "  --resolve <name=address>         alias anglais\n"
            << "  --executer, --execute            exécuter le point d’entrée sans argument\n"
            << "  --executer-noyau                 passer un ContexteDemarrage au noyau\n"
            << "  --execute-kernel                 alias anglais\n"
            << "  --aide, --help                   afficher cette aide\n";
    }
}

int main(int argc, char** argv)
{
    if (argc <= 1) { AfficherAide(); return 1; }

    try
    {
        std::string chemin;
        std::uint64_t base = 0x10000000;
        bool baseIndiquee = false;
        bool executer = false;
        bool executerNoyau = false;
        std::unordered_map<std::string, std::uint64_t> resolutions;
        for (int index = 1; index < argc; ++index)
        {
            const std::string argument = argv[index];
            if (argument == "--version")
            { std::cout << "Chargeur GsE 0.27.0-alpha.4\n"; return 0; }
            if (argument == "--aide" || argument == "--help" || argument == "-h")
            { AfficherAide(); return 0; }
            if (argument == "--executer" || argument == "--execute")
            { executer = true; continue; }
            if (argument == "--executer-noyau" || argument == "--execute-kernel")
            { executer = true; executerNoyau = true; continue; }
            if (argument == "--base")
            {
                if (++index >= argc) throw std::runtime_error("adresse attendue après --base");
                base = LireAdresse(argv[index]);
                baseIndiquee = true;
                continue;
            }
            if (argument == "--resoudre" || argument == "--resolve")
            {
                if (++index >= argc) throw std::runtime_error("résolution attendue après " + argument);
                const std::string resolution = argv[index];
                const auto separateur = resolution.find('=');
                if (separateur == std::string::npos || separateur == 0)
                    throw std::runtime_error("résolution attendue sous la forme nom=adresse");
                resolutions[resolution.substr(0, separateur)] =
                    LireAdresse(resolution.substr(separateur + 1));
                continue;
            }
            if (!argument.empty() && argument[0] == '-')
                throw std::runtime_error("option inconnue : " + argument);
            if (!chemin.empty()) throw std::runtime_error("une seule image GsE peut être chargée");
            chemin = argument;
        }
        if (chemin.empty()) throw std::runtime_error("aucune image GsE indiquée");
        if (executer && baseIndiquee)
            throw std::runtime_error("--base ne peut pas être combiné avec --executer");

        const auto contenu = LireFichier(chemin);
        if (executer)
        {
            const auto rapport = GsPP::VerificateurGsE().Verifier(contenu);
            if (!rapport.Valide)
                throw std::runtime_error("l’image doit être valide avant son exécution");
            const auto tailleImage = Lire64(contenu, 48);
            if (tailleImage == 0
                || tailleImage > 1024ULL * 1024ULL * 1024ULL
                || tailleImage > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
                throw std::runtime_error("taille d’image non exécutable");
            ZoneExecutable zone(static_cast<std::size_t>(tailleImage));
            base = reinterpret_cast<std::uintptr_t>(zone.Obtenir());
            const auto resolveur = [&](std::string_view nom) -> std::optional<std::uint64_t>
            {
                const auto trouve = resolutions.find(std::string(nom));
                if (trouve == resolutions.end()) return std::nullopt;
                return trouve->second;
            };
            const auto image = GsPP::ChargeurGsE().Charger(contenu, base, resolveur);
            zone.Copier(image.Memoire);
            zone.AppliquerProtections(image.Segments);
            std::int32_t resultat = 0;
            if (executerNoyau)
            {
                alignas(4096) std::array<std::uint32_t, 4096> reservePages{};
                alignas(4096) std::array<std::uint32_t, 8192> pagesPhysiques{};
                alignas(4096) std::array<std::uint64_t, 512> tablePages{};
                std::array<GsPP::PlageMemoirePhysique, 2> plagesMemoire{{
                    {pagesPhysiques.data(), 4, 0},
                    {pagesPhysiques.data() + 4096, 4, 0}}};
                std::vector<std::uint32_t> tamponImage(320 * 48);
                GsPP::ContexteDemarrage contexte;
                contexte.BaseNoyau = zone.Obtenir();
                contexte.TailleNoyau = static_cast<std::uint32_t>(image.Memoire.size());
                contexte.ReservePages = reservePages.data();
                contexte.NombrePagesReservees = 4;
                contexte.PlagesMemoireLibres = plagesMemoire.data();
                contexte.NombrePlagesMemoireLibres =
                    static_cast<std::uint32_t>(plagesMemoire.size());
                contexte.TablePages = tablePages.data();
                contexte.TaillePageMemoire = 4096;
                contexte.NombrePagesTablesPagination = 1;
                contexte.NombrePagesCartographiees = 1024;
                contexte.NombrePagesRecuperees = 2;
                contexte.LimitePagination = reinterpret_cast<void*>(4096ULL * 1024ULL);
                contexte.CapacitePlagesMemoire = static_cast<std::uint32_t>(plagesMemoire.size());
                contexte.TamponImage = tamponImage.data();
                contexte.LargeurEcran = 320;
                contexte.HauteurEcran = 48;
                contexte.PixelsParLigne = 320;
                contexte.Drapeaux |= GsPP::ContexteAvecReservePages
                    | GsPP::ContexteAvecTamponImage
                    | GsPP::ContexteAvecPlagesMemoire
                    | GsPP::ContexteAvecPagination
                    | GsPP::ContexteAvecMemoireRecuperee;
#if defined(__GNUC__) && defined(__x86_64__) && !defined(_WIN32)
                using PointEntreeNoyau = std::int32_t (__attribute__((ms_abi)) *)(GsPP::ContexteDemarrage*);
#else
                using PointEntreeNoyau = std::int32_t (*)(GsPP::ContexteDemarrage*);
#endif
                const auto fonction = reinterpret_cast<PointEntreeNoyau>(image.AdressePointEntree);
                resultat = fonction(&contexte);
                const auto* premierePage = reinterpret_cast<const PageMemoireTest*>(
                    pagesPhysiques.data());
                if (premierePage->PremierMot != 1397248594U
                    || pagesPhysiques[1024 + 8] != 1397248595U
                    || pagesPhysiques[1024 + 9] != 1397248597U
                    || pagesPhysiques[1024 + 23] != 1397248596U)
                    throw std::runtime_error(
                        "le noyau n’a pas validé l’allocateur physique et le tas");
                if (tamponImage[0] != 2105376U
                    || tamponImage[12 * 320 + 12] != 65380U
                    || tamponImage[12 * 320 + 44] != 16766720U
                    || tamponImage[39 * 320 + 76] != 4259648U)
                    throw std::runtime_error("la console framebuffer du noyau n’a pas été initialisée");

                const auto adresseAllouerPages = image.ChercherExport(
                    "Sanctuaire::Noyau::AllocatePages");
                const auto adresseLibererPages = image.ChercherExport(
                    "Sanctuaire::Noyau::FreePages");
                const auto adresseAllouerTas = image.ChercherExport(
                    "Sanctuaire::Noyau::AllocateKernelMemory");
                const auto adresseLibererTas = image.ChercherExport(
                    "Sanctuaire::Noyau::FreeKernelMemory");
                const auto adresseInitialiserTas = image.ChercherExport(
                    "Sanctuaire::Noyau::InitializeKernelHeap");
                const auto adresseTotalPages = image.ChercherExport(
                    "Sanctuaire::Noyau::TotalPageCount");
                const auto adressePagesLibres = image.ChercherExport(
                    "Sanctuaire::Noyau::FreePageCount");
                const auto adressePagesAllouees = image.ChercherExport(
                    "Sanctuaire::Noyau::AllocatedPageCount");
                const auto adresseAllocationsTas = image.ChercherExport(
                    "Sanctuaire::Noyau::HeapAllocationCount");
                const auto adresseOctetsTas = image.ChercherExport(
                    "Sanctuaire::Noyau::KernelHeapBytesInUse");
                const auto adresseEtatMemoire = image.ChercherExport(
                    "Sanctuaire::Noyau::MemoryManagerStatus");
                const auto adressePagination = image.ChercherExport(
                    "Sanctuaire::Noyau::PagingAvailable");
                const auto adressePagesCartographiees = image.ChercherExport(
                    "Sanctuaire::Noyau::MappedPageCount");
                const auto adressePageCartographiee = image.ChercherExport(
                    "Sanctuaire::Noyau::IsPageMapped");
                const auto adresseRacinePagination = image.ChercherExport(
                    "Sanctuaire::Noyau::PageTableRoot");
                if (!adresseAllouerPages || !adresseLibererPages
                    || !adresseAllouerTas || !adresseLibererTas || !adresseInitialiserTas
                    || !adresseTotalPages || !adressePagesLibres
                    || !adressePagesAllouees || !adresseAllocationsTas
                    || !adresseOctetsTas || !adresseEtatMemoire
                    || !adressePagination || !adressePagesCartographiees
                    || !adressePageCartographiee || !adresseRacinePagination)
                    throw std::runtime_error("API de gestion mémoire Gs++ incomplète");
#if defined(__GNUC__) && defined(__x86_64__) && !defined(_WIN32)
                using AllouerPagesNoyau = void* (__attribute__((ms_abi)) *)(std::int32_t);
                using LibererPagesNoyau = std::int32_t (__attribute__((ms_abi)) *)(void*, std::int32_t);
                using AllouerTasNoyau = std::int32_t* (__attribute__((ms_abi)) *)(std::int32_t);
                using LibererTasNoyau = std::int32_t (__attribute__((ms_abi)) *)(std::int32_t*);
                using InitialiserTasNoyau = void (__attribute__((ms_abi)) *)();
                using LireEntierNoyau = std::int32_t (__attribute__((ms_abi)) *)();
                using TesterPageNoyau = std::int32_t (__attribute__((ms_abi)) *)(std::int32_t);
                using LirePointeurNoyau = void* (__attribute__((ms_abi)) *)();
#else
                using AllouerPagesNoyau = void* (*)(std::int32_t);
                using LibererPagesNoyau = std::int32_t (*)(void*, std::int32_t);
                using AllouerTasNoyau = std::int32_t* (*)(std::int32_t);
                using LibererTasNoyau = std::int32_t (*)(std::int32_t*);
                using InitialiserTasNoyau = void (*)();
                using LireEntierNoyau = std::int32_t (*)();
                using TesterPageNoyau = std::int32_t (*)(std::int32_t);
                using LirePointeurNoyau = void* (*)();
#endif
                const auto lireEntier = [](const std::optional<std::uint64_t>& adresse)
                {
                    return reinterpret_cast<LireEntierNoyau>(*adresse)();
                };
                if (resultat != 5 || lireEntier(adresseEtatMemoire) != 7
                    || lireEntier(adresseTotalPages) != 12
                    || lireEntier(adressePagesLibres) != 11
                    || lireEntier(adressePagesAllouees) != 1
                    || lireEntier(adressePagination) != 1
                    || lireEntier(adressePagesCartographiees) != 1024
                    || (contexte.Drapeaux & GsPP::ContexteAvecGestionMemoireNoyau) == 0
                    || reinterpret_cast<LirePointeurNoyau>(*adresseRacinePagination)()
                        != tablePages.data())
                    throw std::runtime_error("état initial du gestionnaire mémoire incorrect");

                const auto allouerPages = reinterpret_cast<AllouerPagesNoyau>(
                    *adresseAllouerPages);
                const auto libererPages = reinterpret_cast<LibererPagesNoyau>(
                    *adresseLibererPages);
                void* blocPages = allouerPages(2);
                if (blocPages == nullptr || libererPages(blocPages, 2) != 1)
                    throw std::runtime_error("allocation ou libération physique multi-page en échec");
                blocPages = allouerPages(2);
                if (blocPages == nullptr || libererPages(blocPages, 2) != 1)
                    throw std::runtime_error("réutilisation d’une plage physique en échec");

                const auto allouerTas = reinterpret_cast<AllouerTasNoyau>(*adresseAllouerTas);
                const auto libererTas = reinterpret_cast<LibererTasNoyau>(*adresseLibererTas);
                auto* blocTas = allouerTas(5000);
                if (blocTas == nullptr)
                    throw std::runtime_error("allocation multi-page du tas en échec");
                blocTas[0] = 0x10203040;
                blocTas[1] = 0x30405060;
                blocTas[1249] = 0x50607080;
                reinterpret_cast<InitialiserTasNoyau>(*adresseInitialiserTas)();
                if (lireEntier(adresseAllocationsTas) != 1
                    || lireEntier(adresseOctetsTas) != 5000)
                    throw std::runtime_error("réinitialisation ou comptage du tas incorrect");
                auto* enteteTas = reinterpret_cast<PageMemoireTest*>(blocTas) - 1;
                const auto etatTas = enteteTas->Etat;
                enteteTas->Etat = 0;
                if (libererTas(blocTas) != 0
                    || lireEntier(adresseAllocationsTas) != 1
                    || lireEntier(adresseOctetsTas) != 5000)
                    throw std::runtime_error("un échec de libération a perdu le bloc du tas");
                enteteTas->Etat = etatTas;
                if (libererTas(blocTas) != 1
                    || libererTas(blocTas) != 0)
                    throw std::runtime_error("protection de libération du tas incorrecte");
                if (lireEntier(adressePagesLibres) != 11
                    || lireEntier(adressePagesAllouees) != 1
                    || lireEntier(adresseAllocationsTas) != 0
                    || lireEntier(adresseOctetsTas) != 0)
                    throw std::runtime_error("les statistiques mémoire ne sont pas revenues à l’équilibre");
                const auto pageEstCartographiee = reinterpret_cast<TesterPageNoyau>(
                    *adressePageCartographiee);
                if (pageEstCartographiee(0) != 0
                    || pageEstCartographiee(1) != 1
                    || pageEstCartographiee(1023) != 1
                    || pageEstCartographiee(1024) != 0)
                    throw std::runtime_error("bornes de pagination Gs++ incorrectes");

                const auto adresseGestionnaire = image.ChercherExport(
                    "Sanctuaire::Noyau::GererInterruption");
                const auto adresseTicks = image.ChercherExport(
                    "Sanctuaire::Noyau::ReadTickCount");
                const auto adresseCodeClavier = image.ChercherExport(
                    "Sanctuaire::Noyau::ReadLastKeyCode");
                if (!adresseGestionnaire || !adresseTicks || !adresseCodeClavier)
                    throw std::runtime_error("API d’interruptions Gs++ absente du noyau");
#if defined(__GNUC__) && defined(__x86_64__) && !defined(_WIN32)
                using GestionnaireIrq = void (__attribute__((ms_abi)) *)(std::int32_t, std::int32_t);
                using LireEtatIrq = std::int32_t (__attribute__((ms_abi)) *)();
#else
                using GestionnaireIrq = void (*)(std::int32_t, std::int32_t);
                using LireEtatIrq = std::int32_t (*)();
#endif
                contexte.NombreTicksHorloge = 70;
                contexte.DernierCodeClavier = 30;
                reinterpret_cast<GestionnaireIrq>(*adresseGestionnaire)(32, 70);
                reinterpret_cast<GestionnaireIrq>(*adresseGestionnaire)(33, 30);
                if (tamponImage[12 * 320 + 104] != 53503U
                    || tamponImage[27 * 320 + 196] != 16756736U
                    || reinterpret_cast<LireEtatIrq>(*adresseTicks)() != 70
                    || reinterpret_cast<LireEtatIrq>(*adresseCodeClavier)() != 30)
                    throw std::runtime_error("le répartiteur d’interruptions Gs++ n’a pas réagi");
            }
            else
            {
                using PointEntree = std::int32_t (*)();
                const auto fonction = reinterpret_cast<PointEntree>(image.AdressePointEntree);
                resultat = fonction();
            }
            std::cout << "Image GsE chargée et exécutée.\n"
                      << "Code de retour : " << resultat << '\n';
            return 0;
        }

        const auto resolveur = [&](std::string_view nom) -> std::optional<std::uint64_t>
        {
            const auto trouve = resolutions.find(std::string(nom));
            if (trouve == resolutions.end()) return std::nullopt;
            return trouve->second;
        };
        const auto image = GsPP::ChargeurGsE().Charger(contenu, base, resolveur);
        std::size_t resolus = 0;
        for (const auto& import : image.Imports) if (import.Resolu) ++resolus;
        std::cout << "Image GsE chargée.\n"
                  << "Base : 0x" << std::hex << image.BaseChargement << '\n'
                  << "Point d’entrée : 0x" << image.AdressePointEntree << std::dec << '\n'
                  << "Mémoire : " << image.Memoire.size() << " octets\n"
                  << "Segments : " << image.Segments.size() << '\n'
                  << "Imports résolus : " << resolus << '/' << image.Imports.size() << '\n'
                  << "Exports : " << image.Exports.size() << '\n';
        return 0;
    }
    catch (const std::exception& erreur)
    {
        std::cerr << "GSL0001 : " << erreur.what() << '\n';
        return 1;
    }
}
