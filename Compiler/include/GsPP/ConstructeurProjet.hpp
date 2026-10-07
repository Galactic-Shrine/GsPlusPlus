#pragma once

#include "GsPP/Compilation.hpp"

#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <vector>

namespace GsPP
{
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
            const PreparateurJetonsUnite& preparerJetons = {}) const;
    };
}
