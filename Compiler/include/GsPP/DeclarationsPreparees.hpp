#pragma once

#include "GsPP/CatalogueInclusions.hpp"

#include <optional>
#include <string_view>

namespace GsPP
{
    // Miroirs du contrat public de l'analyse syntaxique avec origines (GsAbi:x64-ms-v1).
    struct NoeudDeclarationHote
    {
        std::uint32_t Genre, Ligne, Colonne, Drapeaux;
        std::uint64_t Parent, DebutNom, TailleNom, HachageNom, HachageEspace, HachageType;
    };

    struct ResultatAnalyseDeclarationsHote
    {
        std::uint32_t Erreur, LigneErreur, ColonneErreur, Detail;
        std::uint64_t NombreNoeuds, CapaciteRequise, NombreOctetsArene, Reserve;
    };

    struct RequeteAnalyseDeclarationsHote
    {
        const char* Source;
        std::uint64_t Taille;
        NoeudDeclarationHote* Noeuds;
        std::uint64_t Capacite;
        ResultatAnalyseDeclarationsHote Resultat;
    };

    struct RequeteDeclarationsAvecOriginesHote
    {
        RequeteAnalyseDeclarationsHote* Analyse;
        const OrigineJetonDeclarationsHote* Origines;
        std::uint64_t NombreOrigines, NombreFichiers, IndexFichierErreur;
        std::uint32_t LigneLocaleErreur, ColonneLocaleErreur, EstInterface, Reserve;
    };

    static_assert(sizeof(NoeudDeclarationHote) == 64);
    static_assert(sizeof(ResultatAnalyseDeclarationsHote) == 48);
    static_assert(sizeof(RequeteAnalyseDeclarationsHote) == 80);
    static_assert(offsetof(RequeteAnalyseDeclarationsHote, Resultat) == 32);
    static_assert(sizeof(RequeteDeclarationsAvecOriginesHote) == 56);
    static_assert(offsetof(RequeteDeclarationsAvecOriginesHote, EstInterface) == 48);

#if defined(__GNUC__) && defined(__x86_64__) && !defined(_WIN32)
    using AnalyseurDeclarationsAvecOriginesHote = std::uint32_t (__attribute__((ms_abi)) *)(RequeteDeclarationsAvecOriginesHote*);
#else
    using AnalyseurDeclarationsAvecOriginesHote = std::uint32_t (*)(RequeteDeclarationsAvecOriginesHote*);
#endif

    struct LimitesDeclarationsPreparees
    {
        std::uint64_t MaximumOctets = 64 * 1024 * 1024;
        std::uint64_t MaximumJetons = 1'000'000;
        std::uint64_t MaximumNoeuds = 1'000'000;
    };

    struct OrigineNoeudDeclaration
    {
        PositionSource Position;
        bool EstInterface = false;
    };

    /**
     * <résumé>Possède le texte et l'arbre compact de l'analyseur Gs++, sans vue vers le catalogue ou l'image.</résumé>
     * @etc. Les nœuds gardent leurs coordonnées synthétiques ; Origines contient les positions originales.
     * @etc. Sur refus syntaxique, seuls Resultat et PositionErreur sont publiés, sans arbre partiel.
     * @etc. HachageType reste opaque ; ce résultat ne remplace pas encore le Programme C++.
     **/
    struct DeclarationsPreparees
    {
        ResultatAnalyseDeclarationsHote Resultat{};
        std::string Source;
        std::vector<NoeudDeclarationHote> Noeuds;
        std::vector<OrigineNoeudDeclaration> Origines;
        std::optional<PositionSource> PositionErreur;

        [[nodiscard]] std::string_view Nom(std::size_t index) const;

        /**
         * <résumé>Lève le refus syntaxique comme ErreurCompilation bilingue, à la position originale possédée.</résumé>
         * @etc. Ne relit ni source ni image ; aucun effet sur un résultat syntaxiquement valide.
         * @etc. Un code ou détail incompatible reste une erreur de contrat hôte.
         **/
        void ExigerValide() const;
    };

    /**
     * <résumé>Valide la préparation, mesure puis publie l'AST Gs++ et ses origines possédées.</résumé>
     * @Paramètre(ResultatPreparationFichier: preparation) Instantané déjà développé, sans nouvelle lecture de fichier.
     * @Paramètre(AnalyseurDeclarationsAvecOriginesHote: analyser) Export de confiance, image chargée pendant les deux appels.
     * @Paramètre(bool: estInterface) Mode global d'interface, en plus du mode de la racine et des jetons.
     * @Retourner(DeclarationsPreparees) Arbre compact ou refus syntaxique numérique situé dans le fichier original.
     * @etc. Contrat incohérent, capacité excessive ou allocation impossible : exception hôte, pas faux diagnostic de langue.
     * @etc. Les copies privées protègent l'instantané contre une modification accidentelle des entrées par le rappel.
     **/
    [[nodiscard]] DeclarationsPreparees AnalyserDeclarationsPreparees(
        const ResultatPreparationFichier& preparation, AnalyseurDeclarationsAvecOriginesHote analyser,
        bool estInterface = false, const LimitesDeclarationsPreparees& limites = {});
}
