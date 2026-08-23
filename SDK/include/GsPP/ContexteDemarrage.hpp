#pragma once

#include <cstddef>
#include <cstdint>

namespace GsPP
{
    struct PlageMemoirePhysique
    {
        void* Debut = nullptr;
        std::uint32_t NombrePages = 0;
        std::uint32_t Reserve = 0;
    };

    struct ContexteDemarrage
    {
        std::uint32_t Version = 3;
        std::uint32_t TailleStructure = sizeof(ContexteDemarrage);
        void* CarteMemoire = nullptr;
        std::uint32_t NombreDescripteursMemoire = 0;
        std::uint32_t TailleDescripteurMemoire = 0;
        void* TamponImage = nullptr;
        std::uint32_t LargeurEcran = 0;
        std::uint32_t HauteurEcran = 0;
        std::uint32_t PixelsParLigne = 0;
        std::uint32_t FormatPixels = 0;
        void* TablesAcpi = nullptr;
        void* BaseNoyau = nullptr;
        std::uint32_t TailleNoyau = 0;
        std::uint32_t Drapeaux = 0;
        void* ReservePages = nullptr;
        std::uint32_t NombrePagesReservees = 0;
        std::uint32_t ReserveArchitecture = 0;
        void* Gdt = nullptr;
        void* Idt = nullptr;
        void* TablePages = nullptr;
        PlageMemoirePhysique* PlagesMemoireLibres = nullptr;
        std::uint32_t NombrePlagesMemoireLibres = 0;
        std::uint32_t ReserveMemoire = 0;
        std::uint32_t DerniereException = 0;
        std::uint32_t CodeErreurException = 0;
        void* PileNoyau = nullptr;
        std::uint32_t NombrePagesPileNoyau = 0;
        std::uint32_t ReservePileNoyau = 0;
        void* PileIst = nullptr;
        std::uint32_t NombrePagesPileIst = 0;
        std::uint32_t ReserveIst = 0;
        void* Tss = nullptr;
        void* ApicLocal = nullptr;
        void* IoApic = nullptr;
        std::uint32_t NombreEntreesIoApic = 0;
        std::uint32_t IdApicLocal = 0;
        std::uint32_t NombreTicksHorloge = 0;
        std::uint32_t FrequenceHorloge = 0;
        std::uint32_t DernierCodeClavier = 0;
        std::uint32_t NombreInterruptionsClavier = 0;
        std::uint32_t DerniereInterruption = 0;
        std::uint32_t NombreInterruptionsInattendues = 0;
        std::uint32_t GsiHorloge = 0;
        std::uint32_t GsiClavier = 0;
        std::uint32_t EtapeDemarrage = 0;
        std::uint32_t ReserveDiagnostic = 0;
        void* AdresseInstructionException = nullptr;
        void* AdresseMemoireException = nullptr;
        void* AdresseInstructionInterruption = nullptr;
        void* PointeurCadreInterruption = nullptr;
        std::uint64_t SelecteurCodeInterruption = 0;
        std::uint64_t DrapeauxInterruption = 0;
        std::uint32_t TaillePageMemoire = 0;
        std::uint32_t NombrePagesTablesPagination = 0;
        std::uint32_t NombrePagesCartographiees = 0;
        std::uint32_t NombrePagesRecuperees = 0;
        void* LimitePagination = nullptr;
        std::uint32_t CapacitePlagesMemoire = 0;
        std::uint32_t ReserveGestionMemoire = 0;
    };

    inline constexpr std::uint32_t ContexteAvecTamponImage = 1U << 0;
    inline constexpr std::uint32_t ContexteAvecAcpi = 1U << 1;
    inline constexpr std::uint32_t ContexteAvecReservePages = 1U << 2;
    inline constexpr std::uint32_t ContexteAvecTablesArchitecture = 1U << 3;
    inline constexpr std::uint32_t ContexteAvecPagination = 1U << 4;
    inline constexpr std::uint32_t ContexteAvecPlagesMemoire = 1U << 5;
    inline constexpr std::uint32_t ContexteAvecDiagnosticsExceptions = 1U << 6;
    inline constexpr std::uint32_t ContexteAvecPileNoyau = 1U << 7;
    inline constexpr std::uint32_t ContexteAvecTssIst = 1U << 8;
    inline constexpr std::uint32_t ContexteAvecInterruptionsMaterielles = 1U << 9;
    inline constexpr std::uint32_t ContexteAvecApic = 1U << 10;
    inline constexpr std::uint32_t ContexteAvecHorloge = 1U << 11;
    inline constexpr std::uint32_t ContexteAvecClavier = 1U << 12;
    inline constexpr std::uint32_t ContexteAvecMemoireRecuperee = 1U << 13;
    inline constexpr std::uint32_t ContexteAvecGestionMemoireNoyau = 1U << 14;

