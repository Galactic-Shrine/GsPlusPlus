#pragma once

#include "GsPP/Abi.hpp"
#include "GsPP/GenerateurX64.hpp"
#include "GsPP/VersionProduit.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace GsPP
{
    struct MetadonneesGsE
    {
        std::string Nom = "Application";
        std::string Version{VersionProduit};
        std::string Editeur = "Galactic-Shrine";
        std::string Cible{IdentifiantCibleX64};
        std::string Abi{IdentifiantAbiX64Ms};
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
