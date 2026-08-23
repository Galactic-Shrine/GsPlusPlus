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
            const auto texte = LireFichier(unite.Chemin);
            const auto nomDiagnostic = unite.NomDiagnostic.empty()
                ? unite.Chemin.string() : unite.NomDiagnostic;
            auto jetons = Lexeur(texte, nomDiagnostic).Analyser();
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
}
