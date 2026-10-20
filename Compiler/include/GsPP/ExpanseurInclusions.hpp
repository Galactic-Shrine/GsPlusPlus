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

    private:
        struct Implementation;
        std::unique_ptr<Implementation> implementation_;
    };
}
