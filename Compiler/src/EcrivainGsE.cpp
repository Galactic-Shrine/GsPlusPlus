#include "GsPP/EcrivainGsE.hpp"

#include "GsPP/FormatGsE.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace GsPP
{
    namespace
    {
        constexpr std::uint64_t TailleEntete = 0x70;
        constexpr std::uint64_t TailleSegment = 64;
        constexpr std::uint64_t TailleSection = 64;

        void Ajouter16(std::vector<std::uint8_t>& sortie, std::uint16_t valeur)
        { sortie.push_back(static_cast<std::uint8_t>(valeur)); sortie.push_back(static_cast<std::uint8_t>(valeur >> 8)); }
        void Ajouter32(std::vector<std::uint8_t>& sortie, std::uint32_t valeur)
        { for (int i = 0; i < 4; ++i) sortie.push_back(static_cast<std::uint8_t>(valeur >> (i * 8))); }
        void Ajouter64(std::vector<std::uint8_t>& sortie, std::uint64_t valeur)
        { for (int i = 0; i < 8; ++i) sortie.push_back(static_cast<std::uint8_t>(valeur >> (i * 8))); }
        void AjouterNomFixe(std::vector<std::uint8_t>& sortie, const std::string& nom, std::size_t taille)
        {
            if (nom.size() >= taille) throw std::runtime_error("nom trop long dans une table GsE : " + nom);
            const auto debut = sortie.size();
            sortie.resize(debut + taille, 0);
            std::memcpy(sortie.data() + debut, nom.data(), nom.size());
        }
        std::uint64_t Aligner(std::uint64_t valeur, std::uint64_t alignement)
        { return (valeur + alignement - 1) & ~(alignement - 1); }
        void Ecrire32(std::vector<std::uint8_t>& sortie, std::size_t position, std::int32_t valeur)
        {
            if (position + 4 > sortie.size()) throw std::runtime_error("relocalisation GsE hors section");
            const auto v = static_cast<std::uint32_t>(valeur);
            for (int i = 0; i < 4; ++i) sortie[position + i] = static_cast<std::uint8_t>(v >> (i * 8));
        }
        void Ecrire64(std::vector<std::uint8_t>& sortie, std::size_t position, std::uint64_t valeur)
        {
            if (position + 8 > sortie.size()) throw std::runtime_error("relocalisation GsE hors section");
            for (int i = 0; i < 8; ++i) sortie[position + i] = static_cast<std::uint8_t>(valeur >> (i * 8));
        }
        std::string Echapper(std::string valeur)
        {
            std::string resultat;
            for (const char c : valeur)
            {
                if (c == '\\' || c == '"') resultat.push_back('\\');
                resultat.push_back(c);
            }
            return resultat;
        }

        struct Segment
        {
            std::uint32_t Type = 1;
            std::uint32_t Drapeaux = 0;
            std::uint64_t Fichier = 0;
            std::uint64_t TailleFichier = 0;
            std::uint64_t Virtuelle = 0;
            std::uint64_t TailleMemoire = 0;
            std::uint64_t Alignement = 0x1000;
        };
        struct Section
        {
            std::string Nom;
            std::uint32_t Type;
            std::uint32_t Drapeaux;
            std::uint64_t Fichier;
            std::uint64_t TailleFichier;
            std::uint64_t Virtuelle;
            std::uint64_t TailleMemoire;
            std::uint32_t TailleEntree;
            std::uint32_t NombreEntrees;
        };
    }

    std::vector<std::uint8_t> EcrivainGsE::Construire(
        const CodeMachine& machine,
        const std::string& pointEntree,
        const MetadonneesGsE& metadonnees) const
    {
        std::unordered_map<std::string, const SymboleMachine*> symboles;
        for (const auto& symbole : machine.Symboles) symboles.emplace(symbole.Nom, &symbole);
        const auto entree = symboles.find(pointEntree);
        if (entree == symboles.end() || !entree->second->EstDefini
            || entree->second->Section != SectionMachine::Texte)
            throw std::runtime_error("point d’entrée GsE exécutable introuvable : " + pointEntree);

        const std::uint64_t texteVa = 0;
        const std::uint64_t donneesVa = Aligner(machine.Texte.size(), 0x1000);
        const std::uint64_t zeroVa = Aligner(donneesVa + machine.Donnees.size(), 0x1000);
        auto adresseSymbole = [&](const SymboleMachine& symbole) -> std::uint64_t
        {
            if (symbole.Section == SectionMachine::Texte) return texteVa + symbole.Decalage;
            if (symbole.Section == SectionMachine::Donnees) return donneesVa + symbole.Decalage;
            if (symbole.Section == SectionMachine::Zero) return zeroVa + symbole.Decalage;
            throw std::runtime_error("symbole sans adresse GsE : " + symbole.Nom);
        };

        std::vector<const SymboleMachine*> imports;
        std::unordered_map<std::string, std::uint32_t> indexImports;
        for (const auto& symbole : machine.Symboles)
        {
            if (!symbole.EstDefini)
            {
                indexImports.emplace(symbole.Nom, static_cast<std::uint32_t>(imports.size()));
                imports.push_back(&symbole);
            }
        }

        auto texte = machine.Texte;
        auto donnees = machine.Donnees;
        struct RelocalisationChargeur { std::uint64_t Rva; std::uint32_t Import; std::uint16_t Type; std::uint16_t Section; std::uint64_t Ajout; };
        std::vector<RelocalisationChargeur> relocalisationsChargeur;
        for (const auto& relocalisation : machine.Relocalisations)
        {
            auto* source = relocalisation.Section == SectionMachine::Texte ? &texte : &donnees;
            const auto sourceVa = relocalisation.Section == SectionMachine::Texte ? texteVa : donneesVa;
            const auto cible = symboles.find(relocalisation.Symbole);
            if (cible != symboles.end() && cible->second->EstDefini)
            {
                const auto cibleVa = adresseSymbole(*cible->second);
                if (relocalisation.Type == TypeRelocalisationMachine::Relatif32)
                {
                    const auto relatif = static_cast<std::int64_t>(cibleVa)
                        - static_cast<std::int64_t>(sourceVa + relocalisation.Decalage + 4);
                    if (relatif < INT32_MIN || relatif > INT32_MAX) throw std::runtime_error("relocalisation GsE REL32 hors portée");
                    Ecrire32(*source, relocalisation.Decalage, static_cast<std::int32_t>(relatif));
                }
                else
                {
                    Ecrire64(*source, relocalisation.Decalage, 0);
                    relocalisationsChargeur.push_back({
                        sourceVa + relocalisation.Decalage,
                        IndiceImportRelocalisationBaseGsE,
                        TypeRelocalisationBase64GsE,
                        static_cast<std::uint16_t>(
                            relocalisation.Section == SectionMachine::Texte ? 0 : 1),
                        cibleVa
                    });
                }
            }
            else
            {
                if (!indexImports.contains(relocalisation.Symbole))
                {
                    indexImports.emplace(relocalisation.Symbole, static_cast<std::uint32_t>(imports.size()));
                    imports.push_back(nullptr);
                }
                relocalisationsChargeur.push_back({
                    sourceVa + relocalisation.Decalage,
                    indexImports.at(relocalisation.Symbole),
                    static_cast<std::uint16_t>(
                        relocalisation.Type == TypeRelocalisationMachine::Relatif32
                            ? TypeRelocalisationImportRelatif32GsE
                            : TypeRelocalisationImportAdresse64GsE),
                    static_cast<std::uint16_t>(relocalisation.Section == SectionMachine::Texte ? 0 : 1),
                    0
                });
            }
        }

        std::vector<std::uint8_t> blocChaines;
        std::unordered_map<std::string, std::uint32_t> positionsChaines;
        auto ajouterChaine = [&](const std::string& nom) -> std::uint32_t
        {
            if (nom.empty()) throw std::runtime_error("nom de symbole GsE vide");
            if (nom.size() > TailleNomSymboleGsEMaximale)
                throw std::runtime_error(
                    "nom de symbole GsE supérieur à 1024 octets UTF-8 : " + nom);
            if (nom.find('\0') != std::string::npos)
                throw std::runtime_error("nom de symbole GsE contenant un octet nul");
            if (!Utf8GsEValide(
                reinterpret_cast<const std::uint8_t*>(nom.data()), nom.size()))
                throw std::runtime_error("nom de symbole GsE en UTF-8 invalide");
            if (const auto existante = positionsChaines.find(nom);
                existante != positionsChaines.end())
                return existante->second;
            if (blocChaines.size() > UINT32_MAX
                || nom.size() + 1 > UINT32_MAX - blocChaines.size())
                throw std::overflow_error("table de chaînes GsE trop grande");
            const auto position = static_cast<std::uint32_t>(blocChaines.size());
            positionsChaines.emplace(nom, position);
            blocChaines.insert(blocChaines.end(), nom.begin(), nom.end());
            blocChaines.push_back(0);
            return position;
        };

        std::vector<std::uint8_t> blocImports;
        std::vector<std::string> nomsImports(imports.size());
        for (const auto& [nom, index] : indexImports) nomsImports[index] = nom;
        for (const auto& nom : nomsImports)
        {
            Ajouter32(blocImports, ajouterChaine(nom));
            Ajouter16(blocImports, static_cast<std::uint16_t>(nom.size()));
            Ajouter16(blocImports, 1); // fonction
            Ajouter16(blocImports, VersionAbiGsE);
            Ajouter16(blocImports, 0);
            Ajouter32(blocImports, 1); // obligatoire
            Ajouter64(blocImports, 0);
            Ajouter64(blocImports, 0);
        }

        std::vector<const SymboleMachine*> exports;
        for (const auto& symbole : machine.Symboles)
            if (symbole.EstDefini && symbole.EstPublic) exports.push_back(&symbole);
        std::vector<std::uint8_t> blocExports;
        for (const auto* symbole : exports)
        {
            Ajouter32(blocExports, ajouterChaine(symbole->Nom));
            Ajouter16(blocExports, static_cast<std::uint16_t>(symbole->Nom.size()));
            Ajouter16(blocExports, 0);
            Ajouter64(blocExports, adresseSymbole(*symbole));
            Ajouter32(blocExports, symbole->Taille);
            Ajouter16(blocExports, symbole->Section == SectionMachine::Texte ? 0
                : symbole->Section == SectionMachine::Donnees ? 1 : 2);
            Ajouter16(blocExports, symbole->Section == SectionMachine::Texte ? 1 : 2);
            Ajouter32(blocExports, 1);
            Ajouter32(blocExports, 0);
        }

        std::vector<std::uint8_t> blocRelocalisations;
        for (const auto& relocalisation : relocalisationsChargeur)
        {
            Ajouter64(blocRelocalisations, relocalisation.Rva);
            Ajouter32(blocRelocalisations, relocalisation.Import);
            Ajouter16(blocRelocalisations, relocalisation.Type);
            Ajouter16(blocRelocalisations, relocalisation.Section);
            Ajouter64(blocRelocalisations, relocalisation.Ajout);
        }

        std::ostringstream meta;
        meta << "Format = \"GsEMetadata:1\";\nMétadonnées {\n"
             << "    Nom = \"" << Echapper(metadonnees.Nom) << "\";\n"
             << "    Version = \"" << Echapper(metadonnees.Version) << "\";\n"
             << "    Éditeur = \"" << Echapper(metadonnees.Editeur) << "\";\n"
             << "    Cible = \"" << Echapper(metadonnees.Cible) << "\";\n"
             << "    Abi = \"" << Echapper(metadonnees.Abi) << "\";\n"
             << "    Langage = \"" << Echapper(metadonnees.Langage) << "\";\n"
             << "    PointEntrée = \"" << Echapper(pointEntree) << "\";\n}\n";
        const auto texteMeta = meta.str();
        std::vector<std::uint8_t> blocMeta(texteMeta.begin(), texteMeta.end());

        const std::uint32_t nombreSegments = 1 + (!donnees.empty() ? 1 : 0) + (machine.TailleZero > 0 ? 1 : 0);
        const std::uint32_t nombreSections = 1 + (!donnees.empty() ? 1 : 0) + (machine.TailleZero > 0 ? 1 : 0)
            + (!blocImports.empty() ? 1 : 0) + (!blocExports.empty() ? 1 : 0)
            + (!blocRelocalisations.empty() ? 1 : 0) + (!blocChaines.empty() ? 1 : 0) + 1;
        std::uint64_t curseur = TailleEntete + nombreSegments * TailleSegment + nombreSections * TailleSection;
        auto placer = [&](std::size_t taille) { curseur = Aligner(curseur, 16); const auto debut = curseur; curseur += taille; return debut; };

        std::vector<Segment> segments;
        std::vector<Section> sections;
        const auto fichierTexte = placer(texte.size());
        segments.push_back({1, 5, fichierTexte, texte.size(), texteVa, texte.size(), 0x1000});
        sections.push_back({".texte", 1, 9, fichierTexte, texte.size(), texteVa, texte.size(), 0, 0});
        std::uint64_t fichierDonnees = 0;
        if (!donnees.empty())
        {
            fichierDonnees = placer(donnees.size());
            segments.push_back({1, 3, fichierDonnees, donnees.size(), donneesVa, donnees.size(), 0x1000});
            sections.push_back({".donnees", 2, 3, fichierDonnees, donnees.size(), donneesVa, donnees.size(), 0, 0});
        }
        if (machine.TailleZero > 0)
        {
            segments.push_back({1, 3, 0, 0, zeroVa, machine.TailleZero, 0x1000});
            sections.push_back({".zero", 3, 3, 0, 0, zeroVa, machine.TailleZero, 0, 0});
        }
        std::uint64_t fichierImports = 0, fichierExports = 0, fichierRelog = 0, fichierChaines = 0;
        if (!blocImports.empty())
        {
            fichierImports = placer(blocImports.size());
            sections.push_back({".imports", TypeSectionImportsGsE, 1, fichierImports,
                blocImports.size(), 0, 0, TailleEntreeImportGsE,
                static_cast<std::uint32_t>(nomsImports.size())});
        }
        if (!blocExports.empty())
        {
            fichierExports = placer(blocExports.size());
            sections.push_back({".exports", TypeSectionExportsGsE, 1, fichierExports,
                blocExports.size(), 0, 0, TailleEntreeExportGsE,
                static_cast<std::uint32_t>(exports.size())});
        }
        if (!blocRelocalisations.empty())
        {
            fichierRelog = placer(blocRelocalisations.size());
            sections.push_back({".relog", TypeSectionRelocalisationsGsE, 1, fichierRelog,
                blocRelocalisations.size(), 0, 0, TailleEntreeRelocalisationGsE,
                static_cast<std::uint32_t>(relocalisationsChargeur.size())});
        }
        if (!blocChaines.empty())
        {
            fichierChaines = placer(blocChaines.size());
            sections.push_back({".chaines", TypeSectionChainesGsE, 1, fichierChaines,
                blocChaines.size(), 0, 0, 0, 0});
        }
        const auto fichierMeta = placer(blocMeta.size());
        sections.push_back({".meta", TypeSectionMetadonneesGsE, 1, fichierMeta,
            blocMeta.size(), 0, 0, 0, 0});

        std::vector<std::uint8_t> sortie;
        sortie.insert(sortie.end(), {'G', 'S', 'E', ':', '0', 0, 0, 0});
        Ajouter16(sortie, VersionMajeureGsE); Ajouter16(sortie, VersionMineureGsE);
        Ajouter16(sortie, 0x70); Ajouter16(sortie, 0x8664);
        Ajouter16(sortie, 1); Ajouter16(sortie, 3); // exécutable, PIC + métadonnées
        Ajouter16(sortie, VersionAbiGsE); Ajouter16(sortie, 0);
        Ajouter32(sortie, nombreSegments); Ajouter32(sortie, nombreSections);
        Ajouter64(sortie, adresseSymbole(*entree->second)); Ajouter64(sortie, 0);
        Ajouter64(sortie, Aligner(zeroVa + machine.TailleZero, 0x1000));
        Ajouter64(sortie, TailleEntete);
        Ajouter64(sortie, TailleEntete + nombreSegments * TailleSegment);
        Ajouter64(sortie, fichierMeta); Ajouter64(sortie, blocMeta.size());
        Ajouter64(sortie, 0); Ajouter64(sortie, 0); Ajouter64(sortie, 0);
        if (sortie.size() != TailleEntete) throw std::logic_error("taille interne de l’en-tête GsE incorrecte");

        for (const auto& segment : segments)
        {
            Ajouter32(sortie, segment.Type); Ajouter32(sortie, segment.Drapeaux);
            Ajouter64(sortie, segment.Fichier); Ajouter64(sortie, segment.TailleFichier);
            Ajouter64(sortie, segment.Virtuelle); Ajouter64(sortie, segment.TailleMemoire);
            Ajouter64(sortie, segment.Alignement); Ajouter64(sortie, 0); Ajouter64(sortie, 0);
        }
        for (const auto& section : sections)
        {
            AjouterNomFixe(sortie, section.Nom, 16); Ajouter32(sortie, section.Type); Ajouter32(sortie, section.Drapeaux);
            Ajouter64(sortie, section.Fichier); Ajouter64(sortie, section.TailleFichier);
            Ajouter64(sortie, section.Virtuelle); Ajouter64(sortie, section.TailleMemoire);
            Ajouter32(sortie, section.TailleEntree); Ajouter32(sortie, section.NombreEntrees);
        }

        auto ajouterBloc = [&](std::uint64_t position, const std::vector<std::uint8_t>& bloc)
        {
            if (position > static_cast<std::uint64_t>(SIZE_MAX))
                throw std::overflow_error("position de bloc GsE non représentable");
            const auto positionBloc = static_cast<std::size_t>(position);
            if (sortie.size() > positionBloc)
                throw std::logic_error("chevauchement de blocs dans l’image GsE");
            sortie.resize(positionBloc, 0);
            sortie.insert(sortie.end(), bloc.begin(), bloc.end());
        };
        ajouterBloc(fichierTexte, texte);
        if (!donnees.empty()) ajouterBloc(fichierDonnees, donnees);
        if (!blocImports.empty()) ajouterBloc(fichierImports, blocImports);
        if (!blocExports.empty()) ajouterBloc(fichierExports, blocExports);
        if (!blocRelocalisations.empty()) ajouterBloc(fichierRelog, blocRelocalisations);
        if (!blocChaines.empty()) ajouterBloc(fichierChaines, blocChaines);
        ajouterBloc(fichierMeta, blocMeta);
        return sortie;
    }

    void EcrivainGsE::Ecrire(
        const CodeMachine& machine,
        const std::string& pointEntree,
        const std::filesystem::path& chemin,
        const MetadonneesGsE& metadonnees) const
    {
        const auto contenu = Construire(machine, pointEntree, metadonnees);
        std::ofstream flux(chemin, std::ios::binary | std::ios::trunc);
        if (!flux) throw std::runtime_error("impossible d’ouvrir la sortie GsE");
        flux.write(reinterpret_cast<const char*>(contenu.data()), static_cast<std::streamsize>(contenu.size()));
        if (!flux) throw std::runtime_error("échec de l’écriture GsE");
    }
}
