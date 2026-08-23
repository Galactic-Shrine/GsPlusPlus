#include "GsPP/EditeurLiens.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace GsPP
{
    namespace
    {
        std::uint32_t Aligner16(std::uint32_t valeur)
        {
            if (valeur > UINT32_MAX - 15)
                throw std::overflow_error("section liée supérieure à 4 Gio");
            return (valeur + 15) & ~std::uint32_t{15};
        }

        std::string AfficherPosition(const PositionSource& position)
        {
            if (position.Fichier.empty()) return "<origine inconnue>";
            return position.Fichier + ':' + std::to_string(position.Ligne)
                + ':' + std::to_string(position.Colonne);
        }

        struct ContratAbi
        {
            std::string Signature;
            GenreSymboleMachine Genre = GenreSymboleMachine::Fonction;
            PositionSource Position;
        };

        std::unordered_set<std::string> SymbolesRequis(
            const std::vector<UniteLiaison>& objets,
            const std::string& racine)
        {
            std::unordered_set<std::string> definitions;
            std::unordered_set<std::string> requis;
            for (const auto& objet : objets)
            {
                for (const auto& symbole : objet.Machine.Symboles)
                {
                    if (symbole.EstDefini && symbole.EstPublic)
                        definitions.insert(symbole.Nom);
                    else if (!symbole.EstDefini)
                        requis.insert(symbole.Nom);
                }
            }
            if (!racine.empty()) requis.insert(racine);
            for (const auto& definition : definitions) requis.erase(definition);
            return requis;
        }

        bool DefinitUnSymboleRequis(
            const UniteLiaison& membre,
            const std::unordered_set<std::string>& requis)
        {
            return std::any_of(
                membre.Machine.Symboles.begin(), membre.Machine.Symboles.end(),
                [&](const SymboleMachine& symbole)
                {
                    return symbole.EstDefini && symbole.EstPublic
                        && requis.contains(symbole.Nom);
                });
        }

        std::uint64_t AdresseSection(
            SectionMachine section,
            std::uint64_t texte,
            std::uint64_t donnees,
            std::uint64_t zero)
        {
            if (section == SectionMachine::Texte) return texte;
            if (section == SectionMachine::Donnees) return donnees;
            if (section == SectionMachine::Zero) return zero;
            return 0;
        }
    }

    CodeMachine EditeurLiens::Lier(
        std::vector<UniteLiaison> objets,
        const std::vector<BibliothequeLiaison>& bibliotheques,
        const std::string& symboleRacine) const
    {
        std::vector<std::vector<bool>> extraits;
        extraits.reserve(bibliotheques.size());
        for (const auto& bibliotheque : bibliotheques)
            extraits.emplace_back(bibliotheque.size(), false);

        while (true)
        {
            const auto requis = SymbolesRequis(objets, symboleRacine);
            bool membreExtrait = false;
            for (std::size_t indexBibliotheque = 0;
                 indexBibliotheque < bibliotheques.size() && !membreExtrait;
                 ++indexBibliotheque)
            {
                const auto& bibliotheque = bibliotheques[indexBibliotheque];
                for (std::size_t indexMembre = 0;
                     indexMembre < bibliotheque.size(); ++indexMembre)
                {
                    if (extraits[indexBibliotheque][indexMembre]) continue;
                    if (!DefinitUnSymboleRequis(bibliotheque[indexMembre], requis))
                        continue;
                    objets.push_back(bibliotheque[indexMembre]);
                    extraits[indexBibliotheque][indexMembre] = true;
                    membreExtrait = true;
                    break;
                }
            }
            if (!membreExtrait) break;
        }

        if (objets.empty())
            throw std::runtime_error("aucun objet à lier");

        CodeMachine resultat;
        struct Bases
        {
            std::uint32_t Texte;
            std::uint32_t Donnees;
            std::uint32_t Zero;
        };
        std::vector<Bases> bases;
        bases.reserve(objets.size());
        for (const auto& objet : objets)
        {
            const auto baseTexte = Aligner16(
                static_cast<std::uint32_t>(resultat.Texte.size()));
            resultat.Texte.resize(baseTexte, 0x90);
            if (objet.Machine.Texte.size() > UINT32_MAX - resultat.Texte.size())
                throw std::overflow_error("section texte liée supérieure à 4 Gio");
            resultat.Texte.insert(
                resultat.Texte.end(),
                objet.Machine.Texte.begin(), objet.Machine.Texte.end());

            const auto baseDonnees = Aligner16(
                static_cast<std::uint32_t>(resultat.Donnees.size()));
            resultat.Donnees.resize(baseDonnees, 0);
            if (objet.Machine.Donnees.size() > UINT32_MAX - resultat.Donnees.size())
                throw std::overflow_error("section de données liée supérieure à 4 Gio");
            resultat.Donnees.insert(
                resultat.Donnees.end(),
                objet.Machine.Donnees.begin(), objet.Machine.Donnees.end());

            const auto baseZero = Aligner16(resultat.TailleZero);
            if (objet.Machine.TailleZero > UINT32_MAX - baseZero)
                throw std::overflow_error("section zéro liée supérieure à 4 Gio");
            resultat.TailleZero = baseZero + objet.Machine.TailleZero;
            bases.push_back({baseTexte, baseDonnees, baseZero});
        }

        std::unordered_map<std::string, ContratAbi> contrats;
        auto verifierContrat = [&](const SymboleMachine& symbole)
        {
            const auto [trouve, insere] = contrats.emplace(
                symbole.Nom,
                ContratAbi{symbole.SignatureAbi, symbole.Genre, symbole.Position});
            if (insere) return;
            if (trouve->second.Signature == symbole.SignatureAbi
                && trouve->second.Genre == symbole.Genre)
                return;
            throw std::runtime_error(
                "incompatibilité ABI pour " + symbole.Nom + " entre "
                + AfficherPosition(trouve->second.Position) + " et "
                + AfficherPosition(symbole.Position));
        };

        std::unordered_map<std::string, std::size_t> definitionsPubliques;
        std::vector<std::unordered_map<std::string, std::string>> nomsLies(objets.size());
        for (std::size_t indexObjet = 0; indexObjet < objets.size(); ++indexObjet)
        {
            const auto& objet = objets[indexObjet];
            for (const auto& symbole : objet.Machine.Symboles)
            {
                if (!symbole.EstDefini || symbole.EstPublic)
                    verifierContrat(symbole);

                std::string nomLie = symbole.Nom;
                if (symbole.EstDefini && !symbole.EstPublic)
                    nomLie = "@GsLocal" + std::to_string(indexObjet) + "::" + symbole.Nom;
                nomsLies[indexObjet].emplace(symbole.Nom, nomLie);

                if (!symbole.EstDefini) continue;
                if (symbole.EstPublic)
                {
                    const auto [_, insere] = definitionsPubliques.emplace(
                        symbole.Nom, indexObjet);
                    if (!insere)
                        throw std::runtime_error(
                            "symbole public défini plusieurs fois : " + symbole.Nom);
                }

                auto lie = symbole;
                lie.Nom = std::move(nomLie);
                if (lie.Section == SectionMachine::Texte)
                    lie.Decalage += bases[indexObjet].Texte;
                else if (lie.Section == SectionMachine::Donnees)
                    lie.Decalage += bases[indexObjet].Donnees;
                else if (lie.Section == SectionMachine::Zero)
                    lie.Decalage += bases[indexObjet].Zero;
                resultat.Symboles.push_back(std::move(lie));
            }
        }

        std::unordered_set<std::string> importsAjoutes;
        for (std::size_t indexObjet = 0; indexObjet < objets.size(); ++indexObjet)
        {
            const auto& objet = objets[indexObjet];
            for (const auto& symbole : objet.Machine.Symboles)
            {
                if (symbole.EstDefini
                    || definitionsPubliques.contains(symbole.Nom)
                    || !importsAjoutes.insert(symbole.Nom).second)
                    continue;
                resultat.Symboles.push_back(symbole);
            }

            for (const auto& relocalisation : objet.Machine.Relocalisations)
            {
                const auto trouveNom = nomsLies[indexObjet].find(relocalisation.Symbole);
                if (trouveNom == nomsLies[indexObjet].end())
                    throw std::runtime_error(
                        "cible de relocalisation absente de l’objet " + objet.Nom
                        + " : " + relocalisation.Symbole);
                auto liee = relocalisation;
                liee.Symbole = trouveNom->second;
                if (liee.Section == SectionMachine::Texte)
                    liee.Decalage += bases[indexObjet].Texte;
                else if (liee.Section == SectionMachine::Donnees)
                    liee.Decalage += bases[indexObjet].Donnees;
                else throw std::runtime_error("section source de relocalisation invalide");
                resultat.Relocalisations.push_back(std::move(liee));
            }
        }

        if (!symboleRacine.empty()
            && !definitionsPubliques.contains(symboleRacine))
            throw std::runtime_error(
                "symbole racine non défini par les objets ou bibliothèques : "
                + symboleRacine);
        return resultat;
    }

    void EditeurLiens::EcrireCarte(
        const CodeMachine& machine,
        const std::filesystem::path& chemin) const
    {
        const auto alignerPage = [](std::uint64_t valeur)
        {
            return (valeur + 0xFFF) & ~std::uint64_t{0xFFF};
        };
        const auto baseTexte = std::uint64_t{0};
        const auto baseDonnees = alignerPage(machine.Texte.size());
        const auto baseZero = alignerPage(baseDonnees + machine.Donnees.size());

        std::vector<const SymboleMachine*> symboles;
        for (const auto& symbole : machine.Symboles)
            if (symbole.EstDefini) symboles.push_back(&symbole);
        std::sort(
            symboles.begin(), symboles.end(),
            [&](const SymboleMachine* gauche, const SymboleMachine* droite)
            {
                const auto adresseGauche = AdresseSection(
                    gauche->Section, baseTexte, baseDonnees, baseZero) + gauche->Decalage;
                const auto adresseDroite = AdresseSection(
                    droite->Section, baseTexte, baseDonnees, baseZero) + droite->Decalage;
                if (adresseGauche != adresseDroite) return adresseGauche < adresseDroite;
                return gauche->Nom < droite->Nom;
            });

        std::ofstream flux(chemin, std::ios::binary | std::ios::trunc);
        if (!flux)
            throw std::runtime_error("impossible d’ouvrir la carte de liens : " + chemin.string());
        flux << "Gs++ Link Map 1.0\n"
             << "texte=0x0 taille=0x" << std::hex << machine.Texte.size() << '\n'
             << "donnees=0x" << baseDonnees << " taille=0x" << machine.Donnees.size() << '\n'
             << "zero=0x" << baseZero << " taille=0x" << machine.TailleZero << "\n\n";
        for (const auto* symbole : symboles)
        {
            const auto adresse = AdresseSection(
                symbole->Section, baseTexte, baseDonnees, baseZero) + symbole->Decalage;
            flux << "0x" << std::setw(16) << std::setfill('0') << adresse
                 << ' ' << (symbole->EstPublic ? "public " : "local  ")
                 << (symbole->Genre == GenreSymboleMachine::Fonction ? "fonction " : "objet    ")
                 << symbole->Nom << " taille=0x" << symbole->Taille
                 << " source=" << AfficherPosition(symbole->Position)
                 << " abi=" << symbole->SignatureAbi << '\n';
        }
        if (!flux) throw std::runtime_error("échec d’écriture de la carte de liens");
    }
}
