#include "GsPP/BibliothequeGsA.hpp"
#include "GsPP/Compilation.hpp"
#include "GsPP/ConstructeurProjet.hpp"
#include "GsPP/EditeurLiens.hpp"
#include "GsPP/EcrivainCoff.hpp"
#include "GsPP/EcrivainGsE.hpp"
#include "GsPP/ErreurCompilation.hpp"
#include "GsPP/GenerateurX64.hpp"
#include "GsPP/ObjetGsO.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    enum class FormatSortie { Coff, GsE, GsO, GsA };

    std::string ExtensionMinuscule(const std::filesystem::path& chemin)
    {
        auto extension = chemin.extension().string();
        std::transform(
            extension.begin(), extension.end(), extension.begin(),
            [](unsigned char caractere)
            {
                return static_cast<char>(std::tolower(caractere));
            });
        return extension;
    }

    bool EstProjet(const std::filesystem::path& chemin)
    {
        const auto extension = ExtensionMinuscule(chemin);
        return extension == ".gspj" || extension == ".gsproject";
    }

    bool EstSolution(const std::filesystem::path& chemin)
    {
        return ExtensionMinuscule(chemin) == ".gsps";
    }

    bool EstObjet(const std::filesystem::path& chemin)
    {
        return ExtensionMinuscule(chemin) == ".gsobj";
    }

    bool EstBibliotheque(const std::filesystem::path& chemin)
    {
        return ExtensionMinuscule(chemin) == ".gsa";
    }

    std::vector<std::uint8_t> LireBinaire(const std::filesystem::path& chemin)
    {
        std::ifstream flux(chemin, std::ios::binary);
        if (!flux) throw std::runtime_error("impossible d’ouvrir le fichier : " + chemin.string());
        flux.seekg(0, std::ios::end);
        const auto taille = flux.tellg();
        if (taille < 0) throw std::runtime_error("taille de fichier invalide : " + chemin.string());
        flux.seekg(0, std::ios::beg);
        std::vector<std::uint8_t> contenu(static_cast<std::size_t>(taille));
        flux.read(
            reinterpret_cast<char*>(contenu.data()),
            static_cast<std::streamsize>(contenu.size()));
        if (!flux && !contenu.empty())
            throw std::runtime_error("échec de lecture : " + chemin.string());
        return contenu;
    }

    void CreerParent(const std::filesystem::path& chemin)
    {
        if (!chemin.parent_path().empty())
            std::filesystem::create_directories(chemin.parent_path());
    }

    void AfficherAide()
    {
        std::cout
            << "Compilateur Gs++ 0.27.0-alpha.6 — frontend auto-hébergé partiel, formats natifs 1.0 ABI 1\n\n"
            << "Utilisation :\n"
            << "  gsppc <sources/interfaces...> --format gsobj -o module.GsObj\n"
            << "  gsppc <objets.GsObj...> [bibliotheques.GsA...] --format gse -o application.GsE\n"
            << "  gsppc <objets.GsObj...> --format gsa -o bibliotheque.GsA\n"
            << "  gsppc <projet.GsPj|solution.GsPs>\n\n"
            << "Extensions Gs++ :\n"
            << "  sources     .Gs++ | .GsPP | .GsPlusPlus\n"
            << "  interfaces .HGs++ | .HGsPP | .HeaderGsPlusPlus\n\n"
            << "Options :\n"
            << "  --format obj|gsobj|gsa|gse       format de sortie\n"
            << "  --point-entree <nom>             point d’entrée du GsE\n"
            << "  --carte <chemin>                 carte de liens avec sources et ABI\n"
            << "  --repertoire-objets <chemin>     remplace le dossier objets d’un projet\n"
            << "  --nom <nom>                      nom dans les métadonnées GsE\n"
            << "  --version-application <version>  version dans les métadonnées\n"
            << "  --editeur <nom>                  éditeur dans les métadonnées\n"
            << "  --jetons                         afficher les jetons\n"
            << "  --ast                            afficher l’AST et les dispositions\n"
            << "  --langue-diagnostics français|anglais\n"
            << "  --version                        afficher la version\n";
    }

    std::string PointEntreeParDefaut(const GsPP::Programme& programme)
    {
        std::string resultat;
        for (const auto& fonction : programme.Fonctions)
        {
            const bool principal = fonction.Nom == "Principal" || fonction.Nom == "Main";
            if (!principal || !fonction.EstPublique || fonction.EstExterne) continue;
            if (!resultat.empty() && resultat != fonction.NomComplet())
                throw std::runtime_error(
                    "plusieurs points d’entrée possibles ; utilisez --point-entree");
            resultat = fonction.NomComplet();
        }
        if (resultat.empty())
            throw std::runtime_error("aucun point d’entrée public Principal/Main trouvé");
        return resultat;
    }

    std::string PointEntreeParDefaut(
        const std::vector<GsPP::UniteLiaison>& objets,
        const std::vector<GsPP::BibliothequeLiaison>& bibliotheques)
    {
        std::string resultat;
        auto examiner = [&](const GsPP::CodeMachine& machine)
        {
            for (const auto& symbole : machine.Symboles)
            {
                if (!symbole.EstDefini || !symbole.EstPublic
                    || symbole.Genre != GsPP::GenreSymboleMachine::Fonction)
                    continue;
                const auto separateur = symbole.Nom.rfind("::");
                const auto nomCourt = separateur == std::string::npos
                    ? symbole.Nom : symbole.Nom.substr(separateur + 2);
                if (nomCourt != "Principal" && nomCourt != "Main") continue;
                if (!resultat.empty() && resultat != symbole.Nom)
                    throw std::runtime_error(
                        "plusieurs points d’entrée possibles ; utilisez --point-entree");
                resultat = symbole.Nom;
            }
        };
        for (const auto& objet : objets) examiner(objet.Machine);
        for (const auto& bibliotheque : bibliotheques)
            for (const auto& membre : bibliotheque) examiner(membre.Machine);
        if (resultat.empty())
            throw std::runtime_error("aucun point d’entrée public Principal/Main trouvé");
        return resultat;
    }

    void AfficherAst(const GsPP::Programme& programme)
    {
        for (const auto& enumeration : programme.Enumerations)
        {
            std::cout << "énumération " << enumeration.NomComplet() << '\n';
            for (const auto& valeur : enumeration.Valeurs)
                std::cout << "  " << valeur.Nom << " = " << valeur.Valeur << '\n';
        }
        for (const auto& structure : programme.Structures)
        {
            std::cout << (structure.EstUnion ? "union " : "structure ")
                      << structure.NomComplet()
                      << " taille=" << structure.Taille
                      << " alignement=" << structure.Alignement << '\n';
            for (const auto& champ : structure.Champs)
                std::cout << "  +" << champ.Decalage << ' '
                          << champ.Type.Afficher() << ' ' << champ.Nom << '\n';
            for (const auto& alias : structure.AliasesChamps)
                std::cout << "  alias " << alias.Nom << " = "
                          << alias.CibleCanonique << '\n';
        }
        for (const auto& fonction : programme.Fonctions)
            std::cout << (fonction.EstExterne ? "déclaration " : "fonction ")
                      << fonction.NomComplet() << " -> "
                      << fonction.TypeRetour.Afficher() << '\n';
        for (const auto& variable : programme.VariablesGlobales)
            std::cout << (variable.EstExterne ? "déclaration globale " : "globale ")
                      << variable.NomComplet() << " : " << variable.Type.Afficher()
                      << (variable.EstInitialisee ? " initialisée" : " zéro") << '\n';
        for (const auto& alias : programme.Aliases)
            std::cout << "alias " << alias.NomComplet() << " = "
                      << alias.CibleCanonique << '\n';
    }

    std::string SortieParDefaut(FormatSortie format)
    {
        if (format == FormatSortie::GsE) return "Application.GsE";
        if (format == FormatSortie::GsO) return "Module.GsObj";
        if (format == FormatSortie::GsA) return "Bibliotheque.GsA";
        return "Application.obj";
    }
}