    using BootContext = ContexteDemarrage;
    using PhysicalMemoryRange = PlageMemoirePhysique;
    inline constexpr std::uint32_t BootContextHasFramebuffer = ContexteAvecTamponImage;
    inline constexpr std::uint32_t BootContextHasAcpi = ContexteAvecAcpi;
    inline constexpr std::uint32_t BootContextHasPageReserve = ContexteAvecReservePages;
    inline constexpr std::uint32_t BootContextHasArchitectureTables = ContexteAvecTablesArchitecture;
    inline constexpr std::uint32_t BootContextHasPaging = ContexteAvecPagination;
    inline constexpr std::uint32_t BootContextHasMemoryRanges = ContexteAvecPlagesMemoire;
    inline constexpr std::uint32_t BootContextHasExceptionDiagnostics = ContexteAvecDiagnosticsExceptions;
    inline constexpr std::uint32_t BootContextHasKernelStack = ContexteAvecPileNoyau;
    inline constexpr std::uint32_t BootContextHasTssIst = ContexteAvecTssIst;
    inline constexpr std::uint32_t BootContextHasHardwareInterrupts = ContexteAvecInterruptionsMaterielles;
    inline constexpr std::uint32_t BootContextHasApic = ContexteAvecApic;
    inline constexpr std::uint32_t BootContextHasTimer = ContexteAvecHorloge;
    inline constexpr std::uint32_t BootContextHasKeyboard = ContexteAvecClavier;
    inline constexpr std::uint32_t BootContextHasReclaimedMemory = ContexteAvecMemoireRecuperee;
    inline constexpr std::uint32_t BootContextHasKernelMemoryManager = ContexteAvecGestionMemoireNoyau;

    static_assert(sizeof(PlageMemoirePhysique) == 16);
    static_assert(sizeof(ContexteDemarrage) == 320);
    static_assert(offsetof(ContexteDemarrage, CarteMemoire) == 8);
    static_assert(offsetof(ContexteDemarrage, TamponImage) == 24);
    static_assert(offsetof(ContexteDemarrage, TablesAcpi) == 48);
    static_assert(offsetof(ContexteDemarrage, BaseNoyau) == 56);
    static_assert(offsetof(ContexteDemarrage, ReservePages) == 72);
    static_assert(offsetof(ContexteDemarrage, Gdt) == 88);
    static_assert(offsetof(ContexteDemarrage, Idt) == 96);
    static_assert(offsetof(ContexteDemarrage, TablePages) == 104);
    static_assert(offsetof(ContexteDemarrage, PlagesMemoireLibres) == 112);
    static_assert(offsetof(ContexteDemarrage, DerniereException) == 128);
    static_assert(offsetof(ContexteDemarrage, PileNoyau) == 136);
    static_assert(offsetof(ContexteDemarrage, PileIst) == 152);
    static_assert(offsetof(ContexteDemarrage, Tss) == 168);
    static_assert(offsetof(ContexteDemarrage, ApicLocal) == 176);
    static_assert(offsetof(ContexteDemarrage, NombreTicksHorloge) == 200);
    static_assert(offsetof(ContexteDemarrage, DernierCodeClavier) == 208);
    static_assert(offsetof(ContexteDemarrage, GsiHorloge) == 224);
    static_assert(offsetof(ContexteDemarrage, EtapeDemarrage) == 232);
    static_assert(offsetof(ContexteDemarrage, AdresseInstructionException) == 240);
    static_assert(offsetof(ContexteDemarrage, AdresseMemoireException) == 248);
    static_assert(offsetof(ContexteDemarrage, AdresseInstructionInterruption) == 256);
    static_assert(offsetof(ContexteDemarrage, PointeurCadreInterruption) == 264);
    static_assert(offsetof(ContexteDemarrage, SelecteurCodeInterruption) == 272);
    static_assert(offsetof(ContexteDemarrage, DrapeauxInterruption) == 280);
    static_assert(offsetof(ContexteDemarrage, TaillePageMemoire) == 288);
    static_assert(offsetof(ContexteDemarrage, NombrePagesTablesPagination) == 292);
    static_assert(offsetof(ContexteDemarrage, NombrePagesCartographiees) == 296);
    static_assert(offsetof(ContexteDemarrage, NombrePagesRecuperees) == 300);
    static_assert(offsetof(ContexteDemarrage, LimitePagination) == 304);
    static_assert(offsetof(ContexteDemarrage, CapacitePlagesMemoire) == 312);
}
