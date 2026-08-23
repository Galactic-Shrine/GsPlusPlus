#pragma once

#include "GsPP/GenerateurX64.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace GsPP
{
    struct UniteLiaison
    {
        std::string Nom;
        CodeMachine Machine;
    };

    using BibliothequeLiaison = std::vector<UniteLiaison>;

    class EditeurLiens final
    {
    public:
        [[nodiscard]] CodeMachine Lier(
            std::vector<UniteLiaison> objets,
            const std::vector<BibliothequeLiaison>& bibliotheques = {},
            const std::string& symboleRacine = {}) const;

        void EcrireCarte(
            const CodeMachine& machine,
            const std::filesystem::path& chemin) const;
    };
}
