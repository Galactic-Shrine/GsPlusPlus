#pragma once

#include "GsPP/Ast.hpp"
#include "GsPP/Jeton.hpp"

#include <filesystem>
#include <functional>
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
    using PreparateurJetonsUnite = std::function<std::vector<Jeton>(const UniteSource&)>;
    /** <résumé>Partage les passes bootstrap avec une préparation de jetons explicitement fournie.</résumé> **/
    [[nodiscard]] Programme AnalyserUnitesAvecPreparation(
        const std::vector<UniteSource>& unites, const PreparateurJetonsUnite& preparer,
        std::ostream* sortieJetons = nullptr);
    void NormaliserDeclarations(Programme& programme);
    [[nodiscard]] std::vector<Jeton> PreparerJetonsSource(
        const std::filesystem::path& chemin, const std::string& nomDiagnostic = {});
}
