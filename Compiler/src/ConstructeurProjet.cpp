#include "GsPP/ConstructeurProjet.hpp"

#include "GsPP/BibliothequeGsA.hpp"
#include "GsPP/Compilation.hpp"
#include "GsPP/EditeurLiens.hpp"
#include "GsPP/EcrivainGsE.hpp"
#include "GsPP/GenerateurX64.hpp"
#include "GsPP/ObjetGsO.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace GsPP
{
    namespace
    {
        struct LigneConfiguration
        {
            std::string Cle;
            std::string Valeur;
            std::size_t Numero = 0;
        };

        struct ConfigurationProjet
        {
            std::string Nom;
            std::string Type = "executable";
            std::filesystem::path Sortie;
            std::filesystem::path RepertoireObjets;
            std::filesystem::path Carte;
            std::string PointEntree;
            bool CompilationAgregee = false;
            std::vector<std::pair<std::filesystem::path, std::string>> Sources;
            std::vector<std::pair<std::filesystem::path, std::string>> Interfaces;
            std::vector<std::filesystem::path> Bibliotheques;
            MetadonneesGsE Metadonnees;
        };

        std::string Minuscule(std::string valeur)
        {
            std::transform(
                valeur.begin(), valeur.end(), valeur.begin(),
                [](unsigned char caractere)
                {
                    return static_cast<char>(std::tolower(caractere));
                });
            return valeur;
        }

        struct NoeudXml
        {
            std::string Nom;
            std::unordered_map<std::string, std::string> Attributs;
            std::vector<NoeudXml> Enfants;
            std::size_t Ligne = 1;
        };

        class LecteurXmlProjet final
        {
        public:
            LecteurXmlProjet(std::string texte, std::filesystem::path chemin)
                : _Texte(std::move(texte)), _Chemin(std::move(chemin))
            {
            }

            [[nodiscard]] NoeudXml Lire()
            {
                if (_Texte.size() >= 3
                    && static_cast<unsigned char>(_Texte[0]) == 0xEF
                    && static_cast<unsigned char>(_Texte[1]) == 0xBB
                    && static_cast<unsigned char>(_Texte[2]) == 0xBF)
                    _Position = 3;
                IgnorerSeparations();
                if (CommencePar("<?xml"))
                {
                    while (!EstFin() && !CommencePar("?>")) Avancer();
                    if (EstFin()) Erreur("déclaration XML non terminée");
                    Avancer();
                    Avancer();
                }
                IgnorerSeparations();
                if (EstFin()) Erreur("document XML vide");
                auto racine = LireNoeud();
                IgnorerSeparations();
                if (!EstFin()) Erreur("contenu après l’élément racine XML");
                return racine;
            }

        private:
            [[noreturn]] void Erreur(const std::string& message) const
            {
                throw std::runtime_error(
                    _Chemin.string() + ':' + std::to_string(_Ligne)
                    + " : " + message);
            }

            [[nodiscard]] bool EstFin() const noexcept
            {
                return _Position >= _Texte.size();
            }

            [[nodiscard]] char Courant() const noexcept
            {
                return EstFin() ? '\0' : _Texte[_Position];
            }

            [[nodiscard]] bool CommencePar(std::string_view texte) const
            {
                return _Texte.compare(_Position, texte.size(), texte) == 0;
            }

            char Avancer()
            {
                const char valeur = Courant();
                if (!EstFin())
                {
                    ++_Position;
                    if (valeur == '\n') ++_Ligne;
                }
                return valeur;
            }

            void Exiger(char attendu, const std::string& message)
            {
                if (Courant() != attendu) Erreur(message);
                Avancer();
            }

            void IgnorerCommentaire()
            {
                if (!CommencePar("<!--")) Erreur("commentaire XML attendu");
                for (int index = 0; index < 4; ++index) Avancer();
                while (!EstFin() && !CommencePar("-->")) Avancer();
                if (EstFin()) Erreur("commentaire XML non terminé");
                Avancer();
                Avancer();
                Avancer();
            }

            void IgnorerSeparations()
            {
                while (true)
                {
                    while (!EstFin()
                           && std::isspace(
                               static_cast<unsigned char>(Courant())) != 0)
                        Avancer();
                    if (CommencePar("<!--")) IgnorerCommentaire();
                    else return;
                }
            }

            [[nodiscard]] std::string LireNom()
            {
                const auto debut = _Position;
                while (!EstFin())
                {
                    const unsigned char valeur =
                        static_cast<unsigned char>(Courant());
                    if (std::isalnum(valeur) == 0
                        && valeur != '_' && valeur != '-'
                        && valeur != ':' && valeur != '.')
                        break;
                    Avancer();
                }
                if (_Position == debut) Erreur("nom XML attendu");
                return _Texte.substr(debut, _Position - debut);
            }

            [[nodiscard]] char DecoderEntite()
            {
                Exiger('&', "entité XML attendue");
                const auto debut = _Position;
                while (!EstFin() && Courant() != ';') Avancer();
                if (EstFin()) Erreur("entité XML non terminée");
                const auto nom = _Texte.substr(debut, _Position - debut);
                Avancer();
                if (nom == "amp") return '&';
                if (nom == "lt") return '<';
                if (nom == "gt") return '>';
                if (nom == "quot") return '"';
                if (nom == "apos") return '\'';
                Erreur("entité XML inconnue : &" + nom + ';');
            }

            [[nodiscard]] std::string LireValeurAttribut()
            {
                const char guillemet = Courant();
                if (guillemet != '"' && guillemet != '\'')
                    Erreur("valeur d’attribut XML entre guillemets attendue");
                Avancer();
                std::string valeur;
                while (!EstFin() && Courant() != guillemet)
                {
                    if (Courant() == '<')
                        Erreur("caractère < interdit dans un attribut XML");
                    valeur.push_back(
                        Courant() == '&' ? DecoderEntite() : Avancer());
                }
                if (EstFin()) Erreur("valeur d’attribut XML non terminée");
                Avancer();
                return valeur;
            }

            [[nodiscard]] NoeudXml LireNoeud()
            {
                const auto ligne = _Ligne;
                Exiger('<', "élément XML attendu");
                if (Courant() == '/' || Courant() == '!'
                    || Courant() == '?')
                    Erreur("ouverture d’élément XML invalide");
                NoeudXml noeud;
                noeud.Ligne = ligne;
                noeud.Nom = LireNom();

                while (true)
                {
                    IgnorerSeparations();
                    if (CommencePar("/>"))
                    {
                        Avancer();
                        Avancer();
                        return noeud;
                    }
                    if (Courant() == '>')
                    {
                        Avancer();
                        break;
                    }
                    const auto nom = LireNom();
                    IgnorerSeparations();
                    Exiger('=', "signe = attendu après l’attribut " + nom);
                    IgnorerSeparations();
                    auto valeur = LireValeurAttribut();
                    if (!noeud.Attributs.emplace(nom, std::move(valeur)).second)
                        Erreur("attribut XML dupliqué : " + nom);
                }

                while (true)
                {
                    if (EstFin())
                        Erreur("élément XML non terminé : " + noeud.Nom);
                    if (CommencePar("<!--"))
                    {
                        IgnorerCommentaire();
                        continue;
                    }
                    if (CommencePar("</"))
                    {
                        Avancer();
                        Avancer();
                        const auto fermeture = LireNom();
                        if (fermeture != noeud.Nom)
                            Erreur("fermeture XML " + fermeture
                                   + " différente de " + noeud.Nom);
                        IgnorerSeparations();
                        Exiger('>', "fin de fermeture XML attendue");
                        return noeud;
                    }
                    if (Courant() == '<')
                    {
                        noeud.Enfants.push_back(LireNoeud());
                        continue;
                    }
                    if (std::isspace(
                            static_cast<unsigned char>(Courant())) == 0)
                        Erreur("texte inattendu dans l’élément " + noeud.Nom);
                    Avancer();
                }
            }

            std::string _Texte;
            std::filesystem::path _Chemin;
            std::size_t _Position = 0;
            std::size_t _Ligne = 1;
        };

        void VerifierAttributs(
            const NoeudXml& noeud,
            std::initializer_list<std::string_view> autorises,
            const std::filesystem::path& chemin)
        {
            std::unordered_set<std::string_view> ensemble(autorises);
            for (const auto& [nom, _] : noeud.Attributs)
                if (!ensemble.contains(nom))
                    throw std::runtime_error(
                        chemin.string() + ':' + std::to_string(noeud.Ligne)
                        + " : attribut XML inconnu sur " + noeud.Nom
                        + " : " + nom);
        }

        std::optional<std::string> AttributAlias(
            const NoeudXml& noeud,
            std::initializer_list<std::string_view> noms,
            const std::filesystem::path& chemin)
        {
            std::optional<std::string> resultat;
            for (const auto nom : noms)
            {
                const auto trouve = noeud.Attributs.find(std::string(nom));
                if (trouve == noeud.Attributs.end()) continue;
                if (resultat)
                    throw std::runtime_error(
                        chemin.string() + ':' + std::to_string(noeud.Ligne)
                        + " : attributs XML équivalents déclarés plusieurs fois");
                resultat = trouve->second;
            }
            return resultat;
        }

        std::string AttributRequis(
            const NoeudXml& noeud,
            std::initializer_list<std::string_view> noms,
            const std::filesystem::path& chemin)
        {
            const auto valeur = AttributAlias(noeud, noms, chemin);
            if (!valeur || valeur->empty())
                throw std::runtime_error(
                    chemin.string() + ':' + std::to_string(noeud.Ligne)
                    + " : attribut XML requis absent sur " + noeud.Nom);
            return *valeur;
        }

        std::vector<LigneConfiguration> LireConfiguration(
            const std::filesystem::path& chemin,
            const std::string& enteteFrancais,
            const std::string& enteteAnglais)
        {
            std::ifstream flux(chemin, std::ios::binary);
            if (!flux)
                throw std::runtime_error(
                    "impossible d’ouvrir la configuration : " + chemin.string());
            const std::string texte{
                std::istreambuf_iterator<char>(flux),
                std::istreambuf_iterator<char>()};
            const auto racine = LecteurXmlProjet(texte, chemin).Lire();
            std::vector<LigneConfiguration> lignes;

            const bool solution = enteteFrancais == "GsSolution";
            if (solution)
            {
                if (racine.Nom != "GsSolution")
                    throw std::runtime_error(
                        chemin.string() + ':' + std::to_string(racine.Ligne)
                        + " : racine GsSolution attendue");
                VerifierAttributs(racine, {"Version"}, chemin);
                if (AttributRequis(racine, {"Version"}, chemin) != "1.0")
                    throw std::runtime_error(
                        chemin.string() + " : version GsSolution 1.0 attendue");
                for (const auto& enfant : racine.Enfants)
                {
                    if (enfant.Nom != "Projet" && enfant.Nom != "Project")
                        throw std::runtime_error(
                            chemin.string() + ':' + std::to_string(enfant.Ligne)
                            + " : élément de solution inconnu : " + enfant.Nom);
                    VerifierAttributs(enfant, {"Chemin", "Path"}, chemin);
                    if (!enfant.Enfants.empty())
                        throw std::runtime_error(
                            chemin.string() + ':' + std::to_string(enfant.Ligne)
                            + " : un projet de solution ne contient aucun enfant");
                    lignes.push_back({
                        enfant.Nom == "Projet" ? "projet" : "project",
                        AttributRequis(enfant, {"Chemin", "Path"}, chemin),
                        enfant.Ligne});
                }
                return lignes;
            }

            if (racine.Nom != enteteFrancais && racine.Nom != enteteAnglais)
                throw std::runtime_error(
                    chemin.string() + ':' + std::to_string(racine.Ligne)
                    + " : racine GsProjet ou GsProject attendue");
            VerifierAttributs(
                racine,
                {"Version", "Nom", "Name", "Type",
                 "ModeCompilation", "CompilationMode",
                 "PointEntree", "EntryPoint",
                 "VersionApplication", "ApplicationVersion",
                 "Editeur", "Publisher"},
                chemin);
            if (AttributRequis(racine, {"Version"}, chemin) != "1.0")
                throw std::runtime_error(
                    chemin.string() + " : version GsProject 1.0 attendue");

            const auto ajouterAttribut = [&](const char* cle,
                                              std::initializer_list<std::string_view> noms)
            {
                if (const auto valeur = AttributAlias(racine, noms, chemin))
                    lignes.push_back({cle, *valeur, racine.Ligne});
            };
            ajouterAttribut("nom", {"Nom", "Name"});
            ajouterAttribut("type", {"Type"});
            ajouterAttribut(
                "mode_compilation", {"ModeCompilation", "CompilationMode"});
            ajouterAttribut("point_entree", {"PointEntree", "EntryPoint"});
            ajouterAttribut(
                "version_application",
                {"VersionApplication", "ApplicationVersion"});
            ajouterAttribut("editeur", {"Editeur", "Publisher"});

            bool constructionLue = false;
            for (const auto& enfant : racine.Enfants)
            {
                if (!enfant.Enfants.empty())
                    throw std::runtime_error(
                        chemin.string() + ':' + std::to_string(enfant.Ligne)
                        + " : l’élément " + enfant.Nom
                        + " ne contient aucun enfant");
                if (enfant.Nom == "Source")
                {
                    VerifierAttributs(enfant, {"Chemin", "Path"}, chemin);
                    lignes.push_back({
                        "source",
                        AttributRequis(enfant, {"Chemin", "Path"}, chemin),
                        enfant.Ligne});
                }
                else if (enfant.Nom == "Interface")
                {
                    VerifierAttributs(enfant, {"Chemin", "Path"}, chemin);
                    lignes.push_back({
                        "interface",
                        AttributRequis(enfant, {"Chemin", "Path"}, chemin),
                        enfant.Ligne});
                }
                else if (enfant.Nom == "Bibliotheque"
                         || enfant.Nom == "Library")
                {
                    VerifierAttributs(enfant, {"Chemin", "Path"}, chemin);
                    lignes.push_back({
                        "bibliotheque",
                        AttributRequis(enfant, {"Chemin", "Path"}, chemin),
                        enfant.Ligne});
                }
                else if (enfant.Nom == "Construction"
                         || enfant.Nom == "Build")
                {
                    if (constructionLue)
                        throw std::runtime_error(
                            chemin.string() + ':' + std::to_string(enfant.Ligne)
                            + " : élément de construction dupliqué");
                    constructionLue = true;
                    VerifierAttributs(
                        enfant,
                        {"RepertoireObjets", "ObjectDirectory",
                         "Sortie", "Output", "Carte", "Map"},
                        chemin);
                    if (const auto valeur = AttributAlias(
                            enfant,
                            {"RepertoireObjets", "ObjectDirectory"},
                            chemin))
                        lignes.push_back({
                            "repertoire_objets", *valeur, enfant.Ligne});
                    if (const auto valeur = AttributAlias(
                            enfant, {"Sortie", "Output"}, chemin))
                        lignes.push_back({"sortie", *valeur, enfant.Ligne});
                    if (const auto valeur = AttributAlias(
                            enfant, {"Carte", "Map"}, chemin))
                        lignes.push_back({"carte", *valeur, enfant.Ligne});
                }
                else
                    throw std::runtime_error(
                        chemin.string() + ':' + std::to_string(enfant.Ligne)
                        + " : élément de projet inconnu : " + enfant.Nom);
            }
            return lignes;
        }

        std::filesystem::path ResoudreChemin(
            const std::filesystem::path& base,
            const std::string& valeur)
        {
            const std::filesystem::path chemin(valeur);
            return chemin.is_absolute() ? chemin.lexically_normal()
                                        : (base / chemin).lexically_normal();
        }

        void CreerParent(const std::filesystem::path& chemin)
        {
            if (!chemin.parent_path().empty())
                std::filesystem::create_directories(chemin.parent_path());
        }

        ConfigurationProjet ChargerProjet(const std::filesystem::path& chemin)
        {
            const auto base = chemin.parent_path().empty()
                ? std::filesystem::path(".") : chemin.parent_path();
            ConfigurationProjet configuration;
            std::unordered_map<std::string, std::size_t> clesUniques;
            auto valeurUnique = [&](const LigneConfiguration& ligne) -> bool
            {
                const auto [_, insere] = clesUniques.emplace(ligne.Cle, ligne.Numero);
                if (!insere)
                    throw std::runtime_error(
                        chemin.string() + ':' + std::to_string(ligne.Numero)
                        + " : clé déclarée plusieurs fois : " + ligne.Cle);
                return true;
            };

            for (const auto& ligne : LireConfiguration(
                     chemin, "GsProjet", "GsProject"))
            {
                if (ligne.Cle == "source")
                    configuration.Sources.emplace_back(
                        ResoudreChemin(base, ligne.Valeur),
                        std::filesystem::path(ligne.Valeur).generic_string());
                else if (ligne.Cle == "interface" || ligne.Cle == "header")
                    configuration.Interfaces.emplace_back(
                        ResoudreChemin(base, ligne.Valeur),
                        std::filesystem::path(ligne.Valeur).generic_string());
                else if (ligne.Cle == "bibliotheque" || ligne.Cle == "library")
                    configuration.Bibliotheques.push_back(
                        ResoudreChemin(base, ligne.Valeur));
                else if (ligne.Cle == "nom" || ligne.Cle == "name")
                { valeurUnique(ligne); configuration.Nom = ligne.Valeur; }
                else if (ligne.Cle == "type")
                { valeurUnique(ligne); configuration.Type = Minuscule(ligne.Valeur); }
                else if (ligne.Cle == "sortie" || ligne.Cle == "output")
                { valeurUnique(ligne); configuration.Sortie = ResoudreChemin(base, ligne.Valeur); }
                else if (ligne.Cle == "repertoire_objets" || ligne.Cle == "object_directory")
                { valeurUnique(ligne); configuration.RepertoireObjets = ResoudreChemin(base, ligne.Valeur); }
                else if (ligne.Cle == "point_entree" || ligne.Cle == "entry_point")
                { valeurUnique(ligne); configuration.PointEntree = ligne.Valeur; }
                else if (ligne.Cle == "carte" || ligne.Cle == "map")
                { valeurUnique(ligne); configuration.Carte = ResoudreChemin(base, ligne.Valeur); }
                else if (ligne.Cle == "mode_compilation" || ligne.Cle == "compilation_mode")
                {
                    valeurUnique(ligne);
                    const auto mode = Minuscule(ligne.Valeur);
                    if (mode == "agregee" || mode == "agrégée" || mode == "aggregate")
                        configuration.CompilationAgregee = true;
                    else if (mode == "separee" || mode == "séparée" || mode == "separate")
                        configuration.CompilationAgregee = false;
                    else
                        throw std::runtime_error(
                            chemin.string() + ':' + std::to_string(ligne.Numero)
                            + " : mode de compilation inconnu : " + ligne.Valeur);
                }
                else if (ligne.Cle == "version_application" || ligne.Cle == "application_version")
                { valeurUnique(ligne); configuration.Metadonnees.Version = ligne.Valeur; }
                else if (ligne.Cle == "editeur" || ligne.Cle == "publisher")
                { valeurUnique(ligne); configuration.Metadonnees.Editeur = ligne.Valeur; }
                else
                    throw std::runtime_error(
                        chemin.string() + ':' + std::to_string(ligne.Numero)
                        + " : clé de projet inconnue : " + ligne.Cle);
            }
            if (configuration.Sources.empty())
                throw std::runtime_error("le projet ne contient aucune source");
            if (configuration.Nom.empty()) configuration.Nom = chemin.stem().string();
            if (configuration.Type == "bibliothèque") configuration.Type = "bibliotheque";
            if (configuration.Type == "library") configuration.Type = "bibliotheque";
            if (configuration.Type != "executable" && configuration.Type != "bibliotheque")
                throw std::runtime_error("type de projet inconnu : " + configuration.Type);
            if (configuration.RepertoireObjets.empty())
                configuration.RepertoireObjets = base / "construction"
                    / chemin.stem() / "objets";
            if (configuration.Sortie.empty())
                configuration.Sortie = base / "construction" / chemin.stem()
                    / (configuration.Nom
                       + (configuration.Type == "executable" ? ".GsE" : ".GsA"));
            if (EstExtensionObsolete(configuration.Sortie))
                throw std::runtime_error(
                    "extension de sortie de projet obsolète refusée : "
                    + configuration.Sortie.string());
            configuration.Metadonnees.Nom = configuration.Nom;
            return configuration;
        }

        BibliothequeLiaison ChargerBibliotheque(
            const std::filesystem::path& chemin)
        {
            BibliothequeLiaison resultat;
            for (auto& membre : LecteurGsA().Lire(chemin))
                resultat.push_back({
                    chemin.filename().string() + '(' + membre.Nom + ')',
                    LecteurGsO().Lire(membre.Objet)});
            return resultat;
        }

        std::string TrouverPointEntree(
            const std::vector<UniteLiaison>& objets,
            const std::vector<BibliothequeLiaison>& bibliotheques)
        {
            std::string resultat;
            auto examiner = [&](const CodeMachine& machine)
            {
                for (const auto& symbole : machine.Symboles)
                {
                    if (!symbole.EstDefini || !symbole.EstPublic
                        || symbole.Genre != GenreSymboleMachine::Fonction)
                        continue;
                    const auto separateur = symbole.Nom.rfind("::");
                    const auto court = separateur == std::string::npos
                        ? symbole.Nom : symbole.Nom.substr(separateur + 2);
                    if (court != "Principal" && court != "Main") continue;
                    if (!resultat.empty() && resultat != symbole.Nom)
                        throw std::runtime_error(
                            "plusieurs points d’entrée Principal/Main ; indiquez point_entree");
                    resultat = symbole.Nom;
                }
            };
            for (const auto& objet : objets) examiner(objet.Machine);
            for (const auto& bibliotheque : bibliotheques)
                for (const auto& membre : bibliotheque) examiner(membre.Machine);
            if (resultat.empty())
                throw std::runtime_error(
                    "aucun point d’entrée public Principal/Main ; indiquez point_entree");
            return resultat;
        }
    }

    ResultatConstructionProjet ConstructeurProjet::Construire(
        const std::filesystem::path& cheminProjet,
        std::ostream& journal,
        const OptionsConstructionProjet& options) const
    {
        auto configuration = ChargerProjet(cheminProjet);
        if (!options.Sortie.empty())
            configuration.Sortie = options.Sortie;
        if (!options.RepertoireObjets.empty())
            configuration.RepertoireObjets = options.RepertoireObjets;
        std::filesystem::create_directories(configuration.RepertoireObjets);
        CreerParent(configuration.Sortie);
        if (!configuration.Carte.empty()) CreerParent(configuration.Carte);

        std::vector<UniteLiaison> objets;
        std::vector<MembreGsA> membresArchive;
        objets.reserve(configuration.CompilationAgregee
                            ? 1 : configuration.Sources.size());
        membresArchive.reserve(configuration.CompilationAgregee
                                    ? 1 : configuration.Sources.size());

        auto produireObjet = [&](std::vector<UniteSource> unites,
                                 const std::string& nomObjet,
                                 const std::string& diagnostic)
        {
            auto programme = AnalyserUnites(unites);
            auto machine = GenerateurX64().Generer(programme);
            const auto cheminObjet = configuration.RepertoireObjets / nomObjet;
            const auto contenu = EcrivainGsO().Construire(machine);
            EcrivainGsO().Ecrire(machine, cheminObjet);
            journal << "[GsObj] " << diagnostic << " -> "
                    << cheminObjet.string() << '\n';
            objets.push_back({nomObjet, std::move(machine)});
            membresArchive.push_back({nomObjet, contenu});
        };

        if (configuration.CompilationAgregee)
        {
            std::vector<UniteSource> unites;
            unites.reserve(
                configuration.Interfaces.size() + configuration.Sources.size());
            for (const auto& [chemin, diagnostic] : configuration.Interfaces)
                unites.push_back({chemin, true, diagnostic});
            for (const auto& [chemin, diagnostic] : configuration.Sources)
                unites.push_back({chemin, false, diagnostic});
            produireObjet(
                std::move(unites),
                "000-" + cheminProjet.stem().string() + ".GsObj",
                cheminProjet.filename().generic_string());
        }
        else
        {
            for (std::size_t index = 0; index < configuration.Sources.size(); ++index)
            {
                std::vector<UniteSource> unites;
                for (const auto& [chemin, diagnostic] : configuration.Interfaces)
                    unites.push_back({chemin, true, diagnostic});
                const auto& [cheminSource, diagnosticSource] = configuration.Sources[index];
                unites.push_back({cheminSource, false, diagnosticSource});

                std::ostringstream prefixe;
                prefixe << std::setw(3) << std::setfill('0') << index << '-';
                produireObjet(
                    std::move(unites),
                    prefixe.str() + cheminSource.stem().string() + ".GsObj",
                    diagnosticSource);
            }
        }

        if (configuration.Type == "bibliotheque")
        {
            EcrivainGsA().Ecrire(membresArchive, configuration.Sortie);
            journal << "[GsA] " << configuration.Sortie.string() << '\n';
            return {configuration.Sortie, configuration.Sources.size()};
        }

        std::vector<BibliothequeLiaison> bibliotheques;
        for (const auto& chemin : configuration.Bibliotheques)
            bibliotheques.push_back(ChargerBibliotheque(chemin));
        if (configuration.PointEntree.empty())
            configuration.PointEntree = TrouverPointEntree(objets, bibliotheques);
        const auto lie = EditeurLiens().Lier(
            std::move(objets), bibliotheques, configuration.PointEntree);
        EcrivainGsE().Ecrire(
            lie, configuration.PointEntree,
            configuration.Sortie, configuration.Metadonnees);
        if (!configuration.Carte.empty())
            EditeurLiens().EcrireCarte(lie, configuration.Carte);
        journal << "[GsE] " << configuration.Sortie.string() << '\n';
        return {configuration.Sortie, configuration.Sources.size()};
    }

    std::vector<ResultatConstructionProjet> ConstructeurProjet::ConstruireSolution(
        const std::filesystem::path& cheminSolution,
        std::ostream& journal) const
    {
        const auto base = cheminSolution.parent_path().empty()
            ? std::filesystem::path(".") : cheminSolution.parent_path();
        std::vector<ResultatConstructionProjet> resultats;
        for (const auto& ligne : LireConfiguration(
                 cheminSolution, "GsSolution", "GsSolution"))
        {
            if (ligne.Cle != "projet" && ligne.Cle != "project")
                throw std::runtime_error(
                    cheminSolution.string() + ':' + std::to_string(ligne.Numero)
                    + " : seule la clé projet est autorisée dans une solution");
            const auto projet = ResoudreChemin(base, ligne.Valeur);
            journal << "== Projet " << projet.string() << " ==\n";
            resultats.push_back(Construire(projet, journal));
        }
        if (resultats.empty())
            throw std::runtime_error("la solution ne contient aucun projet");
        return resultats;
    }
}
