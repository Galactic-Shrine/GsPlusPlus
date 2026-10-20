#pragma once

#include "GsPP/AssemblageDeclarationsPreparees.hpp"

namespace GsPP
{
    struct SymboleSemantiqueHote
    {
        std::uint64_t IndexNoeud, IndexPortee, HachageNom, HachageEspace, HachageType;
        std::uint32_t Genre, Drapeaux;
    };

    struct ResolutionSemantiqueHote
    {
        std::uint64_t IndexNoeud, IndexSymbole, HachageType;
        std::uint32_t GenreCible, Drapeaux;
    };

    struct ResultatAnalyseSemantiqueHote
    {
        std::uint32_t Erreur, LigneErreur, ColonneErreur, Detail;
        std::uint64_t NombreSymboles, CapaciteSymbolesRequise, NombreResolutions, CapaciteResolutionsRequise, NombreOctetsArene;
    };

    struct RequeteAnalyseSemantiqueHote
    {
        const char* Source;
        std::uint64_t TailleSource;
        const NoeudDeclarationHote* Noeuds;
        std::uint64_t NombreNoeuds;
        SymboleSemantiqueHote* Symboles;
        std::uint64_t CapaciteSymboles;
        ResolutionSemantiqueHote* Resolutions;
        std::uint64_t CapaciteResolutions;
        ResultatAnalyseSemantiqueHote Resultat;
    };

    struct RequeteAnalyseSemantiqueUnitesHote
    {
        RequeteAnalyseSemantiqueHote* Analyse;
        const OrigineUniteDeclarationsHote* Origines;
        std::uint64_t NombreOrigines, IndexUniteErreur;
        std::uint32_t LigneLocaleErreur, ColonneLocaleErreur;
    };

    static_assert(sizeof(SymboleSemantiqueHote) == 48);
    static_assert(sizeof(ResolutionSemantiqueHote) == 32);
    static_assert(sizeof(ResultatAnalyseSemantiqueHote) == 56);
    static_assert(sizeof(RequeteAnalyseSemantiqueHote) == 120);
    static_assert(offsetof(RequeteAnalyseSemantiqueHote, Resultat) == 64);
    static_assert(sizeof(RequeteAnalyseSemantiqueUnitesHote) == 40);

#if defined(__GNUC__) && defined(__x86_64__) && !defined(_WIN32)
    using AnalyseurSemantiqueUnitesHote = std::uint32_t (__attribute__((ms_abi)) *)(RequeteAnalyseSemantiqueUnitesHote*);
#else
    using AnalyseurSemantiqueUnitesHote = std::uint32_t (*)(RequeteAnalyseSemantiqueUnitesHote*);
#endif

    struct LimitesSemantiqueAssemblee
    {
        std::uint64_t MaximumOctets = 67'108'864;
        std::uint64_t MaximumNoeuds = 1'000'000;
        std::uint64_t MaximumUnites = 1024;
        std::uint64_t MaximumSymboles = 1'000'000;
        std::uint64_t MaximumResolutions = 1'000'000;
    };

    /**
     * <résumé>Possède l'AST d'entrée et les sorties complètes de sa passe sémantique Gs++.</résumé>
     * @etc. Les indices des symboles et résolutions se rapportent à Declarations.Noeuds, jamais à l'arène native.
     * @etc. Sur refus numérique situé, Declarations conserve l'entrée complète ; aucun préfixe sémantique n'est publié.
     * @etc. FonctionAbsente (20) est global : PositionErreur absente, IndexUniteErreur égal au nombre d'unités.
     * @etc. HachageType reste opaque, sauf EtapeDureeVie où il décrit un décalage ; aucun Programme C++ reconstruit.
     **/
    struct SemantiqueAssemblee
    {
        DeclarationsAssemblees Declarations;
        ResultatAnalyseSemantiqueHote Resultat{};
        std::vector<SymboleSemantiqueHote> Symboles;
        std::vector<ResolutionSemantiqueHote> Resolutions;
        std::optional<PositionSource> PositionErreur;
        std::size_t IndexUniteErreur = 0;

        /** <résumé>Indique si un message invariant, contextuel ou avec nom possédé est disponible ; ne valide pas le contrat.</résumé> **/
        [[nodiscard]] bool PeutTraduireDiagnostic() const noexcept;

