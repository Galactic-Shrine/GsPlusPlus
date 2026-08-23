#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace GsPP
{
    struct SegmentChargeGsE
    {
        std::uint64_t Rva = 0;
        std::uint64_t Taille = 0;
        std::uint32_t Drapeaux = 0;
    };

    struct ImportChargeGsE
    {
        std::string Nom;
        std::uint16_t Type = 0;
        std::uint16_t Abi = 0;
        bool Obligatoire = false;
        bool Resolu = false;
        std::uint64_t Adresse = 0;
    };

    struct ExportChargeGsE
    {
        std::string Nom;
        std::uint64_t Adresse = 0;
        std::uint32_t Taille = 0;
        std::uint16_t Section = 0;
        std::uint16_t Type = 0;
    };

    struct ImageChargeeGsE
    {
        std::vector<std::uint8_t> Memoire;
        std::uint64_t BaseChargement = 0;
        std::uint64_t RvaPointEntree = 0;
        std::uint64_t AdressePointEntree = 0;
        std::string Metadonnees;
        std::vector<SegmentChargeGsE> Segments;
        std::vector<ImportChargeGsE> Imports;
        std::vector<ExportChargeGsE> Exports;

        [[nodiscard]] std::optional<std::uint64_t> ChercherExport(
            std::string_view nom) const;
    };

    using ResolveurImportGsE =
        std::function<std::optional<std::uint64_t>(std::string_view nom)>;

    class ChargeurGsE final
    {
    public:
        [[nodiscard]] ImageChargeeGsE Charger(
            const std::vector<std::uint8_t>& contenu,
            std::uint64_t baseChargement,
            const ResolveurImportGsE& resolveur = {}) const;
        [[nodiscard]] ImageChargeeGsE Charger(
            const std::filesystem::path& chemin,
            std::uint64_t baseChargement,
            const ResolveurImportGsE& resolveur = {}) const;
    };
}
