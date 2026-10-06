#include "GsPP/CatalogueInclusions.hpp"

#include "GsPP/ErreurCompilation.hpp"
#include "GsPP/Lexeur.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace GsPP
{
    namespace
    {
        std::string Identifier(const std::filesystem::path& fichier)
        {
            const auto utf8 = std::filesystem::weakly_canonical(fichier).generic_u8string();
            std::string cle(utf8.begin(), utf8.end());
#if defined(_WIN32)
            std::transform(cle.begin(), cle.end(), cle.begin(), [](unsigned char valeur)
            { return valeur >= 'A' && valeur <= 'Z' ? static_cast<char>(valeur + ('a' - 'A')) : static_cast<char>(valeur); });
#endif
            return cle;
        }

        std::string LireSource(const std::filesystem::path& fichier, std::uint64_t maximum)
        {
            std::ifstream flux(fichier, std::ios::binary);
            if (!flux) throw std::runtime_error("impossible d'ouvrir le fichier du catalogue : " + fichier.string());
            std::string source;
            std::array<char, 8192> bloc{};
            while (flux)
            {
                flux.read(bloc.data(), static_cast<std::streamsize>(bloc.size()));
                const auto taille = static_cast<std::uint64_t>(flux.gcount());
                if (taille > maximum - source.size())
                    throw std::length_error("limite d'octets du catalogue dépassée : " + fichier.string());
                source.append(bloc.data(), static_cast<std::size_t>(taille));
            }
            if (!flux.eof() || flux.bad()) throw std::runtime_error("lecture du catalogue échouée : " + fichier.string());
            return source;
        }

        struct InclusionCandidate
        {
            std::filesystem::path Cible;
            std::uint64_t Debut;
        };

        std::vector<InclusionCandidate> DecouvrirLiens(std::string_view source,
            const UniteSource& fichier, std::uint64_t maximum)
        {
            std::vector<Jeton> jetons;
            try { jetons = Lexeur(source, fichier.NomDiagnostic).Analyser(); }
            catch (const ErreurCompilation&) { return {}; }
            std::vector<InclusionCandidate> inclusions;
            std::size_t position = source.starts_with("\xEF\xBB\xBF") ? 3 : 0;
            std::size_t ligne = 1, colonne = 1;
            for (std::size_t i = 0; i + 1 < jetons.size(); ++i)
            {
                if (jetons[i].Genre != GenreJeton::DirectiveInclure
                    || jetons[i + 1].Genre != GenreJeton::ChaineCaracteres) continue;
                const auto& chemin = jetons[i + 1].Texte;
                if (chemin.empty() || chemin.find_first_of(std::string("\0\r\n", 3)) != std::string::npos) continue;
                if (inclusions.size() >= maximum) throw std::length_error("limite de liens du catalogue dépassée");
                while (position < source.size() && (ligne != jetons[i].Ligne || colonne != jetons[i].Colonne))
                {
                    if (source[position++] == '\n') { ++ligne; colonne = 1; }
                    else ++colonne;
                }
                if (ligne != jetons[i].Ligne || colonne != jetons[i].Colonne)
                    throw std::runtime_error("position de directive absente du catalogue");
                inclusions.push_back({fichier.Chemin.parent_path()
                    / std::filesystem::path(std::u8string(chemin.begin(), chemin.end())), position});
            }
            return inclusions;
        }
    }

    CatalogueInclusions::CatalogueInclusions(CatalogueInclusions&& autre) noexcept
    {
        *this = std::move(autre);
    }

    CatalogueInclusions& CatalogueInclusions::operator=(CatalogueInclusions&& autre) noexcept
    {
        if (this != &autre)
        {
            contenus_ = std::move(autre.contenus_);
            fichiers_ = std::move(autre.fichiers_);
            liens_ = std::move(autre.liens_);
            noms_ = std::move(autre.noms_);
            racine_ = std::exchange(autre.racine_, UINT64_MAX);
            autre.fichiers_.clear(); autre.liens_.clear(); autre.noms_.clear(); autre.contenus_.clear();
        }
        return *this;
    }

    CatalogueInclusions CreerCatalogueInclusions(const UniteSource& racine,
        const std::vector<UniteSource>& fichiersConnus, const LimitesCatalogueInclusions& limites)
    {
        if (!limites.NombreFichiers || limites.NombreFichiers > 1'000'000
            || limites.NombreLiens > 100'000'000 || limites.OctetsParFichier > 1'000'000'000)
            throw std::invalid_argument("limites du catalogue incompatibles avec l'ABI du frontend");
        CatalogueInclusions catalogue;
        std::vector<UniteSource> unites;
        std::vector<unsigned char> etats;
        std::map<std::string, std::uint64_t> noms, identites, actifs;
        auto inscrire = [&](UniteSource fichier)
        {
            if (fichier.Chemin.empty()) throw std::invalid_argument("chemin de catalogue vide");
            if (fichier.NomDiagnostic.empty()) fichier.NomDiagnostic = fichier.Chemin.string();
            if (const auto trouve = noms.find(fichier.NomDiagnostic); trouve != noms.end())
            {
                if (unites[trouve->second].Chemin != fichier.Chemin)
                    throw std::invalid_argument("nom de diagnostic partagé par deux chemins du catalogue");
                return trouve->second;
            }
            if (unites.size() >= limites.NombreFichiers) throw std::length_error("limite de fichiers du catalogue dépassée");
            const auto index = static_cast<std::uint64_t>(unites.size());
            noms.emplace(fichier.NomDiagnostic, index);
            catalogue.noms_.push_back(fichier.NomDiagnostic);
            unites.push_back(std::move(fichier)); etats.push_back(0); catalogue.fichiers_.push_back({});
            return index;
        };
        for (const auto& fichier : fichiersConnus)
        {
            const auto avant = unites.size();
            const auto index = inscrire(fichier);
            if (index != avant)
                throw std::invalid_argument("fichier connu dupliqué dans le catalogue");
        }
        // Le préfixe fourni est conservé même quand la racine porte un nom de diagnostic virtuel.
        catalogue.racine_ = inscrire(racine);
        struct Cadre
        {
            std::uint64_t Index;
            std::string Identite;
            std::vector<InclusionCandidate> Inclusions;
            std::size_t Prochaine = 0;
        };
        std::vector<Cadre> pile;
        std::uint64_t octets = 0, candidats = 0;
        auto entrer = [&](std::uint64_t index)
        {
            const auto& unite = unites[index];
            const auto canonique = Identifier(unite.Chemin);
            auto contenu = catalogue.contenus_.find(canonique);
            if (contenu == catalogue.contenus_.end())
            {
                const auto maximum = std::min(limites.OctetsParFichier, limites.OctetsSources - octets);
                auto source = LireSource(unite.Chemin, maximum);
                octets += source.size();
                contenu = catalogue.contenus_.emplace(canonique, std::move(source)).first;
            }
            const auto identite = identites.try_emplace(canonique, index).first->second;
            const auto& source = contenu->second;
            catalogue.fichiers_[index] = {source.data(), source.size(), identite, EstExtensionInterface(unite.Chemin) ? 1U : 0U, 0};
            auto inclusions = DecouvrirLiens(source, unite, limites.NombreLiens - candidats);
            candidats += inclusions.size();
            etats[index] = 1; actifs.emplace(canonique, index);
            pile.push_back({index, canonique, std::move(inclusions), 0});
        };
        auto parcourir = [&](std::uint64_t racineIndex)
        {
            if (etats[racineIndex] == 2) return;
            entrer(racineIndex);
            while (!pile.empty())
            {
                auto& cadre = pile.back();
                if (cadre.Prochaine == cadre.Inclusions.size())
                {
                    etats[cadre.Index] = 2; actifs.erase(cadre.Identite); pile.pop_back(); continue;
                }
                const auto inclusion = cadre.Inclusions[cadre.Prochaine++];
                const auto parent = cadre.Index;
                std::uint32_t etat = 0;
                std::uint64_t cible = UINT64_MAX;
                bool charger = false;
                if (!std::filesystem::is_regular_file(inclusion.Cible)) etat = 1;
                else if (EstExtensionGsSharp(inclusion.Cible) || EstExtensionObsolete(inclusion.Cible)) etat = 2;
                else
                {
                    const auto canonique = Identifier(inclusion.Cible);
                    if (const auto actif = actifs.find(canonique); actif != actifs.end()) cible = actif->second;
                    else
                    {
                        cible = inscrire({inclusion.Cible, false, {}});
                        charger = etats[cible] != 2;
                    }
                }
                catalogue.liens_.push_back({parent, inclusion.Debut, cible, etat, 0});
                if (charger) entrer(cible);
            }
        };
        parcourir(catalogue.racine_);
        for (std::uint64_t i = 0; i < unites.size(); ++i) parcourir(i);
        for (auto& lien : catalogue.liens_) if (lien.Etat != 0) lien.IndexCible = catalogue.fichiers_.size();
        std::sort(catalogue.liens_.begin(), catalogue.liens_.end(), [](const auto& a, const auto& b)
        { return std::tie(a.IndexFichier, a.DebutDirective) < std::tie(b.IndexFichier, b.DebutDirective); });
        return catalogue;
    }

    SourcePrepareeAvecOrigines PreparerSourceAvecOrigines(const CatalogueInclusions& catalogue,
        ExpanseurInclusionsHote developper, bool marqueUtf8, bool finLigneCrlf,
        std::uint64_t maximumOctets, std::uint64_t maximumOrigines)
    {
        if (!developper || catalogue.IndexRacine() >= catalogue.Fichiers().size())
            throw std::invalid_argument("expanseur ou catalogue absent");
        RequeteExpansionDeclarationsHote requete{catalogue.Fichiers().data(), catalogue.Fichiers().size(),
            catalogue.Liens().data(), catalogue.Liens().size(), catalogue.IndexRacine(), nullptr, 0, nullptr, 0,
            marqueUtf8 ? 1U : 0U, finLigneCrlf ? 1U : 0U, {}};
        const auto mesure = developper(&requete);
        if (mesure != requete.Resultat.Erreur || mesure == 0 || mesure > 14)
            throw std::runtime_error("contrat de mesure du frontend incohérent");
        SourcePrepareeAvecOrigines resultat;
        resultat.Resultat = requete.Resultat;
        if (mesure != 1) return resultat;
        if (requete.Resultat.NombreOctetsSource > 1'000'000'000 || requete.Resultat.NombreOrigines > 100'000'000
            || requete.Resultat.NombreOctetsSource > resultat.Source.max_size()
            || requete.Resultat.NombreOrigines > resultat.Origines.max_size())
            throw std::length_error("capacités de préparation incompatibles avec l'ABI du frontend");
        if (requete.Resultat.NombreOctetsSource > maximumOctets || requete.Resultat.NombreOrigines > maximumOrigines
            || !requete.Resultat.NombreOrigines || requete.Resultat.IndexFichierErreur != requete.NombreFichiers
            || requete.Resultat.LigneErreur || requete.Resultat.ColonneErreur)
            throw std::length_error("capacités de préparation excessives ou mesure incohérente");
        resultat.Source.resize(static_cast<std::size_t>(requete.Resultat.NombreOctetsSource));
        resultat.Origines.resize(static_cast<std::size_t>(requete.Resultat.NombreOrigines));
        requete.SourcePreparee = resultat.Source.data(); requete.CapaciteSource = resultat.Source.size();
        requete.Origines = resultat.Origines.data(); requete.CapaciteOrigines = resultat.Origines.size();
        const auto code = developper(&requete);
        if (code != requete.Resultat.Erreur || code == 1 || code > 14
            || (code == 0 && (requete.Resultat.NombreOctetsSource != requete.CapaciteSource
                || requete.Resultat.NombreOrigines != requete.CapaciteOrigines
                || requete.Resultat.IndexFichierErreur != requete.NombreFichiers
                || requete.Resultat.LigneErreur || requete.Resultat.ColonneErreur)))
            throw std::runtime_error("contrat de publication du frontend incohérent");
        resultat.Resultat = requete.Resultat;
        if (code != 0) { resultat.Source.clear(); resultat.Origines.clear(); }
        return resultat;
    }
}
