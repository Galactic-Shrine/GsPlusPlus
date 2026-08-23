<div align="center">

# Gs++

**A bilingual native systems language, from source code to machine code.**

[![Gs++ validation](https://github.com/Galactic-Shrine/GsPlusPlus/actions/workflows/validation.yml/badge.svg)](https://github.com/Galactic-Shrine/GsPlusPlus/actions/workflows/validation.yml)
[![Version](https://img.shields.io/github/v/release/Galactic-Shrine/GsPlusPlus?include_prereleases&label=version)](https://github.com/Galactic-Shrine/GsPlusPlus/releases)
[![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux-5865f2)](#building)
[![MPL-2.0 license](https://img.shields.io/badge/license-MPL--2.0-blue.svg)](LICENSE)

[Français](README.md) · [English](README.en.md)

</div>

## What is Gs++?

Gs++ is a native systems programming language created by
**⋞Galactic-Shrine⋟**. It targets low-level software, system libraries, and
native applications that require explicit control over data, memory, the ABI,
and object lifetimes.

The `gsppc` compiler turns Gs++ sources directly into machine code. Gs++ is not
a C++ transpiler: it has its own frontend, x86-64 code generator, linker, and
GsObj, GsA, and GsE binary formats.

French is the canonical language syntax. Documented English keywords are
official aliases with the same semantics and generated code.

> **Current status — `0.27.0-alpha.3`**
>
> This prerelease can be used to evaluate and develop with the current Gs++
> toolchain. Binary formats 1.0 and ABI 1 are validated, while the self-hosted
> frontend remains in development. The lexer and the AST for functions,
> globals, aggregates, enumerations, and aliases are now written and validated
> in Gs++. Statements, expressions, and class methods remain to be migrated.

## Language principles

| Principle | What Gs++ provides |
|---|---|
| Native compilation | Direct x86-64 machine-code generation |
| Bilingual syntax | Canonical French and equivalent English aliases |
| Systems programming | Pointers, structures, unions, arrays, globals, and atomics |
| Object model | Classes, visibility, single inheritance, virtual methods, constructors, and destructors |
| Explicit lifetimes | Ordered initialization, RAII, and deterministic destruction |
| Separate compilation | Interfaces, GsObj objects, GsA libraries, and link-time ABI checks |
| Execution profiles | Minimal freestanding profile and explicitly linked hosted services |
| Progressive self-hosting | Compiler components rewritten and validated in Gs++ |
| Reproducibility | Versioned formats, link maps, and a portable conformance matrix |

## A first program

```cpp
namespace Shrine::Examples
{
    public int32 Add(int32 left, int32 right)
    {
        return left + right;
    }

    public int32 Main()
    {
        int32 result = Add(20, 22);

        if (result == 42)
        {
            return result;
        }

        return 0;
    }
}
```

The same API can be written with canonical French keywords such as `espace`,
`publique`, `retourner`, `si`, and `sinon`.

## Build pipeline

```text
Sources and interfaces
  .Gs++ / .GsPP / .GsPlusPlus
  .HGs++ / .HGsPP / .HeaderGsPlusPlus
                │
                ▼
              gsppc
                │
                ├── .GsObj  native Gs++ object
                ├── .GsA    native Gs++ library
                └── .GsE    executable Gs++ image
```

Canonical signatures are `GSOBJ:0`, `GSA:0`, and `GSE:0`. All three binary
formats are version 1.0 and their ABI fields are set to 1. The current target
uses the `GsAbi:x64-ms-v1` link signature.

## Extensions

| Purpose | Extensions |
|---|---|
| Sources | `.Gs++`, `.GsPP`, `.GsPlusPlus` |
| Interfaces | `.HGs++`, `.HGsPP`, `.HeaderGsPlusPlus` |
| Projects | `.GsPj`, `.GsProject` |
| Solutions | `.GsPs` |
| Objects | `.GsObj` |
| Libraries | `.GsA` |
| Executables | `.GsE` |

Projects and solutions use a strict XML 1.0 schema:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<GsProject Version="1.0" Name="Hello" Type="executable">
    <Source Path="Hello.GsPlusPlus" />
    <Build Output="Construction/Hello.GsE" />
</GsProject>
```

The equivalent French XML vocabulary uses `GsProjet`, `Source Chemin`, and
`Construction Sortie`.

## Building

### Requirements

- CMake 4.2 or newer on Windows for the Visual Studio 2026 generator;
- CMake 3.20 or newer on Linux;
- a C++20 compiler;
- Python 3 for the conformance suite;
- Ninja, Bash, and common GNU tools for Linux integration tests.

### Windows — Visual Studio 2026

```powershell
cmake --preset windows-release
cmake --build --preset windows-release --target espace_travail
ctest --preset windows-release
```

### Linux — GNU and Ninja

```bash
cmake --preset linux-release
cmake --build --preset linux-release --target espace_travail
ctest --preset linux-release
```

Tools are written to `Construction/.../Bin`, while Gs++ libraries are written
to `Construction/.../Artefacts/GsPlusPlus`.

After a Windows build:

```powershell
Construction/VisualStudio/Release/Bin/gsppc.exe `
  Exemples/Hello.GsPlusPlus `
  --format gsobj `
  -o Hello.GsObj
```

On Linux:

```bash
Construction/Ninja/Release/Bin/gsppc \
  Exemples/Hello.GsPlusPlus \
  --format gsobj \
  -o Hello.GsObj
```

## Downloading a prerelease

The [`0.27.0-alpha.3` release](https://github.com/Galactic-Shrine/GsPlusPlus/releases/tag/v0.27.0-alpha.3)
provides x86-64 packages for Windows and Linux. Each package contains the
tools, SDK headers, Gs++ libraries, examples, and Markdown documentation. Use
`SHA256SUMS.txt` to verify downloads.

## Repository layout

```text
GsPlusPlus/
├── Compiler/          native compiler, linker, and GsE tools
├── SDK/               public format and contract headers
├── Bibliotheques/     system and hosted libraries
├── AutoHebergement/   components written in Gs++
├── Exemples/          introductory programs
├── Tests/             unit, integration, and conformance tests
├── Benchmarks/        reproducible measurements
└── Documentation/     specifications and validation evidence
```

## Documentation

- [Candidate language specification 1.0](Documentation/SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md)
- [Project and solution XML format 1.0](Documentation/FORMAT_PROJETS_GS_PLUS_PLUS_1.0.md)
- [GsObj 1.0](Documentation/FORMAT_GSOBJ_1.0.md), [GsA 1.0](Documentation/FORMAT_GSA_1.0.md), and [GsE 1.0](Documentation/FORMAT_GSE_1.0.md) formats
- [Native x86-64 ABI](Documentation/ABI_GS_PLUS_PLUS_X64_MS_V1.md)
- [Conformance matrix](Documentation/CONFORMITE_GS_PLUS_PLUS_1.0.md)
- [Self-hosted frontend 0.27](Documentation/FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md)
- [`0.27.0-alpha.3` validation](Documentation/Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.3.md)
- [Roadmap](Documentation/FEUILLE_DE_ROUTE_GS_PLUS_PLUS.md)

All normative documentation is maintained in Markdown as its primary source.

## Current validation

- portable conformance: **20/20** on MSVC and GNU;
- CTest: **3/3** on Windows and **4/4** on Linux;
- four successful smoke benchmark scenarios on each host;
- Windows and Linux GitHub CI;
- reproducible `Lexeur.GsE` and `AnalyseurDeclarations.GsE` across both
  validated toolchains.

## License

Gs++ is distributed under the [Mozilla Public License 2.0](LICENSE).
