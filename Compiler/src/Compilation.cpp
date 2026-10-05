#include "GsPP/Compilation.hpp"

#include "GsPP/AnalyseurSemantique.hpp"
#include "GsPP/AnalyseurSyntaxique.hpp"
#include "GsPP/ErreurCompilation.hpp"
#include "GsPP/Lexeur.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace GsPP
{
    namespace
    {
        std::string LireFichier(const std::filesystem::path& chemin)
        {
            std::ifstream flux(chemin, std::ios::binary);
            if (!flux)
                throw std::runtime_error(
                    "impossible d’ouvrir le fichier source : " + chemin.string());
            std::ostringstream contenu;
            contenu << flux.rdbuf();
            return contenu.str();
        }

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

        void AjouterProgramme(Programme& destination, Programme&& source)
        {
            destination.Structures.insert(
                destination.Structures.end(),
                std::make_move_iterator(source.Structures.begin()),
                std::make_move_iterator(source.Structures.end()));
            destination.Enumerations.insert(
                destination.Enumerations.end(),
                std::make_move_iterator(source.Enumerations.begin()),
                std::make_move_iterator(source.Enumerations.end()));
            destination.VariablesGlobales.insert(
                destination.VariablesGlobales.end(),
                std::make_move_iterator(source.VariablesGlobales.begin()),
                std::make_move_iterator(source.VariablesGlobales.end()));
            destination.Fonctions.insert(
                destination.Fonctions.end(),
                std::make_move_iterator(source.Fonctions.begin()),
                std::make_move_iterator(source.Fonctions.end()));
            destination.Aliases.insert(
                destination.Aliases.end(),
                std::make_move_iterator(source.Aliases.begin()),
                std::make_move_iterator(source.Aliases.end()));
            destination.Utilisations.insert(destination.Utilisations.end(),
                std::make_move_iterator(source.Utilisations.begin()),
                std::make_move_iterator(source.Utilisations.end()));
            destination.EspacesNoms.insert(destination.EspacesNoms.end(),
                std::make_move_iterator(source.EspacesNoms.begin()),
                std::make_move_iterator(source.EspacesNoms.end()));
        }

        bool PrototypesCompatibles(const Fonction& gauche, const Fonction& droite)
        {
            if (!(gauche.TypeRetour == droite.TypeRetour)
                || gauche.Parametres.size() != droite.Parametres.size())
                return false;
            for (std::size_t index = 0; index < gauche.Parametres.size(); ++index)
                if (!(gauche.Parametres[index].Type == droite.Parametres[index].Type))
                    return false;
            return true;
        }

        std::string CleSurcharge(const Fonction& fonction)
        {
            std::string cle = fonction.NomSourceComplet() + '(';
            for (const auto& parametre : fonction.Parametres)
            {
                cle += parametre.Type.Afficher();
                cle.push_back(';');
            }
            cle.push_back(')');
            return cle;
        }

        [[noreturn]] void ErreurDeclaration(
            const PositionSource& position,
            std::string francais,
            std::string anglais)
        {
            throw ErreurCompilation(
                std::move(francais), std::move(anglais),
                position.Ligne, position.Colonne, position.Fichier);
        }
    }

    bool EstExtensionSource(const std::filesystem::path& chemin)
    {
        const auto extension = ExtensionMinuscule(chemin);
        return extension == ".gs++" || extension == ".gspp"
            || extension == ".gsplusplus";
    }

    bool EstExtensionInterface(const std::filesystem::path& chemin)
    {
        const auto extension = ExtensionMinuscule(chemin);
        return extension == ".hgs++" || extension == ".hgspp"
            || extension == ".headergsplusplus";
    }

    bool EstExtensionGsSharp(const std::filesystem::path& chemin)
    {
        const auto extension = ExtensionMinuscule(chemin);
        return extension == ".gs#" || extension == ".gss"
            || extension == ".gssharp";
    }

    bool EstExtensionObsolete(const std::filesystem::path& chemin)
    {
        const auto extension = ExtensionMinuscule(chemin);
        return extension == ".gsph" || extension == ".gso"
            || extension == ".gspph"
            || extension == ".gsplusplusheader";
    }

    void NormaliserDeclarations(Programme& programme)
    {
        std::vector<Fonction> fonctions;
        std::unordered_map<std::string, std::size_t> indicesFonctions;
        for (auto& fonction : programme.Fonctions)
        {
            const auto nom = fonction.NomSourceComplet();
            const auto cle = CleSurcharge(fonction);
            const auto [trouve, insere] = indicesFonctions.emplace(cle, fonctions.size());
            if (insere)
            {
                fonctions.push_back(std::move(fonction));
                continue;
            }

            auto& precedente = fonctions[trouve->second];
            if (!PrototypesCompatibles(precedente, fonction))
                ErreurDeclaration(
                    fonction.Position,
                    "déclarations incompatibles pour la fonction " + nom,
                    "incompatible declarations for function " + nom);
            if (!precedente.EstExterne && !fonction.EstExterne)
                ErreurDeclaration(
                    fonction.Position,
                    "fonction définie plusieurs fois : " + nom,
                    "function defined more than once: " + nom);
            if (precedente.EstExterne && !fonction.EstExterne)
                precedente = std::move(fonction);
        }
        programme.Fonctions = std::move(fonctions);

        std::vector<VariableGlobale> globales;
        std::unordered_map<std::string, std::size_t> indicesGlobales;
        for (auto& globale : programme.VariablesGlobales)
        {
            const auto nom = globale.NomComplet();
            const auto [trouve, insere] = indicesGlobales.emplace(nom, globales.size());
            if (insere)
            {
                globales.push_back(std::move(globale));
                continue;
            }

            auto& precedente = globales[trouve->second];
            if (!(precedente.Type == globale.Type))
                ErreurDeclaration(
                    globale.Position,
                    "déclarations incompatibles pour la globale " + nom,
                    "incompatible declarations for global " + nom);
            if (!precedente.EstExterne && !globale.EstExterne)
                ErreurDeclaration(
                    globale.Position,
                    "variable globale définie plusieurs fois : " + nom,
                    "global variable defined more than once: " + nom);
            if (precedente.EstExterne && !globale.EstExterne)
                precedente = std::move(globale);
        }
        programme.VariablesGlobales = std::move(globales);

        std::vector<DeclarationAlias> aliases;
        std::unordered_map<std::string, std::size_t> indicesAliases;
        for (auto& alias : programme.Aliases)
        {
            const auto nom = alias.NomComplet();
            const auto [trouve, insere] = indicesAliases.emplace(nom, aliases.size());
            if (insere)
            {
                aliases.push_back(std::move(alias));
                continue;
            }
            if (aliases[trouve->second].Cible != alias.Cible)
                ErreurDeclaration(
                    alias.Position,
                    "déclarations incompatibles pour l’alias " + nom,
                    "incompatible declarations for alias " + nom);
        }
        programme.Aliases = std::move(aliases);
    }

    Programme AnalyserUnites(
        const std::vector<UniteSource>& unites,
        std::ostream* sortieJetons)
    {
        if (unites.empty())
            throw std::runtime_error("aucune unité source indiquée");

        Programme programme;
        for (const auto& unite : unites)
        {
            if (EstExtensionObsolete(unite.Chemin))
                throw std::runtime_error(
                    "extension obsolète refusée : " + unite.Chemin.string()
                    + " ; utilisez .HGs++, .HGsPP ou .HeaderGsPlusPlus pour "
                      "une interface Gs++, et .GsObj pour un objet natif");
            if (EstExtensionGsSharp(unite.Chemin))
                throw std::runtime_error(
                    "extension réservée à Gs# : " + unite.Chemin.string()
                    + " ; cette unité doit être confiée au compilateur Gs#");
            const bool extensionInterface = EstExtensionInterface(unite.Chemin);
            if (!EstExtensionSource(unite.Chemin) && !extensionInterface)
                throw std::runtime_error(
                    "extension d’unité Gs++ inconnue : " + unite.Chemin.string());
            if (unite.EstInterface && !extensionInterface)
                throw std::runtime_error(
                    "une interface Gs++ doit utiliser .HGs++, .HGsPP ou "
                    ".HeaderGsPlusPlus : " + unite.Chemin.string());
            const auto nomDiagnostic = unite.NomDiagnostic.empty()
                ? unite.Chemin.string() : unite.NomDiagnostic;
            auto jetons = PreparerJetonsSource(unite.Chemin, nomDiagnostic);
            if (sortieJetons)
            {
                *sortieJetons << "== " << nomDiagnostic << " ==\n";
                for (const auto& jeton : jetons)
                    *sortieJetons << jeton.Ligne << ':' << jeton.Colonne << ' '
                                  << NomGenreJeton(jeton.Genre) << "  "
                                  << jeton.Texte << '\n';
            }
            AjouterProgramme(
                programme,
                AnalyseurSyntaxique(
                    std::move(jetons),
                    nomDiagnostic,
                    unite.EstInterface || extensionInterface).Analyser());
        }

        NormaliserDeclarations(programme);
        AnalyseurSemantique().Analyser(programme);
        return programme;
    }

    /**
     * <résumé>Insère les jetons des fichiers inclus, sans perdre leur fichier ni leur position.</résumé>
     * @etc. Les protections once sont propres à une unité ; aucune inclusion n'est dédupliquée implicitement.
     **/
    std::vector<Jeton> PreparerJetonsSource(
        const std::filesystem::path& chemin, const std::string& nomDiagnostic)
    {
        std::unordered_set<std::string> uneFois;
        std::unordered_set<std::string> actifs;
        std::vector<Jeton> resultat;
        Jeton finPrincipale{GenreJeton::Fin, "", 1, 1};
        const auto identifier = [](const std::filesystem::path& fichier)
        {
            const auto utf8 = std::filesystem::weakly_canonical(fichier).generic_u8string();
            std::string cle(utf8.begin(), utf8.end());
#if defined(_WIN32)
            std::transform(cle.begin(), cle.end(), cle.begin(), [](unsigned char valeur)
            {
                return valeur >= 'A' && valeur <= 'Z' ? static_cast<char>(valeur + ('a' - 'A'))
                    : static_cast<char>(valeur);
            });
#endif
            return cle;
        };
        std::function<void(const std::filesystem::path&, const std::string&)> inclure;
        inclure = [&](const std::filesystem::path& fichier, const std::string& diagnostic)
        {
            const auto canonique = identifier(fichier);
            if (uneFois.contains(canonique)) return;
            if (actifs.size() >= 128 || !actifs.insert(canonique).second)
                throw ErreurCompilation("cycle ou profondeur excessive d'inclusion : " + fichier.string(),
                    "include cycle or excessive depth: " + fichier.string(), 1, 1, diagnostic);
            auto jetons = Lexeur(LireFichier(fichier), diagnostic).Analyser();
            if (fichier == chemin) { finPrincipale = jetons.back(); finPrincipale.Fichier = diagnostic; }
            for (std::size_t index = 0; index + 1 < jetons.size(); ++index)
            {
                auto& jeton = jetons[index];
                jeton.Fichier = diagnostic;
                jeton.EstInterface = EstExtensionInterface(fichier);
                if (jeton.Genre != GenreJeton::DirectiveInclure
                    && jeton.Genre != GenreJeton::DirectivePragma)
                {
                    resultat.push_back(std::move(jeton));
                    continue;
                }
                const auto erreur = [&](const char* fr, const char* en)
                {
                    throw ErreurCompilation(fr, en, jeton.Ligne, jeton.Colonne, diagnostic);
                };
                // Comme en C++, une directive commence une ligne logique.
                if (index != 0 && jetons[index - 1].Ligne == jeton.Ligne)
                    erreur("une directive doit commencer une ligne", "a directive must start a line");
                const auto& argument = jetons[++index];
                if (argument.Genre == GenreJeton::Fin || argument.Ligne != jeton.Ligne)
                    erreur("argument de directive attendu", "expected directive argument");
                if (index + 1 < jetons.size() && jetons[index + 1].Genre != GenreJeton::Fin
                    && jetons[index + 1].Ligne == jeton.Ligne)
                    erreur("texte inattendu après la directive", "unexpected text after directive");
                if (jeton.Genre == GenreJeton::DirectivePragma)
                {
                    if (argument.Genre != GenreJeton::Identifiant || argument.Texte != "once")
                        erreur("seul '#pragma once' est pris en charge", "only '#pragma once' is supported");
                    uneFois.insert(canonique);
                    continue;
                }
                if (argument.Genre != GenreJeton::ChaineCaracteres || argument.Texte.empty()
                    || argument.Texte.find_first_of(std::string("\0\r\n", 3)) != std::string::npos)
                    erreur("chemin d'inclusion entre guillemets attendu", "expected quoted include path");
                const std::u8string cheminUtf8(argument.Texte.begin(), argument.Texte.end());
                const auto cible = fichier.parent_path() / std::filesystem::path(cheminUtf8);
                if (!std::filesystem::is_regular_file(cible))
                    erreur("fichier inclus introuvable", "included file not found");
                if (EstExtensionGsSharp(cible) || EstExtensionObsolete(cible))
                    erreur("extension d'inclusion incompatible avec Gs++", "include extension is incompatible with Gs++");
                const auto canoniqueCible = identifier(cible);
                if (actifs.contains(canoniqueCible) && !uneFois.contains(canoniqueCible))
                    erreur("cycle d'inclusion détecté", "include cycle detected");
                inclure(cible, cible.string());
            }
            actifs.erase(canonique);
        };
        const auto diagnostic = nomDiagnostic.empty() ? chemin.string() : nomDiagnostic;
        inclure(chemin, diagnostic);
        resultat.push_back(std::move(finPrincipale));
        return resultat;
    }
}
