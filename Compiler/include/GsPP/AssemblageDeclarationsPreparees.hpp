#pragma once

#include "GsPP/DeclarationsPreparees.hpp"

#include <span>

namespace GsPP
{
    // Miroirs du contrat d'assemblage public, GsAbi:x64-ms-v1, sans nouvelle ABI.
    struct UniteDeclarationsPrepareeHote
    {
        const char* Source;
        std::uint64_t Taille;
        std::uint32_t EstInterface, Reserve;
    };

    struct OrigineUniteDeclarationsHote
    {
        std::uint64_t DebutOctets, TailleOctets;
        std::uint32_t PremiereLigne, NombreLignes, OctetsBom, Reserve;
    };

    struct ResultatAssemblageDeclarationsHote
    {
        std::uint32_t Erreur, LigneErreur, ColonneErreur, Detail;
        std::uint64_t IndexUniteErreur, NombreNoeuds, NombreOctetsSource, NombreOrigines, NombreOctetsArene;
        std::uint32_t DetailLexical, Reserve;
    };

    struct RequeteAssemblageDeclarationsHote
    {
        const UniteDeclarationsPrepareeHote* Unites;
        std::uint64_t NombreUnites;
        char* SourceAssemblee;
        std::uint64_t CapaciteSource;
        NoeudDeclarationHote* Noeuds;
        std::uint64_t CapaciteNoeuds;
        OrigineUniteDeclarationsHote* Origines;
        std::uint64_t CapaciteOrigines;
        ResultatAssemblageDeclarationsHote Resultat;
    };

    struct TableOriginesUniteDeclarationsHote
    {
        const OrigineJetonDeclarationsHote* Jetons;
        std::uint64_t NombreJetons;
    };

    struct RequeteAssemblageAvecOriginesHote
    {
        RequeteAssemblageDeclarationsHote* Assemblage;
        const TableOriginesUniteDeclarationsHote* Tables;
        std::uint64_t NombreFichiers, IndexFichierErreur;
        std::uint32_t LigneLocaleErreur, ColonneLocaleErreur;
    };

    struct RequeteDiagnosticAssemblageHote
    {
        RequeteAssemblageAvecOriginesHote* Assemblage;
        char* Nom;
        std::uint64_t CapaciteNom, NombreOctetsNom, Reserve;
    };

    static_assert(sizeof(UniteDeclarationsPrepareeHote) == 24);
    static_assert(sizeof(OrigineUniteDeclarationsHote) == 32);
    static_assert(sizeof(ResultatAssemblageDeclarationsHote) == 64);
    static_assert(sizeof(RequeteAssemblageDeclarationsHote) == 128);
    static_assert(offsetof(RequeteAssemblageDeclarationsHote, Resultat) == 64);
    static_assert(sizeof(TableOriginesUniteDeclarationsHote) == 16);
    static_assert(sizeof(RequeteAssemblageAvecOriginesHote) == 40);
    static_assert(sizeof(RequeteDiagnosticAssemblageHote) == 40);
    static_assert(offsetof(RequeteDiagnosticAssemblageHote, NombreOctetsNom) == 24);

#if defined(__GNUC__) && defined(__x86_64__) && !defined(_WIN32)
    using AssembleurDeclarationsAvecOriginesHote = std::uint32_t (__attribute__((ms_abi)) *)(RequeteAssemblageAvecOriginesHote*);
    using AssembleurDeclarationsAvecDiagnosticHote = std::uint32_t (__attribute__((ms_abi)) *)(RequeteDiagnosticAssemblageHote*);
#else
    using AssembleurDeclarationsAvecOriginesHote = std::uint32_t (*)(RequeteAssemblageAvecOriginesHote*);
    using AssembleurDeclarationsAvecDiagnosticHote = std::uint32_t (*)(RequeteDiagnosticAssemblageHote*);
#endif

    struct LimitesAssemblageDeclarations : LimitesDeclarationsPreparees
    {
        std::uint64_t MaximumUnites = 1024;
        std::uint64_t MaximumFichiers = 1'000'000;
        std::uint64_t MaximumOctetsNom = 1'048'576;
    };

    struct OrigineNoeudAssemble
    {
        PositionSource Position;
        std::size_t IndexUnite = 0;
        bool EstInterface = false;
    };

    /**
     * <résumé>Possède le texte assemblé, l'AST compact et les positions originales de ses unités.</résumé>
     * @etc. La racine commune porte IndexUnite == Unites.size(), sans unité d'appartenance.
     * @etc. Un refus situé ne publie que Resultat, PositionErreur et éventuellement NomConflit, jamais d'arbre partiel.
     * @etc. L'entrée avec diagnostic possède le nom qualifié ; les entrées historiques restent numériques.
     * @etc. HachageType reste opaque ; aucune reconstruction de Programme n'est effectuée.
     **/
    struct DeclarationsAssemblees
    {
        ResultatAssemblageDeclarationsHote Resultat{};
        std::string Source;
        std::vector<NoeudDeclarationHote> Noeuds;
        std::vector<OrigineNoeudAssemble> Origines;
        std::vector<OrigineUniteDeclarationsHote> Unites;
        std::optional<PositionSource> PositionErreur;
        std::string NomConflit;

        [[nodiscard]] std::string_view Nom(std::size_t index) const;
        /** <résumé>Lève un refus bilingue possédé ; un conflit numérique sans nom reste une erreur de contrat hôte.</résumé> **/
        void ExigerValide() const;
    };

    /**
     * <résumé>Valide les instantanés puis appelle l'assemblage Gs++ brut ou normalisé avec origines.</résumé>
     * @Paramètre(span: preparations) Instantanés non nuls, dans l'ordre des unités ; aucun fichier relu.
     * @Paramètre(AssembleurDeclarationsAvecOriginesHote: assembler) Export brut ou normalisé de confiance, image chargée.
     * @Retourner(DeclarationsAssemblees) Sorties possédées ou refus numérique à sa position originale.
     * @etc. Le frontend Gs++ réanalyse les textes préparés ; aucun parseur syntaxique C++ n'est appelé.
     * @etc. Limites cumulées, copies privées, mesure/publication et gardes des trois sorties.
     **/
    [[nodiscard]] DeclarationsAssemblees AssemblerDeclarationsPreparees(
        std::span<const ResultatPreparationFichier* const> preparations,
        AssembleurDeclarationsAvecOriginesHote assembler, const LimitesAssemblageDeclarations& limites = {});

    /**
     * <résumé>Possède aussi le nom qualifié du conflit Gs++, pour un diagnostic bilingue exact après déchargement.</résumé>
     * @Paramètre(span: preparations) Instantanés validés, aucun fichier relu ni parseur syntaxique C++ appelé.
     * @Paramètre(AssembleurDeclarationsAvecDiagnosticHote: assembler) Export normalisé avec diagnostic de confiance.
     * @Retourner(DeclarationsAssemblees) AST complet ou refus situé avec nom possédé, utilisable par ExigerValide.
     * @etc. Un conflit exige une seconde analyse bornée pour copier son nom ; succès et refus syntaxique gardent le protocole usuel.
     **/
    [[nodiscard]] DeclarationsAssemblees AssemblerDeclarationsNormaliseesAvecDiagnostic(
        std::span<const ResultatPreparationFichier* const> preparations,
        AssembleurDeclarationsAvecDiagnosticHote assembler, const LimitesAssemblageDeclarations& limites = {});
}
