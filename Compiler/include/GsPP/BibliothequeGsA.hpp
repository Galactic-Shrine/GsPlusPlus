#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace GsPP
{
    inline constexpr std::uint16_t VersionMajeureGsA = 1;
    inline constexpr std::uint16_t VersionMineureGsA = 0;
    inline constexpr std::uint16_t VersionAbiGsA = 1;
    inline constexpr std::size_t TailleEnteteGsA = 32;

    struct MembreGsA
    {
        std::string Nom;
        std::vector<std::uint8_t> Objet;
    };

    class EcrivainGsA final
    {
    public:
        [[nodiscard]] std::vector<std::uint8_t> Construire(
            const std::vector<MembreGsA>& membres) const;
        void Ecrire(
            const std::vector<MembreGsA>& membres,
            const std::filesystem::path& chemin) const;
    };

    class LecteurGsA final
    {
    public:
        [[nodiscard]] std::vector<MembreGsA> Lire(
            const std::vector<std::uint8_t>& contenu) const;
        [[nodiscard]] std::vector<MembreGsA> Lire(
            const std::filesystem::path& chemin) const;
    };
}
