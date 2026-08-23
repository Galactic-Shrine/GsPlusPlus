#pragma once

#include <cstddef>
#include <cstdint>

namespace GsPP
{
    inline constexpr std::uint16_t VersionMajeureGsE = 1;
    inline constexpr std::uint16_t VersionMineureGsE = 0;
    inline constexpr std::uint16_t VersionAbiGsE = 1;

    inline constexpr std::size_t TailleNomSymboleGsEMaximale = 1024;
    inline constexpr std::uint32_t TailleEntreeImportGsE = 32;
    inline constexpr std::uint32_t TailleEntreeExportGsE = 32;
    inline constexpr std::uint32_t TailleEntreeRelocalisationGsE = 24;

    inline constexpr std::uint16_t TypeRelocalisationImportRelatif32GsE = 1;
    inline constexpr std::uint16_t TypeRelocalisationImportAdresse64GsE = 2;
    inline constexpr std::uint16_t TypeRelocalisationBase64GsE = 3;
    inline constexpr std::uint32_t IndiceImportRelocalisationBaseGsE = 0xFFFFFFFFU;

    inline constexpr std::uint32_t TypeSectionTexteGsE = 1;
    inline constexpr std::uint32_t TypeSectionDonneesGsE = 2;
    inline constexpr std::uint32_t TypeSectionZeroGsE = 3;
    inline constexpr std::uint32_t TypeSectionImportsGsE = 4;
    inline constexpr std::uint32_t TypeSectionExportsGsE = 5;
    inline constexpr std::uint32_t TypeSectionRelocalisationsGsE = 6;
    inline constexpr std::uint32_t TypeSectionMetadonneesGsE = 7;
    inline constexpr std::uint32_t TypeSectionChainesGsE = 8;

    [[nodiscard]] inline bool Utf8GsEValide(
        const std::uint8_t* texte,
        std::size_t taille)
    {
        std::size_t position = 0;
        auto suite = [&](std::size_t index)
        { return index < taille && (texte[index] & 0xC0U) == 0x80U; };
        while (position < taille)
        {
            const auto premier = texte[position];
            if (premier <= 0x7FU) { ++position; continue; }
            if (premier >= 0xC2U && premier <= 0xDFU
                && suite(position + 1))
            { position += 2; continue; }
            if (premier == 0xE0U && position + 2 < taille
                && texte[position + 1] >= 0xA0U && texte[position + 1] <= 0xBFU
                && suite(position + 2))
            { position += 3; continue; }
            if (((premier >= 0xE1U && premier <= 0xECU)
                    || (premier >= 0xEEU && premier <= 0xEFU))
                && suite(position + 1) && suite(position + 2))
            { position += 3; continue; }
            if (premier == 0xEDU && position + 2 < taille
                && texte[position + 1] >= 0x80U && texte[position + 1] <= 0x9FU
                && suite(position + 2))
            { position += 3; continue; }
            if (premier == 0xF0U && position + 3 < taille
                && texte[position + 1] >= 0x90U && texte[position + 1] <= 0xBFU
                && suite(position + 2) && suite(position + 3))
            { position += 4; continue; }
            if (premier >= 0xF1U && premier <= 0xF3U
                && suite(position + 1) && suite(position + 2) && suite(position + 3))
            { position += 4; continue; }
            if (premier == 0xF4U && position + 3 < taille
                && texte[position + 1] >= 0x80U && texte[position + 1] <= 0x8FU
                && suite(position + 2) && suite(position + 3))
            { position += 4; continue; }
            return false;
        }
        return true;
    }
}
