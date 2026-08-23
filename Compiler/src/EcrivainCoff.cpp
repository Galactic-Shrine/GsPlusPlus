#include "GsPP/EcrivainCoff.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace GsPP
{
    namespace
    {
        void Ajouter16(std::vector<std::uint8_t>& sortie, std::uint16_t valeur)
        {
            sortie.push_back(static_cast<std::uint8_t>(valeur));
            sortie.push_back(static_cast<std::uint8_t>(valeur >> 8));
        }

        void Ajouter32(std::vector<std::uint8_t>& sortie, std::uint32_t valeur)
        {
            for (int index = 0; index < 4; ++index)
                sortie.push_back(static_cast<std::uint8_t>(valeur >> (index * 8)));
        }

        void AjouterNomCourt(std::vector<std::uint8_t>& sortie, const std::string& nom)
        {
            std::array<std::uint8_t, 8> valeur{};
            std::memcpy(valeur.data(), nom.data(), std::min(nom.size(), valeur.size()));
            sortie.insert(sortie.end(), valeur.begin(), valeur.end());
        }

        struct SectionCoff
        {
            SectionMachine Type;
            std::string Nom;
            const std::vector<std::uint8_t>* Donnees;
            std::uint32_t TailleMemoire;
            std::uint32_t Caracteristiques;
            std::uint32_t DecalageFichier = 0;
            std::uint32_t DecalageRelocalisations = 0;
            std::vector<const CodeMachine::Relocalisation*> Relocalisations;
        };
    }

    std::vector<std::uint8_t> EcrivainCoff::Construire(const CodeMachine& machine) const
    {
        constexpr std::uint32_t tailleEntete = 20;
        constexpr std::uint32_t tailleEnteteSection = 40;
        constexpr std::uint32_t tailleRelocalisation = 10;
        constexpr std::uint32_t tailleSymbole = 18;

        std::vector<SectionCoff> sections;
        sections.push_back({SectionMachine::Texte, ".text", &machine.Texte,
                            static_cast<std::uint32_t>(machine.Texte.size()), 0x60500020, 0, 0, {}});
        if (!machine.Donnees.empty())
            sections.push_back({SectionMachine::Donnees, ".data", &machine.Donnees,
                                static_cast<std::uint32_t>(machine.Donnees.size()), 0xC0500040, 0, 0, {}});
        if (machine.TailleZero > 0)
            sections.push_back({SectionMachine::Zero, ".bss", nullptr,
                                machine.TailleZero, 0xC0500080, 0, 0, {}});

        std::unordered_map<SectionMachine, std::int16_t> indicesSections;
        for (std::size_t index = 0; index < sections.size(); ++index)
            indicesSections.emplace(sections[index].Type, static_cast<std::int16_t>(index + 1));

        for (const auto& relocalisation : machine.Relocalisations)
        {
            const auto section = std::find_if(sections.begin(), sections.end(), [&](const SectionCoff& valeur)
            { return valeur.Type == relocalisation.Section; });
            if (section == sections.end()) throw std::runtime_error("section COFF de relocalisation absente");
            section->Relocalisations.push_back(&relocalisation);
        }

        std::uint32_t curseur = tailleEntete
            + static_cast<std::uint32_t>(sections.size()) * tailleEnteteSection;
        for (auto& section : sections)
        {
            if (section.Donnees && !section.Donnees->empty())
            {
                section.DecalageFichier = curseur;
                curseur += static_cast<std::uint32_t>(section.Donnees->size());
            }
        }
        for (auto& section : sections)
        {
            if (section.Relocalisations.size() > 0xFFFF)
                throw std::runtime_error("trop de relocalisations COFF");
            if (!section.Relocalisations.empty())
            {
                section.DecalageRelocalisations = curseur;
                curseur += static_cast<std::uint32_t>(section.Relocalisations.size()) * tailleRelocalisation;
            }
        }
        const auto debutSymboles = curseur;

        std::vector<SymboleMachine> symboles = machine.Symboles;
        std::unordered_map<std::string, std::uint32_t> indicesSymboles;
        for (std::size_t index = 0; index < symboles.size(); ++index)
        {
            if (!indicesSymboles.emplace(symboles[index].Nom, static_cast<std::uint32_t>(index)).second)
                throw std::runtime_error("symbole machine dupliqué : " + symboles[index].Nom);
        }
        for (const auto& relocalisation : machine.Relocalisations)
        {
            if (!indicesSymboles.contains(relocalisation.Symbole))
            {
                indicesSymboles.emplace(relocalisation.Symbole, static_cast<std::uint32_t>(symboles.size()));
                symboles.push_back({relocalisation.Symbole, 0, 0, false,
                                    SectionMachine::Indefinie, false,
                                    GenreSymboleMachine::Fonction, {}, {}});
            }
        }

        std::vector<std::uint8_t> chaines(4, 0);
        struct NomEncode { bool Court; std::string Nom; std::uint32_t Decalage; };
        std::vector<NomEncode> noms;
        for (const auto& symbole : symboles)
        {
            if (symbole.Nom.size() <= 8) noms.push_back({true, symbole.Nom, 0});
            else
            {
                const auto decalage = static_cast<std::uint32_t>(chaines.size());
                chaines.insert(chaines.end(), symbole.Nom.begin(), symbole.Nom.end());
                chaines.push_back(0);
                noms.push_back({false, symbole.Nom, decalage});
            }
        }
        const auto tailleChaines = static_cast<std::uint32_t>(chaines.size());
        for (int index = 0; index < 4; ++index)
            chaines[index] = static_cast<std::uint8_t>(tailleChaines >> (index * 8));

        std::vector<std::uint8_t> sortie;
        sortie.reserve(debutSymboles + symboles.size() * tailleSymbole + chaines.size());
        Ajouter16(sortie, 0x8664);
        Ajouter16(sortie, static_cast<std::uint16_t>(sections.size()));
        Ajouter32(sortie, 0);
        Ajouter32(sortie, debutSymboles);
        Ajouter32(sortie, static_cast<std::uint32_t>(symboles.size()));
        Ajouter16(sortie, 0);
        Ajouter16(sortie, 0);

        for (const auto& section : sections)
        {
            AjouterNomCourt(sortie, section.Nom);
            Ajouter32(sortie, 0);
            Ajouter32(sortie, 0);
            Ajouter32(sortie, section.TailleMemoire);
            Ajouter32(sortie, section.DecalageFichier);
            Ajouter32(sortie, section.DecalageRelocalisations);
            Ajouter32(sortie, 0);
            Ajouter16(sortie, static_cast<std::uint16_t>(section.Relocalisations.size()));
            Ajouter16(sortie, 0);
            Ajouter32(sortie, section.Caracteristiques);
        }

        for (const auto& section : sections)
            if (section.Donnees) sortie.insert(sortie.end(), section.Donnees->begin(), section.Donnees->end());

        for (const auto& section : sections)
        {
            for (const auto* relocalisation : section.Relocalisations)
            {
                Ajouter32(sortie, relocalisation->Decalage);
                Ajouter32(sortie, indicesSymboles.at(relocalisation->Symbole));
                Ajouter16(sortie, relocalisation->Type == TypeRelocalisationMachine::Relatif32
                    ? 0x0004 : 0x0001);
            }
        }

        for (std::size_t index = 0; index < symboles.size(); ++index)
        {
            const auto& symbole = symboles[index];
            const auto& nom = noms[index];
            if (nom.Court) AjouterNomCourt(sortie, nom.Nom);
            else { Ajouter32(sortie, 0); Ajouter32(sortie, nom.Decalage); }
            Ajouter32(sortie, symbole.Decalage);
            const auto numeroSection = symbole.EstDefini ? indicesSections.at(symbole.Section) : 0;
            Ajouter16(sortie, static_cast<std::uint16_t>(numeroSection));
            Ajouter16(sortie, symbole.Section == SectionMachine::Texte
                || symbole.Section == SectionMachine::Indefinie ? 0x20 : 0);
            sortie.push_back(symbole.EstPublic || !symbole.EstDefini ? 2 : 3);
            sortie.push_back(0);
        }
        sortie.insert(sortie.end(), chaines.begin(), chaines.end());
        return sortie;
    }

    void EcrivainCoff::Ecrire(const CodeMachine& machine, const std::filesystem::path& chemin) const
    {
        const auto contenu = Construire(machine);
        std::ofstream flux(chemin, std::ios::binary | std::ios::trunc);
        if (!flux) throw std::runtime_error("impossible d’ouvrir le fichier de sortie");
        flux.write(reinterpret_cast<const char*>(contenu.data()), static_cast<std::streamsize>(contenu.size()));
        if (!flux) throw std::runtime_error("échec de l’écriture du fichier de sortie");
    }
}
