#include "GsPP/ObjetGsO.hpp"

#include "GsPP/FormatGsE.hpp"
#include "GsPP/FormatGsO.hpp"

#include <algorithm>
#include <climits>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

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

        void Ajouter64(std::vector<std::uint8_t>& sortie, std::uint64_t valeur)
        {
            for (int index = 0; index < 8; ++index)
                sortie.push_back(static_cast<std::uint8_t>(valeur >> (index * 8)));
        }

        std::uint16_t Lire16(
            const std::vector<std::uint8_t>& contenu,
            std::size_t position)
        {
            if (position > contenu.size() || contenu.size() - position < 2)
                throw std::runtime_error("objet GsObj tronqué");
            return static_cast<std::uint16_t>(contenu[position])
                | static_cast<std::uint16_t>(contenu[position + 1] << 8);
        }

        std::uint32_t Lire32(
            const std::vector<std::uint8_t>& contenu,
            std::size_t position)
        {
            if (position > contenu.size() || contenu.size() - position < 4)
                throw std::runtime_error("objet GsObj tronqué");
            std::uint32_t valeur = 0;
            for (int index = 0; index < 4; ++index)
                valeur |= static_cast<std::uint32_t>(contenu[position + index])
                    << (index * 8);
            return valeur;
        }

        std::uint64_t Lire64(
            const std::vector<std::uint8_t>& contenu,
            std::size_t position)
        {
            if (position > contenu.size() || contenu.size() - position < 8)
                throw std::runtime_error("objet GsObj tronqué");
            std::uint64_t valeur = 0;
            for (int index = 0; index < 8; ++index)
                valeur |= static_cast<std::uint64_t>(contenu[position + index])
                    << (index * 8);
            return valeur;
        }

        std::uint64_t Aligner16(std::uint64_t valeur)
        {
            if (valeur > std::numeric_limits<std::uint64_t>::max() - 15)
                throw std::overflow_error("taille GsObj trop grande");
            return (valeur + 15) & ~std::uint64_t{15};
        }

        void ExigerPlage(
            std::uint64_t position,
            std::uint64_t taille,
            std::size_t tailleFichier,
            const char* description)
        {
            const auto limite = static_cast<std::uint64_t>(tailleFichier);
            if (position > limite || taille > limite - position)
                throw std::runtime_error(std::string("plage GsObj invalide : ") + description);
        }

        std::vector<std::uint8_t> LireFichier(const std::filesystem::path& chemin)
        {
            std::ifstream flux(chemin, std::ios::binary);
            if (!flux)
                throw std::runtime_error("impossible d’ouvrir l’objet GsObj : " + chemin.string());
            flux.seekg(0, std::ios::end);
            const auto taille = flux.tellg();
            if (taille < 0) throw std::runtime_error("taille d’objet GsObj invalide");
            flux.seekg(0, std::ios::beg);
            std::vector<std::uint8_t> contenu(static_cast<std::size_t>(taille));
            flux.read(
                reinterpret_cast<char*>(contenu.data()),
                static_cast<std::streamsize>(contenu.size()));
            if (!flux && !contenu.empty())
                throw std::runtime_error("échec de lecture de l’objet GsObj");
            return contenu;
        }

        void VerifierTexteUtf8(
            const std::string& valeur,
            std::size_t maximum,
            const char* description,
            bool videAutorise)
        {
            if (!videAutorise && valeur.empty())
                throw std::runtime_error(std::string(description) + " vide");
            if (valeur.size() > maximum)
                throw std::runtime_error(std::string(description) + " trop long");
            if (valeur.find('\0') != std::string::npos)
                throw std::runtime_error(std::string(description) + " contenant un octet nul");
            if (!valeur.empty()
                && !Utf8GsEValide(
                    reinterpret_cast<const std::uint8_t*>(valeur.data()),
                    valeur.size()))
                throw std::runtime_error(std::string(description) + " en UTF-8 invalide");
        }
    }

    std::vector<std::uint8_t> EcrivainGsO::Construire(
        const CodeMachine& machine) const
    {
        if (machine.Texte.size() > UINT32_MAX
            || machine.Donnees.size() > UINT32_MAX)
            throw std::overflow_error("une section GsObj dépasse 4 Gio");
        if (machine.Symboles.size() > UINT32_MAX
            || machine.Relocalisations.size() > UINT32_MAX)
            throw std::overflow_error("trop d’entrées dans l’objet GsObj");

        std::unordered_map<std::string, std::uint32_t> indicesSymboles;
        for (std::size_t index = 0; index < machine.Symboles.size(); ++index)
        {
            const auto& symbole = machine.Symboles[index];
            VerifierTexteUtf8(
                symbole.Nom, TailleNomSymboleGsOMaximale,
                "nom de symbole GsObj", false);
            VerifierTexteUtf8(
                symbole.SignatureAbi, TailleSignatureAbiGsOMaximale,
                "signature ABI GsObj", false);
            VerifierTexteUtf8(
                symbole.Position.Fichier, TailleCheminSourceGsOMaximale,
                "chemin source GsObj", true);
            if (!indicesSymboles.emplace(
                    symbole.Nom, static_cast<std::uint32_t>(index)).second)
                throw std::runtime_error("symbole dupliqué dans l’objet GsObj : " + symbole.Nom);
        }

        std::vector<std::uint8_t> chaines{0};
        std::unordered_map<std::string, std::uint32_t> positionsChaines{{"", 0}};
        auto ajouterChaine = [&](const std::string& valeur) -> std::uint32_t
        {
            if (const auto trouve = positionsChaines.find(valeur);
                trouve != positionsChaines.end())
                return trouve->second;
            if (chaines.size() > UINT32_MAX
                || valeur.size() + 1 > UINT32_MAX - chaines.size())
                throw std::overflow_error("table de chaînes GsObj trop grande");
            const auto position = static_cast<std::uint32_t>(chaines.size());
            positionsChaines.emplace(valeur, position);
            chaines.insert(chaines.end(), valeur.begin(), valeur.end());
            chaines.push_back(0);
            return position;
        };

        std::vector<std::uint8_t> tableSymboles;
        tableSymboles.reserve(machine.Symboles.size() * TailleEntreeSymboleGsO);
        for (const auto& symbole : machine.Symboles)
        {
            Ajouter32(tableSymboles, ajouterChaine(symbole.Nom));
            Ajouter16(tableSymboles, static_cast<std::uint16_t>(symbole.Nom.size()));
            tableSymboles.push_back(
                symbole.Genre == GenreSymboleMachine::Fonction ? 1 : 2);
            tableSymboles.push_back(static_cast<std::uint8_t>(symbole.Section));
            Ajouter32(tableSymboles,
                (symbole.EstPublic ? 1U : 0U) | (symbole.EstDefini ? 2U : 0U));
            Ajouter32(tableSymboles, symbole.Decalage);
            Ajouter32(tableSymboles, symbole.Taille);
            Ajouter32(tableSymboles, ajouterChaine(symbole.SignatureAbi));
            Ajouter16(tableSymboles, static_cast<std::uint16_t>(symbole.SignatureAbi.size()));
            Ajouter16(tableSymboles, 0);
            Ajouter32(tableSymboles, ajouterChaine(symbole.Position.Fichier));
            Ajouter16(tableSymboles, static_cast<std::uint16_t>(symbole.Position.Fichier.size()));
            Ajouter16(tableSymboles, 0);
            if (symbole.Position.Ligne > UINT32_MAX
                || symbole.Position.Colonne > UINT32_MAX)
                throw std::overflow_error("position source GsObj hors plage");
            Ajouter32(tableSymboles, static_cast<std::uint32_t>(symbole.Position.Ligne));
            Ajouter32(tableSymboles, static_cast<std::uint32_t>(symbole.Position.Colonne));
            Ajouter32(tableSymboles, 0);
        }

        std::vector<std::uint8_t> tableRelocalisations;
        tableRelocalisations.reserve(
            machine.Relocalisations.size() * TailleEntreeRelocalisationGsO);
        for (const auto& relocalisation : machine.Relocalisations)
        {
            const auto symbole = indicesSymboles.find(relocalisation.Symbole);
            if (symbole == indicesSymboles.end())
                throw std::runtime_error(
                    "cible de relocalisation GsObj absente : " + relocalisation.Symbole);
            Ajouter32(tableRelocalisations, relocalisation.Decalage);
            Ajouter32(tableRelocalisations, symbole->second);
            tableRelocalisations.push_back(static_cast<std::uint8_t>(relocalisation.Section));
            tableRelocalisations.push_back(static_cast<std::uint8_t>(relocalisation.Type));
            Ajouter16(tableRelocalisations, 0);
            Ajouter64(tableRelocalisations, 0);
            Ajouter32(tableRelocalisations, 0);
        }

        const std::uint64_t positionSymboles = TailleEnteteGsO;
        const std::uint64_t positionRelocalisations = positionSymboles + tableSymboles.size();
        const std::uint64_t positionChaines = positionRelocalisations + tableRelocalisations.size();
        const std::uint64_t positionTexte = Aligner16(positionChaines + chaines.size());
        const std::uint64_t positionDonnees = Aligner16(positionTexte + machine.Texte.size());
        const std::uint64_t tailleTotale = positionDonnees + machine.Donnees.size();
        if (tailleTotale > SIZE_MAX)
            throw std::overflow_error("objet GsObj non représentable en mémoire");

        std::vector<std::uint8_t> sortie;
        sortie.reserve(static_cast<std::size_t>(tailleTotale));
        for (const auto octet : SignatureGsObj) sortie.push_back(octet);
        sortie.push_back(0);
        Ajouter16(sortie, VersionMajeureGsO);
        Ajouter16(sortie, VersionMineureGsO);
        Ajouter16(sortie, ArchitectureAmd64GsO);
        Ajouter16(sortie, VersionAbiGsO);
        Ajouter32(sortie, static_cast<std::uint32_t>(TailleEnteteGsO));
        Ajouter32(sortie, 0);
        Ajouter32(sortie, static_cast<std::uint32_t>(machine.Symboles.size()));
        Ajouter32(sortie, static_cast<std::uint32_t>(machine.Relocalisations.size()));
        Ajouter64(sortie, machine.Texte.size());
        Ajouter64(sortie, machine.Donnees.size());
        Ajouter64(sortie, machine.TailleZero);
        Ajouter64(sortie, positionTexte);
        Ajouter64(sortie, positionDonnees);
        Ajouter64(sortie, positionSymboles);
        Ajouter64(sortie, positionRelocalisations);
        Ajouter64(sortie, positionChaines);
        Ajouter64(sortie, chaines.size());
        Ajouter32(sortie, 0);
        Ajouter32(sortie, 0);
        if (sortie.size() != TailleEnteteGsO)
            throw std::logic_error("taille interne d’en-tête GsObj incorrecte");

        sortie.insert(sortie.end(), tableSymboles.begin(), tableSymboles.end());
        sortie.insert(
            sortie.end(), tableRelocalisations.begin(), tableRelocalisations.end());
        sortie.insert(sortie.end(), chaines.begin(), chaines.end());
        sortie.resize(static_cast<std::size_t>(positionTexte), 0);
        sortie.insert(sortie.end(), machine.Texte.begin(), machine.Texte.end());
        sortie.resize(static_cast<std::size_t>(positionDonnees), 0);
        sortie.insert(sortie.end(), machine.Donnees.begin(), machine.Donnees.end());
        return sortie;
    }

    void EcrivainGsO::Ecrire(
        const CodeMachine& machine,
        const std::filesystem::path& chemin) const
    {
        const auto contenu = Construire(machine);
        std::ofstream flux(chemin, std::ios::binary | std::ios::trunc);
        if (!flux)
            throw std::runtime_error("impossible d’ouvrir la sortie GsObj : " + chemin.string());
        flux.write(
            reinterpret_cast<const char*>(contenu.data()),
            static_cast<std::streamsize>(contenu.size()));
        if (!flux) throw std::runtime_error("échec de l’écriture de l’objet GsObj");
    }

    CodeMachine LecteurGsO::Lire(const std::vector<std::uint8_t>& contenu) const
    {
        if (contenu.size() < TailleEnteteGsO)
            throw std::runtime_error("objet GsObj tronqué");
        constexpr std::array<std::uint8_t, 6> ancienneSignature{
            'G', 'S', 'O', 'B', 'J', 0};
        if (std::equal(
                ancienneSignature.begin(), ancienneSignature.end(), contenu.begin()))
            throw std::runtime_error(
                "ancien format GsObj GSOBJ\\0 non pris en charge ; recompilez l’unité");
        if (!std::equal(
                SignatureGsObj.begin(), SignatureGsObj.end(), contenu.begin()))
            throw std::runtime_error("signature d’objet GsObj invalide");
        if (contenu[7] != 0)
            throw std::runtime_error("octet réservé GsObj invalide");
        if (Lire16(contenu, 8) != VersionMajeureGsO
            || Lire16(contenu, 10) != VersionMineureGsO)
            throw std::runtime_error("version d’objet GsObj incompatible");
        if (Lire16(contenu, 12) != ArchitectureAmd64GsO)
            throw std::runtime_error("architecture d’objet GsObj incompatible");
        if (Lire16(contenu, 14) != VersionAbiGsO)
            throw std::runtime_error("version ABI d’objet GsObj incompatible");
        if (Lire32(contenu, 16) != TailleEnteteGsO)
            throw std::runtime_error("taille d’en-tête GsObj invalide");

        if (Lire32(contenu, 20) != 0
            || Lire32(contenu, 104) != 0
            || Lire32(contenu, 108) != 0)
            throw std::runtime_error("champ réservé GsObj non nul");
        const auto nombreSymboles = Lire32(contenu, 24);
        const auto nombreRelocalisations = Lire32(contenu, 28);
        if (nombreSymboles > 1'000'000 || nombreRelocalisations > 4'000'000)
            throw std::runtime_error("nombre d’entrées GsObj déraisonnable");
        const auto tailleTexte = Lire64(contenu, 32);
        const auto tailleDonnees = Lire64(contenu, 40);
        const auto tailleZero = Lire64(contenu, 48);
        const auto positionTexte = Lire64(contenu, 56);
        const auto positionDonnees = Lire64(contenu, 64);
        const auto positionSymboles = Lire64(contenu, 72);
        const auto positionRelocalisations = Lire64(contenu, 80);
        const auto positionChaines = Lire64(contenu, 88);
        const auto tailleChaines = Lire64(contenu, 96);
        if (tailleTexte > UINT32_MAX || tailleDonnees > UINT32_MAX
            || tailleZero > UINT32_MAX)
            throw std::runtime_error("section GsObj supérieure à 4 Gio");

        ExigerPlage(positionTexte, tailleTexte, contenu.size(), "texte");
        ExigerPlage(positionDonnees, tailleDonnees, contenu.size(), "données");
        ExigerPlage(
            positionSymboles,
            static_cast<std::uint64_t>(nombreSymboles) * TailleEntreeSymboleGsO,
            contenu.size(), "symboles");
        ExigerPlage(
            positionRelocalisations,
            static_cast<std::uint64_t>(nombreRelocalisations)
                * TailleEntreeRelocalisationGsO,
            contenu.size(), "relocalisations");
        ExigerPlage(positionChaines, tailleChaines, contenu.size(), "chaînes");
        if (tailleChaines == 0 || contenu[static_cast<std::size_t>(positionChaines)] != 0)
            throw std::runtime_error("table de chaînes GsObj invalide");

        auto lireChaine = [&](std::uint32_t position, std::uint16_t taille,
                              std::size_t maximum, const char* description,
                              bool videAutorise) -> std::string
        {
            if (position > tailleChaines
                || static_cast<std::uint64_t>(taille) + 1 > tailleChaines - position)
                throw std::runtime_error(std::string("référence de chaîne GsObj invalide : ") + description);
            const auto absolue = static_cast<std::size_t>(positionChaines + position);
            if (contenu[absolue + taille] != 0)
                throw std::runtime_error(std::string("chaîne GsObj non terminée : ") + description);
            std::string valeur(
                reinterpret_cast<const char*>(contenu.data() + absolue), taille);
            VerifierTexteUtf8(valeur, maximum, description, videAutorise);
            return valeur;
        };

        CodeMachine machine;
        machine.Texte.assign(
            contenu.begin() + static_cast<std::ptrdiff_t>(positionTexte),
            contenu.begin() + static_cast<std::ptrdiff_t>(positionTexte + tailleTexte));
        machine.Donnees.assign(
            contenu.begin() + static_cast<std::ptrdiff_t>(positionDonnees),
            contenu.begin() + static_cast<std::ptrdiff_t>(positionDonnees + tailleDonnees));
        machine.TailleZero = static_cast<std::uint32_t>(tailleZero);
        machine.Symboles.reserve(nombreSymboles);
        std::unordered_set<std::string> nomsSymboles;
        for (std::uint32_t index = 0; index < nombreSymboles; ++index)
        {
            const auto position = static_cast<std::size_t>(positionSymboles)
                + static_cast<std::size_t>(index) * TailleEntreeSymboleGsO;
            SymboleMachine symbole;
            symbole.Nom = lireChaine(
                Lire32(contenu, position), Lire16(contenu, position + 4),
                TailleNomSymboleGsOMaximale, "nom de symbole GsObj", false);
            const auto genre = contenu[position + 6];
            if (genre != 1 && genre != 2)
                throw std::runtime_error("genre de symbole GsObj invalide");
            symbole.Genre = genre == 1
                ? GenreSymboleMachine::Fonction : GenreSymboleMachine::Objet;
            const auto section = contenu[position + 7];
            if (section > static_cast<std::uint8_t>(SectionMachine::Indefinie))
                throw std::runtime_error("section de symbole GsObj invalide");
            symbole.Section = static_cast<SectionMachine>(section);
            const auto drapeaux = Lire32(contenu, position + 8);
            if ((drapeaux & ~3U) != 0)
                throw std::runtime_error("drapeaux de symbole GsObj invalides");
            symbole.EstPublic = (drapeaux & 1U) != 0;
            symbole.EstDefini = (drapeaux & 2U) != 0;
            symbole.Decalage = Lire32(contenu, position + 12);
            symbole.Taille = Lire32(contenu, position + 16);
            symbole.SignatureAbi = lireChaine(
                Lire32(contenu, position + 20), Lire16(contenu, position + 24),
                TailleSignatureAbiGsOMaximale, "signature ABI GsObj", false);
            symbole.Position.Fichier = lireChaine(
                Lire32(contenu, position + 28), Lire16(contenu, position + 32),
                TailleCheminSourceGsOMaximale, "chemin source GsObj", true);
            symbole.Position.Ligne = Lire32(contenu, position + 36);
            symbole.Position.Colonne = Lire32(contenu, position + 40);
            if (!nomsSymboles.insert(symbole.Nom).second)
                throw std::runtime_error("symbole GsObj dupliqué : " + symbole.Nom);
            if (symbole.EstDefini == (symbole.Section == SectionMachine::Indefinie))
                throw std::runtime_error("état de définition GsObj incohérent pour " + symbole.Nom);
            std::uint64_t tailleSection = 0;
            if (symbole.Section == SectionMachine::Texte) tailleSection = tailleTexte;
            else if (symbole.Section == SectionMachine::Donnees) tailleSection = tailleDonnees;
            else if (symbole.Section == SectionMachine::Zero) tailleSection = tailleZero;
            if (symbole.EstDefini
                && (symbole.Decalage > tailleSection
                    || symbole.Taille > tailleSection - symbole.Decalage))
                throw std::runtime_error("plage de symbole GsObj invalide : " + symbole.Nom);
            machine.Symboles.push_back(std::move(symbole));
        }

        machine.Relocalisations.reserve(nombreRelocalisations);
        for (std::uint32_t index = 0; index < nombreRelocalisations; ++index)
        {
            const auto position = static_cast<std::size_t>(positionRelocalisations)
                + static_cast<std::size_t>(index) * TailleEntreeRelocalisationGsO;
            CodeMachine::Relocalisation relocalisation;
            relocalisation.Decalage = Lire32(contenu, position);
            const auto indexSymbole = Lire32(contenu, position + 4);
            if (indexSymbole >= machine.Symboles.size())
                throw std::runtime_error("index de symbole de relocalisation GsObj invalide");
            relocalisation.Symbole = machine.Symboles[indexSymbole].Nom;
            const auto section = contenu[position + 8];
            if (section != static_cast<std::uint8_t>(SectionMachine::Texte)
                && section != static_cast<std::uint8_t>(SectionMachine::Donnees))
                throw std::runtime_error("section de relocalisation GsObj invalide");
            relocalisation.Section = static_cast<SectionMachine>(section);
            const auto type = contenu[position + 9];
            if (type > static_cast<std::uint8_t>(TypeRelocalisationMachine::Adresse64))
                throw std::runtime_error("type de relocalisation GsObj invalide");
            relocalisation.Type = static_cast<TypeRelocalisationMachine>(type);
            if (Lire64(contenu, position + 12) != 0)
                throw std::runtime_error("ajout de relocalisation GsObj non pris en charge");
            const std::uint64_t tailleSection =
                relocalisation.Section == SectionMachine::Texte
                ? tailleTexte : tailleDonnees;
            const std::uint64_t largeur =
                relocalisation.Type == TypeRelocalisationMachine::Relatif32 ? 4 : 8;
            if (relocalisation.Decalage > tailleSection
                || largeur > tailleSection - relocalisation.Decalage)
                throw std::runtime_error("plage de relocalisation GsObj invalide");
            machine.Relocalisations.push_back(std::move(relocalisation));
        }
        return machine;
    }

    CodeMachine LecteurGsO::Lire(const std::filesystem::path& chemin) const
    {
        return Lire(LireFichier(chemin));
    }
}
