#pragma once

#include "GsPP/CatalogueInclusions.hpp"
#include "GsPP/DeclarationsPreparees.hpp"
#include "GsPP/AssemblageDeclarationsPreparees.hpp"
#include "GsPP/SemantiqueAssemblee.hpp"

#include <memory>

namespace GsPP
{
    /**
     * <résumé>Possède le chargement exécutable d'une image d'expansion explicitement choisie.</résumé>
     * @Paramètre(path: chemin) Image GsE de confiance : son code natif s'exécute dans le processus hôte.
     * @etc. Aucune découverte automatique, aucun appel du point d'entrée, aucun bac à sable.
     * @etc. L'image doit rester chargée jusqu'à la destruction de toutes ses sessions.
     **/
    class ExpanseurInclusionsCharge final
    {
    public:
        explicit ExpanseurInclusionsCharge(const std::filesystem::path& chemin);
        ~ExpanseurInclusionsCharge();
        ExpanseurInclusionsCharge(const ExpanseurInclusionsCharge&) = delete;
        ExpanseurInclusionsCharge& operator=(const ExpanseurInclusionsCharge&) = delete;
        [[nodiscard]] ExpanseurInclusionsAvecRepriseHote Developper() const noexcept;
        /** <résumé>Vérifie les exports syntaxiques FR/EN à la demande ; aucune obligation pour l'expansion seule.</résumé> **/
        [[nodiscard]] AnalyseurDeclarationsAvecOriginesHote AnalyseurDeclarations() const;
        /** <résumé>Vérifie à la demande la paire d'exports d'assemblage brut ou normalisé avec origines.</résumé> **/
        [[nodiscard]] AssembleurDeclarationsAvecOriginesHote AssembleurDeclarations(bool normaliser = true) const;
        /** <résumé>Vérifie à la demande les exports normalisés avec nom qualifié de diagnostic possédé.</résumé> **/
        [[nodiscard]] AssembleurDeclarationsAvecDiagnosticHote AssembleurDeclarationsAvecDiagnostic() const;
        /** <résumé>Vérifie à la demande la paire sémantique FR/EN avec isolation des unités de traduction.</résumé> **/
        [[nodiscard]] AnalyseurSemantiqueUnitesHote AnalyseurSemantiqueUnites() const;
        /** <résumé>Vérifie à la demande les exports facultatifs de sémantique avec préfixe canonique possédé.</résumé> **/
        [[nodiscard]] AnalyseurSemantiqueUnitesAvecNomHote AnalyseurSemantiqueUnitesAvecNom() const;
        /** <résumé>Entrée facultative 99/100/118 ; nulle si les deux exports sont absents, incohérences refusées.</résumé> **/
        [[nodiscard]] AnalyseurSemantiqueUnitesAvecContexteHote AnalyseurSemantiqueUnitesAvecContexte() const;
        /** <résumé>Export facultatif : empreinte et contexte de collision 119 ; paire FR/EN absente admise.</résumé> **/
        [[nodiscard]] AnalyseurSemantiqueUnitesAvecLiaisonHote AnalyseurSemantiqueUnitesAvecLiaison() const;
        /** <résumé>Export facultatif : groupe canonique d'appel ambigu 22, sans sélection hôte.</résumé> **/
        [[nodiscard]] AnalyseurSemantiqueUnitesAvecAppelsHote AnalyseurSemantiqueUnitesAvecAppels() const;
        /** <résumé>Export facultatif : nom appelé des imports multigroupes ambigus 22.</résumé> **/
        [[nodiscard]] AnalyseurSemantiqueUnitesAvecGroupesHote AnalyseurSemantiqueUnitesAvecGroupes() const;
        /** <résumé>Export facultatif : groupe sans surcharge compatible 21, sans sélection hôte.</résumé> **/
        [[nodiscard]] AnalyseurSemantiqueUnitesAvecEchecsAppelsHote AnalyseurSemantiqueUnitesAvecEchecsAppels() const;
        /** <résumé>Export facultatif : groupe de constructeurs refusé 21, sans nouvelle résolution hôte.</résumé> **/
        [[nodiscard]] AnalyseurSemantiqueUnitesAvecConstructionsHote AnalyseurSemantiqueUnitesAvecConstructions() const;
        [[nodiscard]] AnalyseurSemantiqueUnitesAvecConstructionsImplicitesHote AnalyseurSemantiqueUnitesAvecConstructionsImplicites() const;
        [[nodiscard]] AnalyseurSemantiqueUnitesAvecAliasAppelsHote AnalyseurSemantiqueUnitesAvecAliasAppels() const;

    private:
        struct Implementation;
        std::unique_ptr<Implementation> implementation_;
    };

    /**
     * <résumé>Raccorde explicitement les diagnostics sémantiques Gs++ au pipeline C++ de compilation directe.</résumé>
     * @etc. Syntaxe/normalisation C++ d'abord ; assemblage/sémantique Gs++ sur les mêmes instantanés, sans relecture.
     * @etc. La passe C++ conserve les annotations nécessaires au backend ; ce n'est pas un frontend autonome.
     * @etc. Refus numérique validé : repli C++ uniquement à la même position ; désaccord ou contrat incohérent propagé.
     * @etc. Image native de confiance explicite ; exige les exports normalisés, sémantiques et canoniques.
     **/
    [[nodiscard]] Programme AnalyserUnitesAvecDiagnosticsFrontend(const std::vector<UniteSource>& unites,
        const ExpanseurInclusionsCharge& frontend, std::ostream* sortieJetons = nullptr);
}
