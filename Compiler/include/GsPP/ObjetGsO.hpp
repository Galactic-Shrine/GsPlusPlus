#pragma once

#include "GsPP/GenerateurX64.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace GsPP
{
    class EcrivainGsO final
    {
    public:
        [[nodiscard]] std::vector<std::uint8_t> Construire(
            const CodeMachine& machine) const;
        void Ecrire(
            const CodeMachine& machine,
            const std::filesystem::path& chemin) const;
    };

    class LecteurGsO final
    {
    public:
        [[nodiscard]] CodeMachine Lire(
            const std::vector<std::uint8_t>& contenu) const;
        [[nodiscard]] CodeMachine Lire(
            const std::filesystem::path& chemin) const;
    };
}