int main(int argc, char** argv)
{
    using namespace GsPP;
    if (argc <= 1) { AfficherAide(); return 1; }

    std::vector<std::filesystem::path> entrees;
    std::filesystem::path sortie;
    std::filesystem::path carte;
    std::filesystem::path repertoireObjets;
    bool afficherJetons = false;
    bool afficherAst = false;
    bool formatExplicite = false;
    FormatSortie format = FormatSortie::Coff;
    std::string pointEntree;
    MetadonneesGsE metadonnees;
    LangueDiagnostic langue = LangueDiagnostic::Francais;

    try
    {
        for (int index = 1; index < argc; ++index)
        {
            const std::string argument = argv[index];
            if (argument == "--version")
            { std::cout << "Gs++ Compiler 0.27.0-alpha.6\n"; return 0; }
            if (argument == "--aide" || argument == "--help" || argument == "-h")
            { AfficherAide(); return 0; }
            if (argument == "--jetons") { afficherJetons = true; continue; }
            if (argument == "--ast") { afficherAst = true; continue; }
            if (argument == "-o")
            {
                if (++index >= argc) throw std::runtime_error("chemin attendu après -o");
                sortie = argv[index];
                continue;
            }
            if (argument == "--repertoire-objets" || argument == "--object-directory")
            {
                if (++index >= argc)
                    throw std::runtime_error(
                        "chemin attendu après --repertoire-objets");
                repertoireObjets = argv[index];
                continue;
            }
            if (argument == "--format")
            {
                if (++index >= argc) throw std::runtime_error("format attendu");
                const std::string valeur = argv[index];
                formatExplicite = true;
                if (valeur == "obj" || valeur == "coff") format = FormatSortie::Coff;
                else if (valeur == "gse" || valeur == "GsE") format = FormatSortie::GsE;
                else if (valeur == "gsobj" || valeur == "GsObj") format = FormatSortie::GsO;
                else if (valeur == "gsa" || valeur == "GsA"
                         || valeur == "bibliotheque" || valeur == "library")
                    format = FormatSortie::GsA;
                else throw std::runtime_error("format inconnu : " + valeur);
                continue;
            }
            if (argument == "--point-entree" || argument == "--entry-point")
            {
                if (++index >= argc) throw std::runtime_error("nom attendu après --point-entree");
                pointEntree = argv[index];
                continue;
            }
            if (argument == "--carte" || argument == "--map")
            {
                if (++index >= argc) throw std::runtime_error("chemin attendu après --carte");
                carte = argv[index];
                continue;
            }
            if (argument == "--nom" || argument == "--name")
            {
                if (++index >= argc) throw std::runtime_error("nom d’application attendu");
                metadonnees.Nom = argv[index];
                continue;
            }
            if (argument == "--version-application" || argument == "--application-version")
            {
                if (++index >= argc) throw std::runtime_error("version d’application attendue");
                metadonnees.Version = argv[index];
                continue;
            }
            if (argument == "--editeur" || argument == "--publisher")
            {
                if (++index >= argc) throw std::runtime_error("nom d’éditeur attendu");
                metadonnees.Editeur = argv[index];
                continue;
            }
            if (argument == "--langue-diagnostics")
            {
                if (++index >= argc) throw std::runtime_error("langue attendue");
                const std::string valeur = argv[index];
                if (valeur == "français" || valeur == "francais" || valeur == "french")
                    langue = LangueDiagnostic::Francais;
                else if (valeur == "anglais" || valeur == "english")
                    langue = LangueDiagnostic::Anglais;
                else throw std::runtime_error("langue inconnue : " + valeur);
                continue;
            }
            if (!argument.empty() && argument[0] == '-')
                throw std::runtime_error("option inconnue : " + argument);
            entrees.emplace_back(argument);
        }

        if (entrees.empty()) throw std::runtime_error("aucun fichier d’entrée indiqué");
        const bool contientProjet = std::any_of(entrees.begin(), entrees.end(), EstProjet);
        const bool contientSolution = std::any_of(entrees.begin(), entrees.end(), EstSolution);
        if (contientProjet || contientSolution)
        {
            if (entrees.size() != 1 || (contientProjet && contientSolution))
                throw std::runtime_error("un projet ou une solution doit être l’unique entrée");
            if (formatExplicite || !pointEntree.empty() || !carte.empty())
                throw std::runtime_error(
                    "le format, le point d’entrée et la carte d’un projet "
                    "sont définis dans le fichier de projet");
            if (contientSolution && (!sortie.empty() || !repertoireObjets.empty()))
                throw std::runtime_error(
                    "les remplacements de sortie s’appliquent uniquement à un projet");
            if (contientProjet)
            {
                OptionsConstructionProjet options;
                options.Sortie = sortie;
                options.RepertoireObjets = repertoireObjets;
                const auto resultat = ConstructeurProjet().Construire(
                    entrees[0], std::cout, options);
                std::cout << "Projet construit : " << resultat.Sortie.string()
                          << " (" << resultat.NombreUnites << " unité(s))\n";
            }
            else
            {
                const auto resultats = ConstructeurProjet().ConstruireSolution(entrees[0], std::cout);
                std::cout << "Solution construite : " << resultats.size() << " projet(s)\n";
            }
            return 0;
        }

        if (!repertoireObjets.empty())
            throw std::runtime_error(
                "--repertoire-objets est réservé à la construction d’un projet");

        const bool contientObjet = std::any_of(entrees.begin(), entrees.end(), EstObjet);
        const bool contientBibliotheque = std::any_of(
            entrees.begin(), entrees.end(), EstBibliotheque);
        const bool contientBinaire = contientObjet || contientBibliotheque;
        const bool contientSource = std::any_of(
            entrees.begin(), entrees.end(),
            [](const std::filesystem::path& chemin)
            { return !EstObjet(chemin) && !EstBibliotheque(chemin); });
        if (contientBinaire && contientSource)
            throw std::runtime_error(
                "les sources et objets ne peuvent pas être mélangés dans la même commande");
        if (contientBinaire && !formatExplicite) format = FormatSortie::GsE;
        if (sortie.empty()) sortie = SortieParDefaut(format);
        if (EstExtensionObsolete(sortie))
            throw std::runtime_error(
                "extension de sortie obsolète refusée : " + sortie.string());
        CreerParent(sortie);
        if (!carte.empty()) CreerParent(carte);

        if (contientSource)
        {
            if (format == FormatSortie::GsA)
                throw std::runtime_error(
                    "compilez d’abord chaque unité en GsObj avant de créer une GsA");
            std::vector<UniteSource> unites;
            for (const auto& chemin : entrees)
                unites.push_back({chemin, EstExtensionInterface(chemin), chemin.string()});
            auto programme = AnalyserUnites(
                unites, afficherJetons ? &std::cout : nullptr);
            if (afficherAst) AfficherAst(programme);
            const auto machine = GenerateurX64().Generer(programme);
            if (format == FormatSortie::GsE)
            {
                if (pointEntree.empty()) pointEntree = PointEntreeParDefaut(programme);
                EcrivainGsE().Ecrire(machine, pointEntree, sortie, metadonnees);
                if (!carte.empty()) EditeurLiens().EcrireCarte(machine, carte);
            }
            else if (format == FormatSortie::GsO)
                EcrivainGsO().Ecrire(machine, sortie);
            else EcrivainCoff().Ecrire(machine, sortie);
            std::cout << "Compilation réussie : " << sortie.string()
                      << " (" << entrees.size() << " unité(s), "
                      << machine.Texte.size() << " octets de code, "
                      << machine.Donnees.size() << " octets de données, "
                      << machine.TailleZero << " octets zéro, "
                      << machine.Symboles.size() << " symbole(s), "
                      << machine.Relocalisations.size() << " relocalisation(s))\n";
            return 0;
        }

        if (afficherJetons || afficherAst)
            throw std::runtime_error("--jetons et --ast ne s’appliquent qu’aux sources");
        std::vector<UniteLiaison> objets;
        std::vector<BibliothequeLiaison> bibliotheques;
        std::vector<MembreGsA> membresArchive;
        for (const auto& chemin : entrees)
        {
            if (EstObjet(chemin))
            {
                const auto contenu = LireBinaire(chemin);
                objets.push_back({chemin.filename().string(), LecteurGsO().Lire(contenu)});
                membresArchive.push_back({chemin.filename().string(), contenu});
            }
            else
            {
                BibliothequeLiaison bibliotheque;
                for (auto& membre : LecteurGsA().Lire(chemin))
                    bibliotheque.push_back({
                        chemin.filename().string() + '(' + membre.Nom + ')',
                        LecteurGsO().Lire(membre.Objet)});
                bibliotheques.push_back(std::move(bibliotheque));
            }
        }

        if (format == FormatSortie::GsA)
        {
            if (contientBibliotheque)
                throw std::runtime_error("une bibliothèque GsA ne peut contenir que des objets GsObj");
            EcrivainGsA().Ecrire(membresArchive, sortie);
            std::cout << "Bibliothèque créée : " << sortie.string()
                      << " (" << membresArchive.size() << " membre(s))\n";
            return 0;
        }
        if (format != FormatSortie::GsE)
            throw std::runtime_error("la liaison d’objets produit un GsE ou une GsA");
        if (pointEntree.empty()) pointEntree = PointEntreeParDefaut(objets, bibliotheques);
        const auto machine = EditeurLiens().Lier(
            std::move(objets), bibliotheques, pointEntree);
        EcrivainGsE().Ecrire(machine, pointEntree, sortie, metadonnees);
        if (!carte.empty()) EditeurLiens().EcrireCarte(machine, carte);
        std::cout << "Édition de liens réussie : " << sortie.string()
                  << " (" << machine.Symboles.size() << " symbole(s), "
                  << machine.Relocalisations.size() << " relocalisation(s))\n";
        return 0;
    }
    catch (const ErreurCompilation& erreur)
    {
        if (!erreur.Fichier().empty()) std::cerr << erreur.Fichier() << ':';
        std::cerr << erreur.Ligne() << ':' << erreur.Colonne()
                  << " GS1001 : " << erreur.Message(langue) << '\n';
        return 2;
    }
    catch (const std::exception& erreur)
    {
        std::cerr << "GS0001 : " << erreur.what() << '\n';
        return 1;
    }
}
