#include "GsPP/ChargeurGsE.hpp"

#include "GsPP/FormatGsE.hpp"
#include "GsPP/VerificateurGsE.hpp"

#include <algorithm>
#include <bit>
#include <climits>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace GsPP
{
    namespace
    {
        constexpr std::uint64_t TailleImageMaximale = 1024ULL * 1024ULL * 1024ULL;

        std::uint16_t Lire16(const std::vector<std::uint8_t>& contenu, std::size_t position)
        {
            return static_cast<std::uint16_t>(contenu[position]
                | (static_cast<std::uint16_t>(contenu[position + 1]) << 8));
        }

        std::uint32_t Lire32(const std::vector<std::uint8_t>& contenu, std::size_t position)
        {
            std::uint32_t valeur = 0;
            for (int index = 0; index < 4; ++index)
                valeur |= static_cast<std::uint32_t>(contenu[position + index]) << (index * 8);
            return valeur;
        }

        std::uint64_t Lire64(const std::vector<std::uint8_t>& contenu, std::size_t position)
        {
            std::uint64_t valeur = 0;
            for (int index = 0; index < 8; ++index)
                valeur |= static_cast<std::uint64_t>(contenu[position + index]) << (index * 8);
            return valeur;
        }

        void Ecrire32(
            std::vector<std::uint8_t>& memoire,
            std::size_t position,
            std::int32_t valeur)
        {
            const auto nonSigne = static_cast<std::uint32_t>(valeur);
            for (int index = 0; index < 4; ++index)
                memoire[position + index] = static_cast<std::uint8_t>(nonSigne >> (index * 8));
        }

        void Ecrire64(
            std::vector<std::uint8_t>& memoire,
            std::size_t position,
            std::uint64_t valeur)
        {
            for (int index = 0; index < 8; ++index)
                memoire[position + index] = static_cast<std::uint8_t>(valeur >> (index * 8));
        }

        std::uint64_t AjouterSigne(std::uint64_t valeur, std::int64_t ajout)
        {
            if (ajout >= 0)
            {
                const auto positif = static_cast<std::uint64_t>(ajout);
                if (valeur > std::numeric_limits<std::uint64_t>::max() - positif)
                    throw std::runtime_error("dépassement de l’adresse d’une relocalisation GsE");
                return valeur + positif;
            }
            const auto amplitude = static_cast<std::uint64_t>(-(ajout + 1)) + 1;
            if (valeur < amplitude)
                throw std::runtime_error("dépassement inférieur de l’adresse d’une relocalisation GsE");
            return valeur - amplitude;
        }

        std::int32_t CalculerRelatif32(std::uint64_t cible, std::uint64_t prochaineInstruction)
        {
            if (cible >= prochaineInstruction)
            {
                const auto distance = cible - prochaineInstruction;
                if (distance > static_cast<std::uint64_t>(INT32_MAX))
                    throw std::runtime_error("relocalisation d’import REL32 hors portée");
                return static_cast<std::int32_t>(distance);
            }
            const auto distance = prochaineInstruction - cible;
            constexpr std::uint64_t amplitudeMin = static_cast<std::uint64_t>(INT32_MAX) + 1;
            if (distance > amplitudeMin)
                throw std::runtime_error("relocalisation d’import REL32 hors portée");
            if (distance == amplitudeMin) return INT32_MIN;
            return -static_cast<std::int32_t>(distance);
        }

        std::string DecrireErreurs(const RapportVerificationGsE& rapport)
        {
            std::ostringstream texte;
            texte << "image GsE invalide";
            for (const auto& erreur : rapport.Erreurs) texte << " ; " << erreur;
            return texte.str();
        }

        struct SectionLue
        {
            std::uint32_t Type = 0;
            std::uint64_t Fichier = 0;
            std::uint64_t Taille = 0;
            std::uint32_t TailleEntree = 0;
            std::uint32_t Nombre = 0;
        };
    }

    std::optional<std::uint64_t> ImageChargeeGsE::ChercherExport(std::string_view nom) const
    {
        const auto exportTrouve = std::find_if(
            Exports.begin(), Exports.end(),
            [&](const ExportChargeGsE& symbole) { return symbole.Nom == nom; });
        if (exportTrouve == Exports.end()) return std::nullopt;
        return exportTrouve->Adresse;
    }

    ImageChargeeGsE ChargeurGsE::Charger(
        const std::vector<std::uint8_t>& contenu,
        std::uint64_t baseChargement,
        const ResolveurImportGsE& resolveur) const
    {
        const auto rapport = VerificateurGsE().Verifier(contenu);
        if (!rapport.Valide) throw std::runtime_error(DecrireErreurs(rapport));

        const auto nombreSegments = Lire32(contenu, 24);
        const auto nombreSections = Lire32(contenu, 28);
        const auto rvaPointEntree = Lire64(contenu, 32);
        const auto tailleImage = Lire64(contenu, 48);
        const auto tableSegments = Lire64(contenu, 56);
        const auto tableSections = Lire64(contenu, 64);
        const auto positionMetadonnees = Lire64(contenu, 72);
        const auto tailleMetadonnees = Lire64(contenu, 80);

        if (tailleImage == 0 || tailleImage > TailleImageMaximale
            || tailleImage > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
            throw std::runtime_error("taille d’image GsE non chargeable");
        if (baseChargement > std::numeric_limits<std::uint64_t>::max() - tailleImage)
            throw std::runtime_error("plage d’adresses de chargement GsE invalide");

        ImageChargeeGsE image;
        image.Memoire.resize(static_cast<std::size_t>(tailleImage), 0);
        image.BaseChargement = baseChargement;
        image.RvaPointEntree = rvaPointEntree;
        image.AdressePointEntree = baseChargement + rvaPointEntree;
        image.Metadonnees.assign(
            contenu.begin() + static_cast<std::ptrdiff_t>(positionMetadonnees),
            contenu.begin() + static_cast<std::ptrdiff_t>(positionMetadonnees + tailleMetadonnees));

        for (std::uint32_t index = 0; index < nombreSegments; ++index)
        {
            const auto position = static_cast<std::size_t>(tableSegments + index * 64ULL);
            const auto drapeaux = Lire32(contenu, position + 4);
            const auto fichier = Lire64(contenu, position + 8);
            const auto tailleFichier = Lire64(contenu, position + 16);
            const auto rva = Lire64(contenu, position + 24);
            const auto tailleMemoire = Lire64(contenu, position + 32);
            if (rva > tailleImage || tailleMemoire > tailleImage - rva)
                throw std::runtime_error("segment hors de l’image GsE");
            if (tailleFichier > 0)
                std::memcpy(
                    image.Memoire.data() + static_cast<std::size_t>(rva),
                    contenu.data() + static_cast<std::size_t>(fichier),
                    static_cast<std::size_t>(tailleFichier));
            image.Segments.push_back({rva, tailleMemoire, drapeaux});
        }

        std::vector<SectionLue> sections;
        for (std::uint32_t index = 0; index < nombreSections; ++index)
        {
            const auto position = static_cast<std::size_t>(tableSections + index * 64ULL);
            sections.push_back({
                Lire32(contenu, position + 16),
                Lire64(contenu, position + 24),
                Lire64(contenu, position + 32),
                Lire32(contenu, position + 56),
                Lire32(contenu, position + 60)
            });
        }

        const auto sectionChaines = std::find_if(
            sections.begin(), sections.end(),
            [](const SectionLue& section) { return section.Type == TypeSectionChainesGsE; });
        auto lireNomSymbole = [&](std::size_t position) -> std::string
        {
            if (sectionChaines == sections.end())
                throw std::runtime_error("table de chaînes GsE absente");
            const auto positionChaine = Lire32(contenu, position);
            const auto tailleNom = Lire16(contenu, position + 4);
            const auto debut = static_cast<std::size_t>(sectionChaines->Fichier + positionChaine);
            return std::string(
                contenu.begin() + static_cast<std::ptrdiff_t>(debut),
                contenu.begin() + static_cast<std::ptrdiff_t>(debut + tailleNom));
        };

        for (const auto& section : sections)
        {
            if (section.Type != TypeSectionImportsGsE) continue;
            for (std::uint32_t index = 0; index < section.Nombre; ++index)
            {
                const auto position = static_cast<std::size_t>(
                    section.Fichier + index * TailleEntreeImportGsE);
                ImportChargeGsE import;
                import.Nom = lireNomSymbole(position);
                import.Type = Lire16(contenu, position + 6);
                import.Abi = Lire16(contenu, position + 8);
                import.Obligatoire = (Lire32(contenu, position + 12) & 1) != 0;
                if (resolveur)
                {
                    if (const auto adresse = resolveur(import.Nom))
                    {
                        import.Resolu = true;
                        import.Adresse = *adresse;
                    }
                }
                if (import.Obligatoire && !import.Resolu)
                    throw std::runtime_error("import GsE obligatoire non résolu : " + import.Nom);
                image.Imports.push_back(std::move(import));
            }
        }

        for (const auto& section : sections)
        {
            if (section.Type != TypeSectionExportsGsE) continue;
            for (std::uint32_t index = 0; index < section.Nombre; ++index)
            {
                const auto position = static_cast<std::size_t>(
                    section.Fichier + index * TailleEntreeExportGsE);
                const auto rva = Lire64(contenu, position + 8);
                image.Exports.push_back({
                    lireNomSymbole(position),
                    baseChargement + rva,
                    Lire32(contenu, position + 16),
                    Lire16(contenu, position + 20),
                    Lire16(contenu, position + 22)
                });
            }
        }

        for (const auto& section : sections)
        {
            if (section.Type != TypeSectionRelocalisationsGsE) continue;
            for (std::uint32_t index = 0; index < section.Nombre; ++index)
            {
                const auto position = static_cast<std::size_t>(
                    section.Fichier + index * TailleEntreeRelocalisationGsE);
                const auto rva = Lire64(contenu, position);
                const auto indexImport = Lire32(contenu, position + 8);
                const auto type = Lire16(contenu, position + 12);
                const auto ajoutBrut = Lire64(contenu, position + 16);
                if (type == TypeRelocalisationBase64GsE)
                {
                    Ecrire64(
                        image.Memoire,
                        static_cast<std::size_t>(rva),
                        baseChargement + ajoutBrut);
                    continue;
                }
                const auto ajout = std::bit_cast<std::int64_t>(ajoutBrut);
                const auto& import = image.Imports.at(indexImport);
                if (!import.Resolu) continue;
                const auto cible = AjouterSigne(import.Adresse, ajout);
                if (type == TypeRelocalisationImportRelatif32GsE)
                {
                    const auto prochaine = baseChargement + rva + 4;
                    Ecrire32(image.Memoire, static_cast<std::size_t>(rva),
                             CalculerRelatif32(cible, prochaine));
                }
                else Ecrire64(image.Memoire, static_cast<std::size_t>(rva), cible);
            }
        }

        return image;
    }

    ImageChargeeGsE ChargeurGsE::Charger(
        const std::filesystem::path& chemin,
        std::uint64_t baseChargement,
        const ResolveurImportGsE& resolveur) const
    {
        std::ifstream flux(chemin, std::ios::binary);
        if (!flux) throw std::runtime_error("impossible d’ouvrir l’image GsE : " + chemin.string());
        const std::vector<std::uint8_t> contenu(
            (std::istreambuf_iterator<char>(flux)), std::istreambuf_iterator<char>());
        return Charger(contenu, baseChargement, resolveur);
    }
}
