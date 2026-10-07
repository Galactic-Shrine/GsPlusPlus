#include "GsPP/CatalogueInclusions.hpp"

#include "GsPP/ErreurCompilation.hpp"
#include "GsPP/FichiersSource.hpp"
#include "GsPP/Lexeur.hpp"

#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace GsPP
{
    namespace
    {
        struct InclusionCandidate
        {
            std::filesystem::path Cible;
            std::uint64_t Debut;
        };

        std::string DecoderCheminDemande(std::string_view brut)
        {
            if (brut.size() < 3 || brut.front() != '"' || brut.back() != '"')
                throw std::runtime_error("argument de résolution incohérent");
            std::string chemin;
            for (std::size_t i = 1; i + 1 < brut.size(); ++i)
            {
                char valeur = brut[i];
                if (valeur == '\\')
                {
                    if (++i + 1 >= brut.size()) throw std::runtime_error("échappement de résolution incomplet");
                    valeur = brut[i];
                    if (valeur == 't') valeur = '\t';
                    else if (valeur != '\\' && valeur != '"') throw std::runtime_error("échappement de résolution incohérent");
                }
                else if (valeur == '"') throw std::runtime_error("chaîne de résolution incohérente");
                if (!valeur || valeur == '\r' || valeur == '\n') throw std::runtime_error("chemin de résolution invalide");
                chemin.push_back(valeur);
            }
            if (chemin.empty()) throw std::runtime_error("chemin de résolution vide");
            return chemin;
        }

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
            chemins_ = std::move(autre.chemins_);
            disponibles_ = std::move(autre.disponibles_);
            racine_ = std::exchange(autre.racine_, UINT64_MAX);
            autre.fichiers_.clear(); autre.liens_.clear(); autre.noms_.clear(); autre.contenus_.clear(); autre.disponibles_.clear();
            autre.chemins_.clear();
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
            catalogue.chemins_.push_back(fichier.Chemin);
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
            const auto canonique = IdentifierFichierSource(unite.Chemin);
            auto contenu = catalogue.contenus_.find(canonique);
            if (contenu == catalogue.contenus_.end())
            {
                const auto maximum = std::min(limites.OctetsParFichier, limites.OctetsSources - octets);
                auto source = LireFichierSource(unite.Chemin, maximum);
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
                if (!EstFichierSourceRegulier(inclusion.Cible)) etat = 1;
                else if (EstExtensionGsSharp(inclusion.Cible) || EstExtensionObsolete(inclusion.Cible)) etat = 2;
                else
                {
                    const auto canonique = IdentifierFichierSource(inclusion.Cible);
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
        catalogue.disponibles_.assign(catalogue.fichiers_.size(), 1);
        return catalogue;
    }

    SourcePrepareeAvecOrigines PreparerSourceAvecOrigines(const CatalogueInclusions& catalogue,
        ExpanseurInclusionsHote developper, bool marqueUtf8, bool finLigneCrlf,
        std::uint64_t maximumOctets, std::uint64_t maximumOrigines)
    {
        if (!developper || catalogue.IndexRacine() >= catalogue.Fichiers().size()
            || std::find(catalogue.Disponibles().begin(), catalogue.Disponibles().end(), 0U) != catalogue.Disponibles().end())
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

    class AdaptateurExpansionFichier
    {
    public:
        template<class Expanseur>
        static ResultatPreparationFichier Executer(const UniteSource& racine,
            Expanseur developper, const std::vector<UniteSource>& fichiersConnus,
            const LimitesCatalogueInclusions& limites, bool marqueUtf8, bool finLigneCrlf,
            std::uint64_t maximumOctets, std::uint64_t maximumOrigines)
        {
            if (!limites.NombreFichiers || limites.NombreFichiers > 1'000'000
                || limites.NombreLiens > 100'000'000 || limites.OctetsParFichier > 1'000'000'000)
                throw std::invalid_argument("expanseur ou limites à la demande invalides");
            ResultatPreparationFichier resultat;
            auto& catalogue = resultat.Catalogue;
            std::vector<UniteSource> unites;
            std::vector<std::string> canoniques;
            std::map<std::string, std::uint64_t> noms, identites;
            std::uint64_t octets = 0;
            auto inscrire = [&](UniteSource unite)
            {
                if (unite.Chemin.empty()) throw std::invalid_argument("chemin de catalogue vide");
                if (unite.NomDiagnostic.empty()) unite.NomDiagnostic = unite.Chemin.string();
                if (const auto trouve = noms.find(unite.NomDiagnostic); trouve != noms.end())
                {
                    if (unites[trouve->second].Chemin != unite.Chemin)
                        throw std::invalid_argument("nom de diagnostic partagé par deux chemins du catalogue");
                    return trouve->second;
                }
                if (unites.size() >= limites.NombreFichiers) throw std::length_error("limite de fichiers du catalogue dépassée");
                const auto index = static_cast<std::uint64_t>(unites.size());
                noms.emplace(unite.NomDiagnostic, index); catalogue.noms_.push_back(unite.NomDiagnostic);
                catalogue.chemins_.push_back(unite.Chemin);
                catalogue.fichiers_.push_back({nullptr, 0, index, EstExtensionInterface(unite.Chemin) ? 1U : 0U, 0});
                catalogue.disponibles_.push_back(0); unites.push_back(std::move(unite)); canoniques.emplace_back();
                return index;
            };
            auto identifier = [&](std::uint64_t index)
            {
                if (canoniques[index].empty()) canoniques[index] = IdentifierFichierSource(unites[index].Chemin);
                catalogue.fichiers_[index].Identite = identites.try_emplace(canoniques[index], index).first->second;
            };
            for (const auto& fichier : fichiersConnus)
            {
                const auto avant = unites.size();
                if (inscrire(fichier) != avant) throw std::invalid_argument("fichier connu dupliqué dans le catalogue");
            }
            catalogue.racine_ = inscrire(racine); identifier(catalogue.racine_);
            auto executer = [&](bool publier)
            {
                RequeteExpansionDeclarationsHote requete{catalogue.fichiers_.data(), catalogue.fichiers_.size(),
                    catalogue.liens_.data(), catalogue.liens_.size(), catalogue.racine_, nullptr, 0, nullptr, 0,
                    marqueUtf8 ? 1U : 0U, finLigneCrlf ? 1U : 0U, {}};
                if (publier)
                {
                    requete.SourcePreparee = resultat.Preparation.Source.data(); requete.CapaciteSource = resultat.Preparation.Source.size();
                    requete.Origines = resultat.Preparation.Origines.data(); requete.CapaciteOrigines = resultat.Preparation.Origines.size();
                }
                RequeteExpansionDeclarationsADemandeHote demande{&requete, catalogue.disponibles_.data(), {}};
                ++resultat.NombreAppels;
                const auto code = developper(&demande);
                if (code != requete.Resultat.Erreur || code > 16)
                    throw std::runtime_error("contrat d'expansion à la demande incohérent");
                resultat.Preparation.Resultat = requete.Resultat;
                return std::pair{code, demande.Demande};
            };
            while (true)
            {
                const auto [code, demande] = executer(false);
                if (code != 15 && code != 16)
                {
                    if (demande.Operation || demande.Reserve || demande.IndexFichier || demande.DebutDirective || demande.DebutArgument || demande.TailleArgument)
                        throw std::runtime_error("demande résiduelle hors suspension");
                    if (code == 0) throw std::runtime_error("mesure à la demande publiée sans capacité");
                    if (code != 1) return resultat;
                    break;
                }
                const auto& mesure = resultat.Preparation.Resultat;
                if (demande.Reserve || demande.IndexFichier >= unites.size() || mesure.NombreOctetsSource || mesure.NombreOrigines
                    || mesure.IndexFichierErreur != unites.size() || mesure.LigneErreur || mesure.ColonneErreur)
                    throw std::runtime_error("suspension d'expansion incohérente");
                const auto index = demande.IndexFichier;
                if (code == 16)
                {
                    if (demande.Operation != 2 || demande.DebutDirective || demande.DebutArgument || demande.TailleArgument
                        || catalogue.disponibles_[index]) throw std::runtime_error("demande de lecture incohérente ou répétée");
                    identifier(index);
                    auto contenu = catalogue.contenus_.find(canoniques[index]);
                    if (contenu == catalogue.contenus_.end())
                    {
                        const auto maximum = std::min(limites.OctetsParFichier, limites.OctetsSources - octets);
                        auto source = LireFichierSource(unites[index].Chemin, maximum);
                        octets += source.size(); ++resultat.NombreLectures;
                        contenu = catalogue.contenus_.emplace(canoniques[index], std::move(source)).first;
                    }
                    catalogue.fichiers_[index].Source = contenu->second.data(); catalogue.fichiers_[index].Taille = contenu->second.size();
                    catalogue.disponibles_[index] = 1;
                }
                else
                {
                    const auto& fichier = catalogue.fichiers_[index];
                    if (demande.Operation != 1 || !catalogue.disponibles_[index] || demande.DebutDirective >= fichier.Taille
                        || demande.DebutArgument <= demande.DebutDirective || demande.DebutArgument > fichier.Taille
                        || demande.TailleArgument > fichier.Taille - demande.DebutArgument || fichier.Source[demande.DebutDirective] != '#')
                        throw std::runtime_error("demande de résolution incohérente");
                    const auto cle = std::pair{index, demande.DebutDirective};
                    const auto position = std::lower_bound(catalogue.liens_.begin(), catalogue.liens_.end(), cle,
                        [](const auto& lien, const auto& recherche) { return std::pair{lien.IndexFichier, lien.DebutDirective} < recherche; });
                    if (position != catalogue.liens_.end() && position->IndexFichier == index && position->DebutDirective == demande.DebutDirective)
                        throw std::runtime_error("demande de résolution répétée");
                    if (catalogue.liens_.size() >= limites.NombreLiens) throw std::length_error("limite de liens du catalogue dépassée");
                    const auto chemin = DecoderCheminDemande(std::string_view(fichier.Source + demande.DebutArgument, static_cast<std::size_t>(demande.TailleArgument)));
                    const auto cible = unites[index].Chemin.parent_path() / std::filesystem::path(std::u8string(chemin.begin(), chemin.end()));
                    std::uint32_t etat = 0;
                    std::uint64_t indexCible = UINT64_MAX;
                    if (!EstFichierSourceRegulier(cible)) etat = 1;
                    else if (EstExtensionGsSharp(cible) || EstExtensionObsolete(cible)) etat = 2;
                    else { indexCible = inscrire({cible, false, {}}); identifier(indexCible); }
                    catalogue.liens_.insert(position, {index, demande.DebutDirective, indexCible, etat, 0});
                    for (auto& lien : catalogue.liens_) if (lien.Etat != 0) lien.IndexCible = catalogue.fichiers_.size();
                    ++resultat.NombreResolutions;
                }
            }
            const auto mesure = resultat.Preparation.Resultat;
            if (mesure.NombreOctetsSource > maximumOctets || mesure.NombreOrigines > maximumOrigines
                || mesure.NombreOctetsSource > 1'000'000'000 || mesure.NombreOrigines > 100'000'000 || !mesure.NombreOrigines
                || mesure.NombreOctetsSource > resultat.Preparation.Source.max_size()
                || mesure.NombreOrigines > resultat.Preparation.Origines.max_size()
                || mesure.IndexFichierErreur != catalogue.fichiers_.size() || mesure.LigneErreur || mesure.ColonneErreur)
                throw std::length_error("capacités de préparation à la demande excessives ou incohérentes");
            resultat.Preparation.Source.resize(static_cast<std::size_t>(mesure.NombreOctetsSource));
            resultat.Preparation.Origines.resize(static_cast<std::size_t>(mesure.NombreOrigines));
            const auto [code, demande] = executer(true);
            const auto& publication = resultat.Preparation.Resultat;
            if (code == 1 || code >= 15 || demande.Operation
                || (code == 0 && (publication.NombreOctetsSource != mesure.NombreOctetsSource || publication.NombreOrigines != mesure.NombreOrigines
                    || publication.IndexFichierErreur != catalogue.fichiers_.size() || publication.LigneErreur || publication.ColonneErreur)))
                throw std::runtime_error("contrat de publication à la demande incohérent");
            if (code != 0) { resultat.Preparation.Source.clear(); resultat.Preparation.Origines.clear(); }
            return resultat;
        }
    };

    ResultatPreparationFichier PreparerFichierAvecOrigines(const UniteSource& racine,
        ExpanseurInclusionsADemandeHote developper, const std::vector<UniteSource>& fichiersConnus,
        const LimitesCatalogueInclusions& limites, bool marqueUtf8, bool finLigneCrlf,
        std::uint64_t maximumOctets, std::uint64_t maximumOrigines)
    {
        if (!developper) throw std::invalid_argument("expanseur à la demande nul");
        return AdaptateurExpansionFichier::Executer(racine, developper, fichiersConnus, limites,
            marqueUtf8, finLigneCrlf, maximumOctets, maximumOrigines);
    }

    ResultatPreparationFichier PreparerFichierAvecOriginesEnSession(const UniteSource& racine,
        ExpanseurInclusionsEnSessionHote developper, const std::vector<UniteSource>& fichiersConnus,
        const LimitesCatalogueInclusions& limites, bool marqueUtf8, bool finLigneCrlf,
        std::uint64_t maximumOctets, std::uint64_t maximumOrigines)
    {
        if (!developper) throw std::invalid_argument("expanseur en session nul");
        struct Session
        {
            ExpanseurInclusionsEnSessionHote Developper;
            RequeteExpansionDeclarationsEnSessionHote Requete{};
            ~Session() { Requete.Expansion = nullptr; Requete.Operation = 1; Developper(&Requete); }
        } session{developper, {}};
        auto resultat = AdaptateurExpansionFichier::Executer(racine, [&](RequeteExpansionDeclarationsADemandeHote* demande)
        {
            session.Requete.Expansion = demande;
            return developper(&session.Requete);
        }, fichiersConnus, limites, marqueUtf8, finLigneCrlf, maximumOctets, maximumOrigines);
        resultat.NombreLexages = session.Requete.NombreLexages;
        resultat.NombreReutilisations = session.Requete.NombreReutilisations;
        return resultat;
    }

    ResultatPreparationFichier PreparerFichierAvecReprise(const UniteSource& racine,
        ExpanseurInclusionsAvecRepriseHote developper, const std::vector<UniteSource>& fichiersConnus,
        const LimitesCatalogueInclusions& limites, bool marqueUtf8, bool finLigneCrlf,
        std::uint64_t maximumOctets, std::uint64_t maximumOrigines)
    {
        if (!developper) throw std::invalid_argument("expanseur avec reprise nul");
        struct Reprise
        {
            ExpanseurInclusionsAvecRepriseHote Developper;
            RequeteExpansionDeclarationsAvecRepriseHote Requete{};
            ~Reprise() { Requete.Expansion = nullptr; Requete.Operation = 1; Developper(&Requete); }
        } reprise{developper, {}};
        auto resultat = AdaptateurExpansionFichier::Executer(racine, [&](RequeteExpansionDeclarationsADemandeHote* demande)
        {
            reprise.Requete.Expansion = demande;
            return developper(&reprise.Requete);
        }, fichiersConnus, limites, marqueUtf8, finLigneCrlf, maximumOctets, maximumOrigines);
        resultat.NombreLexages = reprise.Requete.NombreLexages;
        resultat.NombreReutilisations = reprise.Requete.NombreReutilisations;
        resultat.NombreJetonsParcourus = reprise.Requete.NombreJetonsParcourus;
        return resultat;
    }

    std::vector<Jeton> ConvertirPreparationEnJetons(const ResultatPreparationFichier& resultat)
    {
        const auto& preparation = resultat.Preparation;
        const auto& diagnostic = preparation.Resultat;
        const auto& fichiers = resultat.Catalogue.Fichiers();
        const auto& noms = resultat.Catalogue.NomsFichiers();
        const auto& chemins = resultat.Catalogue.CheminsFichiers();
        const auto& disponibles = resultat.Catalogue.Disponibles();
        const auto racine = resultat.Catalogue.IndexRacine();
        const auto incoherent = []() -> void
        { throw std::runtime_error("sorties de l'expanseur incompatibles avec le pipeline de jetons"); };
        if (fichiers.size() != noms.size() || fichiers.size() != chemins.size()
            || disponibles.size() != fichiers.size() || racine >= fichiers.size()) incoherent();
        if (diagnostic.Erreur)
        {
            if (!preparation.Source.empty() || !preparation.Origines.empty()) incoherent();
            if (diagnostic.Erreur == 3) throw std::bad_alloc();
            if (diagnostic.Erreur == 14) throw std::length_error("limite de taille de l'expansion Gs++ dépassée");
            if (diagnostic.IndexFichierErreur >= noms.size() || !diagnostic.LigneErreur || !diagnostic.ColonneErreur)
                incoherent();
            const auto index = diagnostic.IndexFichierErreur;
            if (diagnostic.Erreur == 4)
            {
                if (!disponibles[index] || diagnostic.DetailLexical < 2 || diagnostic.DetailLexical > 7) incoherent();
                const auto& fichier = fichiers[index];
                try { (void)Lexeur(std::string_view(fichier.Source, fichier.Taille), noms[index]).Analyser(); }
                catch (const ErreurCompilation& erreur)
                {
                    if (erreur.Ligne() != diagnostic.LigneErreur || erreur.Colonne() != diagnostic.ColonneErreur)
                        incoherent();
                    throw;
                }
                incoherent();
            }
            if (diagnostic.DetailLexical) incoherent();
            const char* fr = nullptr; const char* en = nullptr;
            switch (diagnostic.Erreur)
            {
            case 5: fr = "une directive doit commencer une ligne"; en = "a directive must start a line"; break;
            case 6: fr = "argument de directive attendu"; en = "expected directive argument"; break;
            case 7: fr = "texte inattendu après la directive"; en = "unexpected text after directive"; break;
            case 8: fr = "seul '#pragma once' est pris en charge"; en = "only '#pragma once' is supported"; break;
            case 9: fr = "chemin d'inclusion entre guillemets attendu"; en = "expected quoted include path"; break;
            case 10: fr = "fichier inclus introuvable"; en = "included file not found"; break;
            case 11: fr = "extension d'inclusion incompatible avec Gs++"; en = "include extension is incompatible with Gs++"; break;
            case 12: fr = "cycle d'inclusion détecté"; en = "include cycle detected"; break;
            case 13:
                throw ErreurCompilation("cycle ou profondeur excessive d'inclusion : " + chemins[index].string(),
                    "include cycle or excessive depth: " + chemins[index].string(),
                    diagnostic.LigneErreur, diagnostic.ColonneErreur, noms[index]);
            default: incoherent();
            }
            throw ErreurCompilation(fr, en, diagnostic.LigneErreur, diagnostic.ColonneErreur, noms[index]);
        }
        if (diagnostic.DetailLexical || diagnostic.IndexFichierErreur != fichiers.size()
            || diagnostic.LigneErreur || diagnostic.ColonneErreur
            || diagnostic.NombreOctetsSource != preparation.Source.size()
            || diagnostic.NombreOrigines != preparation.Origines.size() || preparation.Origines.empty()) incoherent();
        std::vector<Jeton> jetons;
        try { jetons = Lexeur(preparation.Source).Analyser(); }
        catch (const ErreurCompilation&) { incoherent(); }
        if (jetons.size() != preparation.Origines.size()) incoherent();
        std::size_t position = preparation.Source.starts_with("\xEF\xBB\xBF") ? 3 : 0;
        std::size_t ligne = 1, colonne = 1;
        auto avancer = [&](std::size_t fin)
        {
            while (position < fin)
            {
                if (preparation.Source[position++] == '\n') { ++ligne; colonne = 1; }
                else ++colonne;
            }
        };
        auto separateur = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f'; };
        for (std::size_t i = 0; i < jetons.size(); ++i)
        {
            const auto& origine = preparation.Origines[i];
            const bool fin = i + 1 == jetons.size();
            if (origine.Reserve || origine.EstInterface > 1 || origine.IndexFichier >= fichiers.size()
                || disponibles[origine.IndexFichier] != 1 || !origine.Ligne || !origine.Colonne
                || origine.DebutOctets < position || origine.DebutOctets > preparation.Source.size()
                || origine.TailleOctets > preparation.Source.size() - origine.DebutOctets
                || (fin ? (origine.TailleOctets != 0 || origine.DebutOctets != preparation.Source.size()
                    || origine.IndexFichier != racine || origine.EstInterface != 0) : origine.TailleOctets == 0)) incoherent();
            while (position < origine.DebutOctets)
            {
                if (!separateur(preparation.Source[position])) incoherent();
                avancer(position + 1);
            }
            if (jetons[i].Ligne != ligne || jetons[i].Colonne != colonne) incoherent();
            if (!fin)
            {
                if (origine.EstInterface != fichiers[origine.IndexFichier].EstInterface) incoherent();
                std::vector<Jeton> fragment;
                try { fragment = Lexeur(std::string_view(preparation.Source).substr(
                    origine.DebutOctets, origine.TailleOctets)).Analyser(); }
                catch (const ErreurCompilation&) { incoherent(); }
                if (fragment.size() != 2 || fragment[0].Ligne != 1 || fragment[0].Colonne != 1
                    || fragment[0].Genre != jetons[i].Genre || fragment[0].Texte != jetons[i].Texte
                    || jetons[i].Genre == GenreJeton::DirectiveInclure || jetons[i].Genre == GenreJeton::DirectivePragma)
                    incoherent();
                avancer(static_cast<std::size_t>(origine.DebutOctets + origine.TailleOctets));
            }
            jetons[i].Fichier = noms[origine.IndexFichier];
            jetons[i].Ligne = origine.Ligne; jetons[i].Colonne = origine.Colonne;
            jetons[i].EstInterface = origine.EstInterface != 0;
        }
        return jetons;
    }

    std::vector<Jeton> PreparerJetonsAvecReprise(const UniteSource& racine, ExpanseurInclusionsAvecRepriseHote developper)
    {
        return ConvertirPreparationEnJetons(PreparerFichierAvecReprise(racine, developper));
    }
}
