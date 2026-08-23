#pragma once

#include "GsPP/GenerateurX64.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace GsPP
{
    struct MetadonneesGsE
    {
        std::string Nom = "Application";
        std::string Version = "0.1.0";
        std::string Editeur = "Galactic-Shrine";
        std::string Cible = "shrine-x86_64";
        std::string Abi = "shrine-x86_64-v2";
        std::string Langage = "Gs++";
    };

    class EcrivainGsE final
    {
    public:
        [[nodiscard]] std::vector<std::uint8_t> Construire(
            const CodeMachine& machine,
            const std::string& pointEntree,
            const MetadonneesGsE& metadonnees = {}) const;
        void Ecrire(
            const CodeMachine& machine,
            const std::string& pointEntree,
            const std::filesystem::path& chemin,
            const MetadonneesGsE& metadonnees = {}) const;
    };
}
