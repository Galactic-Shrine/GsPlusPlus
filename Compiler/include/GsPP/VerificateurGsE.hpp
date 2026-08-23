#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace GsPP
{
    struct RapportVerificationGsE
    {
        bool Valide = false;
        std::vector<std::string> Erreurs;
        std::vector<std::string> Informations;
    };

    class VerificateurGsE final
    {
    public:
        [[nodiscard]] RapportVerificationGsE Verifier(
            const std::vector<std::uint8_t>& contenu) const;
        [[nodiscard]] RapportVerificationGsE Verifier(
            const std::filesystem::path& chemin) const;
    };
}
