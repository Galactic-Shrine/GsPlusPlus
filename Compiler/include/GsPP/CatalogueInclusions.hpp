#pragma once

#include "GsPP/Compilation.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace GsPP
{
    // Miroirs hôtes de l'ABI publique du frontend Gs++ (sans pointeur vers son AST).
    struct OrigineJetonDeclarationsHote
    {
        std::uint64_t DebutOctets, TailleOctets, IndexFichier;
        std::uint32_t Ligne, Colonne, EstInterface, Reserve;
    };

    struct FichierInclusionDeclarationsHote
    {
        const char* Source;
        std::uint64_t Taille, Identite;
        std::uint32_t EstInterface, Reserve;
    };

    struct LienInclusionDeclarationsHote
    {
        std::uint64_t IndexFichier, DebutDirective, IndexCible;
        std::uint32_t Etat, Reserve;
    };

    struct ResultatExpansionDeclarationsHote
    {
        std::uint32_t Erreur, DetailLexical;
        std::uint64_t IndexFichierErreur;
        std::uint32_t LigneErreur, ColonneErreur;
        std::uint64_t NombreOctetsSource, NombreOrigines, NombreOctetsArene;
    };

    struct RequeteExpansionDeclarationsHote
    {
        const FichierInclusionDeclarationsHote* Fichiers;
        std::uint64_t NombreFichiers;
        const LienInclusionDeclarationsHote* Liens;
        std::uint64_t NombreLiens, IndexRacine;
        char* SourcePreparee;
        std::uint64_t CapaciteSource;
        OrigineJetonDeclarationsHote* Origines;
        std::uint64_t CapaciteOrigines;
        std::uint32_t MarqueUtf8, FinLigneCrlf;
        ResultatExpansionDeclarationsHote Resultat;
    };

    static_assert(sizeof(OrigineJetonDeclarationsHote) == 40);
    static_assert(sizeof(FichierInclusionDeclarationsHote) == 32);
    static_assert(sizeof(LienInclusionDeclarationsHote) == 32);
    static_assert(sizeof(ResultatExpansionDeclarationsHote) == 48);
    static_assert(sizeof(RequeteExpansionDeclarationsHote) == 128);
    static_assert(offsetof(RequeteExpansionDeclarationsHote, Resultat) == 80);

#if defined(__GNUC__) && defined(__x86_64__) && !defined(_WIN32)
    using ExpanseurInclusionsHote = std::uint32_t (__attribute__((ms_abi)) *)(RequeteExpansionDeclarationsHote*);
#else
    using ExpanseurInclusionsHote = std::uint32_t (*)(RequeteExpansionDeclarationsHote*);
#endif

    struct LimitesCatalogueInclusions
    {
        std::uint64_t NombreFichiers = 4096;
        std::uint64_t NombreLiens = 100'000;
        std::uint64_t OctetsParFichier = 16 * 1024 * 1024;
        std::uint64_t OctetsSources = 64 * 1024 * 1024;
    };

    /**
     * <résumé>Possède un instantané des fichiers et les vues transmises au frontend.</résumé>
     * @etc. Non copiable : les vues resteraient attachées aux textes de l'ancien propriétaire.
     * @etc. Le déplacement conserve les vues ; l'objet déplacé devient vide et doit être reconstruit avant usage.
     **/
    class CatalogueInclusions
    {
    public:
        CatalogueInclusions() = default;
        CatalogueInclusions(const CatalogueInclusions&) = delete;
        CatalogueInclusions& operator=(const CatalogueInclusions&) = delete;
        CatalogueInclusions(CatalogueInclusions&& autre) noexcept;
        CatalogueInclusions& operator=(CatalogueInclusions&& autre) noexcept;

        const std::vector<FichierInclusionDeclarationsHote>& Fichiers() const noexcept { return fichiers_; }
        const std::vector<LienInclusionDeclarationsHote>& Liens() const noexcept { return liens_; }
        const std::vector<std::string>& NomsFichiers() const noexcept { return noms_; }
        std::uint64_t IndexRacine() const noexcept { return racine_; }

    private:
        // Les nœuds de map gardent les adresses des chaînes, y compris les chaînes courtes.
        std::map<std::string, std::string> contenus_;
        std::vector<FichierInclusionDeclarationsHote> fichiers_;
        std::vector<LienInclusionDeclarationsHote> liens_;
        std::vector<std::string> noms_;
        std::uint64_t racine_ = UINT64_MAX;

        friend CatalogueInclusions CreerCatalogueInclusions(const UniteSource&,
            const std::vector<UniteSource>&, const LimitesCatalogueInclusions&);
    };

    /**
     * <résumé>Lit et résout un graphe de chemins, sans sélectionner les jetons ni exécuter once.</résumé>
     * @Paramètre(UniteSource: racine) Chemin physique et nom de diagnostic facultatif, distincts.
     * @Paramètre(vector<UniteSource>: fichiersConnus) Préfixe du catalogue commun, indices conservés.
     * @Paramètre(LimitesCatalogueInclusions: limites) Bornes hôtes indépendantes de la profondeur sémantique de 128.
     * @Retourner(CatalogueInclusions) Instantané propriétaire et liens uniques triés.
     * @etc. Parcours itératif ; une seule lecture par identité canonique, aucune copie implicite.
     * @etc. Lecture anticipée de tout le graphe : les échecs d'E/S et les limites sont des erreurs hôtes.
     * @etc. Le lexeur C++ découvre les chemins ; les refus lexicaux restent dans les textes pour le frontend.
     **/
    CatalogueInclusions CreerCatalogueInclusions(const UniteSource& racine,
        const std::vector<UniteSource>& fichiersConnus = {}, const LimitesCatalogueInclusions& limites = {});

    struct SourcePrepareeAvecOrigines
    {
        std::string Source;
        std::vector<OrigineJetonDeclarationsHote> Origines;
        ResultatExpansionDeclarationsHote Resultat{};
    };

    /**
     * <résumé>Mesure puis publie les sorties possédées de l'expansion Gs++, ou son diagnostic.</résumé>
     * @etc. Un refus du frontend rend Source/Origines vides ; un contrat ABI incohérent lève une exception hôte.
     * @etc. Le pointeur exporté et son image chargée doivent rester valides pendant les deux appels.
     **/
    SourcePrepareeAvecOrigines PreparerSourceAvecOrigines(const CatalogueInclusions& catalogue,
        ExpanseurInclusionsHote developper, bool marqueUtf8 = false, bool finLigneCrlf = false,
        std::uint64_t maximumOctets = 64 * 1024 * 1024, std::uint64_t maximumOrigines = 1'000'000);
}