        /**
         * <résumé>Vérifie le résultat possédé et lève un diagnostic bilingue exact s'il est pris en charge.</résumé>
         * @etc. Aucun fichier relu ni appel natif ; le refus global sans fonction n'a pas de fichier.
         * @etc. Invariants 39/85..88/101/113/115 : champs par défaut, énumérations, références de tableaux et héritage.
         * @etc. Code 16 invariant ; codes 12/13/15/17 : nom simple de la déclaration répétée, Detail index AST précédent.
         * @etc. Genre, portée, ordre et deux noms possédés vérifiés ; aucune recherche de déclaration ou résolution de type.
         * @etc. Code 14 : position de l'alias, Detail index AST du champ en conflit, même déclaré après l'alias.
         * @etc. Code 107 : nom de l'alias revisité ; Detail rang de symbole de l'entrée, même pour un préfixe acyclique.
         * @etc. Alias, entrée et même agrégat/unité vérifiés, sans chercher les cibles ni refaire la détection de cycle.
         * @etc. Code 108 : nom cible possédé dans le nœud nominal 37, Detail index AST, position de l'alias.
         * @etc. Ancien AST sans cible nominale (Detail 0) numérique ; aucune inversion de hachage ou résolution hôte.
         * @etc. Code 109 : nom complet de l'alias revisité, namespace compris ; Detail rang du symbole d'entrée.
         * @etc. Entrée et alias signalé peuvent appartenir à deux unités/espaces différents ; aucun graphe reconstruit.
         * @etc. Segments nominaux 39 et préfixes cumulés vérifiés ; ancien AST sans préfixe de namespace numérique.
         * @etc. Code 110 : cible écrite qualifiée possédée dans le nœud nominal 38, Detail index AST, position de l'alias.
         * @etc. Tranches, lexèmes, hachages, unité et rang lexical vérifiés ; aucun nom/type/namespace résolu dans l'hôte.
         * @etc. Ancien AST sans cible nominale (Detail 0) numérique ; cycles 109 acceptent l'enfant 38 ou l'ancien AST.
         * @etc. Code 112 : cible écrite dans l'enfant 38 de l'alias à l'origine ; Detail reste un nombre de surcharges.
         * @etc. Ce nombre est borné par les nœuds de fonction, sans reconstruire les candidats ; ancien AST numérique.
         * @etc. Code 120 : nom écrit qualifié de l'espace utilisé, nœud 36 unique ; Detail nul, aucune résolution hôte.
         * @etc. Parent racine, feuille nominale, tranches/hachages/unité et phase sans résolutions contrôlés.
         * @etc. Code 121 : nom écrit de référence 24 ou cible d'alias 38 ; Detail nul, aucun import/candidat résolu.
         * @etc. Code 121 d'héritage : Detail indexe la base nominale 40 ; position de la classe, sans recherche hôte.
         * @etc. Code 121 de type déclaré : Detail indexe la feuille 41 ; empreinte du type et décalage lexical contrôlés.
         * @etc. Variables/champs/paramètres/retours directs ; callbacks et conversions sans nom nominal restent numériques.
         * @etc. Anciens AST sans enfant 38, 40 ou 41 numériques, sans reconstruction de type/candidat dans l'hôte.
         * @etc. Codes 18/19/24 : lexage limité à la tranche du nom possédé, sans analyse syntaxique/sémantique C++.
         * @etc. Code 23 : quatre variantes via l'accès ./-> et son rôle de cible d'appel, sans résolution de type.
         * @etc. Code 25 : Detail indexe les genres symboliques dans l'ordre AST ; champ, méthode et opérateur distincts.
         * @etc. Le nom d'un champ reste celui de l'accès (alias compris) ; celui d'une méthode vient de sa déclaration.
         * @etc. Codes 34/35 : champ canonique désigné par Detail, initialiseur et constructeur de la même classe.
         * @etc. Code 33 : nom écrit ; Detail 0 (absent) ou 1 (hérité), sans résolution de champ/base hôte.
         * @etc. Codes 38/40 : Detail désigne le champ déclaré à l'origine du refus ; valeur par défaut ou constante.
         * @etc. Code 36 et code 41 explicite : champ canonique et arguments de l'initialiseur vérifiés.
         * @etc. Code 41 par défaut reste numérique : son message exige les types résolus absents du contrat.
         * @etc. Les autres familles restent numériques et lèvent une erreur hôte, sans message source inventé.
         **/
        void ExigerValide() const;
    };

    /**
     * <résumé>Analyse un assemblage réussi en conservant l'isolation des unités et ses origines possédées.</résumé>
     * @Paramètre(DeclarationsAssemblees: declarations) AST réussi et origines inchangées ; normalement issu de la normalisation.
     * @Paramètre(AnalyseurSemantiqueUnitesHote: analyser) Export de confiance, image encore chargée pendant l'appel.
     * @Retourner(SemantiqueAssemblee) Sorties complètes possédées ou refus numérique, situé s'il concerne un nœud source.
     * @etc. Pas de normalisation implicite, de fichier relu, de catalogue requis ni d'analyse syntaxique/sémantique C++.
     * @etc. Copies privées, bornes avant appel, mesure/publication et gardes ; préfixes natifs jetés sur refus/exception.
     **/
    [[nodiscard]] SemantiqueAssemblee AnalyserSemantiqueAssemblee(const DeclarationsAssemblees& declarations,
        AnalyseurSemantiqueUnitesHote analyser, const LimitesSemantiqueAssemblee& limites = {});
}
