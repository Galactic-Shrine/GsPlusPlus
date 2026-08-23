#pragma once

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
            std::ostream& journal) const;
    };
}
