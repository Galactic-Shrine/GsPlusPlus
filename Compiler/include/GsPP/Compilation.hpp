#pragma once

#include "GsPP/Ast.hpp"

#include <filesystem>
#include <iosfwd>
#include <vector>

namespace GsPP
{
    struct UniteSource
    {
        std::filesystem::path Chemin;
        bool EstInterface = false;
        std::string NomDiagnostic;
    };

    [[nodiscard]] bool EstExtensionSource(const std::filesystem::path& chemin);
    [[nodiscard]] bool EstExtensionInterface(const std::filesystem::path& chemin);
    [[nodiscard]] bool EstExtensionGsSharp(const std::filesystem::path& chemin);
    [[nodiscard]] bool EstExtensionObsolete(const std::filesystem::path& chemin);
    [[nodiscard]] Programme AnalyserUnites(
        const std::vector<UniteSource>& unites,
        std::ostream* sortieJetons = nullptr);
    void NormaliserDeclarations(Programme& programme);
}
