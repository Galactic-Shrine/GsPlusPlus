#pragma once

#include "GsPP/Compilation.hpp"

#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <vector>

namespace GsPP
{
    class ExpanseurInclusionsCharge;
    struct ResultatConstructionProjet
    {
        std::filesystem::path Sortie;
        std::size_t NombreUnites = 0;
    };

    struct OptionsConstructionProjet
    {
        std::filesystem::path Sortie;
        std::filesystem::path RepertoireObjets;
        /**
         * <résumé>Préparation facultative partagée par les interfaces et sources du projet.</résumé>
         * @etc. Vide : bootstrap. Le propriétaire des ressources capturées doit vivre jusqu'à la fin de la construction.
         **/
        PreparateurJetonsUnite PreparerJetons;
        /**
         * <résumé>Frontend de diagnostics emprunté pour toutes les compilations du projet.</résumé>
         * @etc. Doit rester chargé jusqu'à la fin ; incompatible avec PreparerJetons ; nul conserve le comportement existant.
         **/
        const ExpanseurInclusionsCharge* DiagnosticsFrontend = nullptr;
    };

    class ConstructeurProjet final
    {
    public:
        [[nodiscard]] ResultatConstructionProjet Construire(
            const std::filesystem::path& cheminProjet,
            std::ostream& journal,
            const OptionsConstructionProjet& options = {}) const;
        [[nodiscard]] std::vector<ResultatConstructionProjet> ConstruireSolution(
            const std::filesystem::path& cheminSolution,
            std::ostream& journal,
            const PreparateurJetonsUnite& preparerJetons = {},
            const ExpanseurInclusionsCharge* diagnosticsFrontend = nullptr) const;
    };
}
