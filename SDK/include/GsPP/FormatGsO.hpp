#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace GsPP
{
    inline constexpr std::array<std::uint8_t, 7> SignatureGsObj{
        'G', 'S', 'O', 'B', 'J', ':', '0'};
    inline constexpr std::uint16_t VersionMajeureGsO = 1;
    inline constexpr std::uint16_t VersionMineureGsO = 0;
    inline constexpr std::uint16_t ArchitectureAmd64GsO = 0x8664;
    inline constexpr std::uint16_t VersionAbiGsO = 1;
    inline constexpr std::size_t TailleEnteteGsO = 112;
    inline constexpr std::size_t TailleEntreeSymboleGsO = 48;
    inline constexpr std::size_t TailleEntreeRelocalisationGsO = 24;
    inline constexpr std::size_t TailleNomSymboleGsOMaximale = 1024;
    inline constexpr std::size_t TailleSignatureAbiGsOMaximale = 65535;
    inline constexpr std::size_t TailleCheminSourceGsOMaximale = 65535;
}
