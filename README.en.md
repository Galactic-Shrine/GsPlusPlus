<div align="center">

<img src="Assets/Gs++.png" alt="Logo Gs++" width="200"/>

**A bilingual native systems language, from source code to machine code.**

[![Gs++ validation](https://github.com/Galactic-Shrine/GsPlusPlus/actions/workflows/validation.yml/badge.svg)](https://github.com/Galactic-Shrine/GsPlusPlus/actions/workflows/validation.yml)
[![GitHub release](https://img.shields.io/github/v/release/Galactic-Shrine/GsPlusPlus?include_prereleases&label=release)](https://github.com/Galactic-Shrine/GsPlusPlus/releases)
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

> **Current source status — `0.27.0-alpha.10`**
>
> This prerelease can be used to evaluate and develop with the current Gs++
> toolchain. Binary formats 1.0 and ABI 1 are validated, while the self-hosted
> frontend remains in development. Its single `Frontend.GsE` image combines
> keyword classification, lexing, the compact AST, and semantic analysis. The
> semantic stage now covers resolvable overloads, members, constructors,
> initializers, nested aggregates, indexing, addresses, dereferences, and
> indirect calls. Development after alpha.8 also validates the lvalues,
> indices, targets, arities, types, and references of those forms, together
> with typed resolution for binary and unary free operators. All twenty-four
> unary and binary intrinsic forms are now validated as well, including literal
> adaptation, ordinary and function pointers, and nine diagnostic families.
> The stage also emits ordered construction, destruction, and virtual-table
> plans for local objects and constructor subobjects. It also enforces global
> declaration constraints for class objects, references, `void` types, public
> imports, and uninitialized constants. Global initializers are now checked
> recursively as well: aggregates require lists, function pointers require a
> direct function target, initialized data pointers are rejected, and scalar
> leaves must have a structurally constant form. It now evaluates numeric
> constants, including implicit and explicit enumerator values, signed and
> unsigned operations, conversions, and logical short-circuiting; division by
> zero and out-of-range values are rejected. The `EmitGlobals` API now produces
> initial bytes, data/zero layouts, and function relocations, compared with the
> bootstrap. Alpha.9 also checks explicit casts, function signatures, and
> out-of-range constant conversions. Alpha.10 extends implicit
> adaptation to compound constants, overload selection and related qualifiers.
> It adds nested callback signatures, context-aware named types, signature
> constraints, field aliases and root type, function and global aliases,
> then calls through method aliases with an explicit `Class&` receiver.
> The remaining implicit conversions and
> other semantic families
> still need to be migrated; this API does not yet replace the backend or
> object-file writers.

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

Gs++ APIs use the canonical `GalacticShrine::GsPP::` namespace prefix. For
example, hosted services are exposed under `GalacticShrine::GsPP::Hebergee`,
while host-provided imports use `GalacticShrine::GsPP::Hote`.

## A first program

```cpp
namespace Shrine::Examples {

    /**
     * <summary>Adds two signed 32-bit integers.</summary>
     * @Parameter(int32: left) First value.
     * @Parameter(int32: right) Second value.
     * @Returns(int32) Sum of both values.
     **/
    public int32 Add(int32 left, int32 right) {

        return left + right;
    }

    /**
     * <summary>Runs the example program.</summary>
     * @Returns(int32) Program result.
     **/
    public int32 Main() {

        int32 result = Add(20, 22);

        if (result == 42) {
            return result;
        }

        return 0;
    }
}
```

The same API can be written with canonical French keywords such as `espace`,
`publique`, `retourner`, `si`, and `sinon`.

### Include files and use their names

Development sources after alpha.10 also support:

```cpp
#include "Types.HGsPP"
using namespace GalacticShrine::GsPP::Types;
```

The French forms are `#inclure "Types.HGsPP"` and
`utilisant espace GalacticShrine::GsPP::Types;`. Including a file inserts its
declarations; using a namespace makes names available without their prefix.
`#pragma once` guards repeated inclusion. XML projects still select compiled
sources and linked libraries.

This addition works in the `gsppc` bootstrap and has a
[runnable bilingual example](Exemples/Directives/Application.GsPj).
The self-hosted analyzers also parse namespace using directives and resolve
their names, aliases and overload sets. Included files are still expanded by
the host bootstrap; this is not a full C++ preprocessor. The
[current rules and limitations](Documentation/SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md#inclusion-textuelle-et-utilisation-despaces-de-noms)
describe the supported forms. Published alpha.10 packages remain unchanged.

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

### Planned multi-target compilation

The product plan calls for a native target by default, with an explicit choice
of another system. The first targets are Windows, GNU/Linux, and the native
Galactic-Shrine platform on x86-64. The toolchain must produce the destination's
executable format and use its linking conventions and SDK; cross-compilation
depends on their availability.

Target selection and native Windows PE (`.exe`) and Linux ELF output are
**planned, not yet implemented**. Current Windows/Linux builds validate the
compiler on those hosts and the existing Gs++ contract.
The [product plan](Documentation/PLAN_PRODUIT_GS_PLUS_PLUS_1.0.md) describes this
work and its acceptance criteria.

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

Planned for **0.28.0**: `.Glib` will replace `.GsA` for static libraries;
`.GdLib` is reserved for dynamic libraries if that support is introduced.
`.GsE` remains unchanged. The 0.27 toolchain still uses `.GsA`.

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

Structures and enumerations can each have their own `.HGsPP` interface,
explicitly listed in the project and supplied to every separately compiled
source. The [TypesParFichier example](Exemples/TypesParFichier/Application.GsPj)
separates `Point`, `Etat` and their use; its entry point returns 42.
A file's presence in the directory does not automatically expose its types.

## Building

### Requirements

For the native solution, install Visual Studio 2026 with MSVC v145 C++ tools
and the Windows SDK. **CMake is not required for this mode.**
The following requirements apply to the CMake build:

- CMake 4.2 or newer on Windows for the Visual Studio 2026 generator;
- CMake 3.20 or newer on Linux;
- a C++20 compiler;
- Python 3 for the conformance suite;
- Ninja, Bash, and common GNU tools for Linux integration tests.

### Windows — native Visual Studio 2026 solution, without CMake

Open [`GsPlusPlus.slnx`](GsPlusPlus.slnx) and build `Release | x64`.
From a Visual Studio developer terminal:

```powershell
msbuild GsPlusPlus.slnx /m /p:Configuration=Release /p:Platform=x64
msbuild VisualStudio/Validation.vcxproj /m /p:Configuration=Release /p:Platform=x64
```

Outputs are under `Construction/MSBuild/x64/Release/`.
See the [Visual Studio guide](VisualStudio/README.md) for targets and tests.

### Windows — CMake with Visual Studio 2026

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

Local CMake output is ignored by Git and stays under
`Construction/CMake/...`. Tools are written to its `Bin`
subdirectory, while Gs++ libraries are written to `Artefacts/GsPlusPlus`.

The root [`VERSION`](VERSION) file is the single technical source of truth for
the product version. CMake and MSBuild propagate it to tools and GsE metadata;
tests, benchmarks and CMake package names also use this file.

After a Windows build:

```powershell
./Construction/CMake/VisualStudio/Release/Bin/gsppc.exe `
  Exemples/Hello.GsPlusPlus `
  --format gsobj `
  -o Hello.GsObj
```

On Linux:

```bash
./Construction/CMake/Ninja/Release/Bin/gsppc \
  Exemples/Hello.GsPlusPlus \
  --format gsobj \
  -o Hello.GsObj
```

## Downloading a prerelease

The [`0.27.0-alpha.10` release](https://github.com/Galactic-Shrine/GsPlusPlus/releases/tag/v0.27.0-alpha.10)
provides x86-64 packages for Windows and Linux. Each package contains the
tools, SDK headers, Gs++ libraries, examples, and Markdown documentation. Use
`SHA256SUMS.txt` to verify downloads.

Source and extracted-package validation is recorded in the
[alpha.10 matrix](Documentation/Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.10.md).

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

- [Current documentation index](Documentation/README.md)
- [Latest release notes](RELEASE_NOTES.md)
- [Candidate language specification 1.0](Documentation/SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md)
- [Gs++ 1.0 source-code conventions](Documentation/CONVENTIONS_CODE_GS_PLUS_PLUS_1.0.md)
- [Project and solution XML format 1.0](Documentation/FORMAT_PROJETS_GS_PLUS_PLUS_1.0.md)
- [GsObj 1.0](Documentation/FORMAT_GSOBJ_1.0.md), [GsA 1.0](Documentation/FORMAT_GSA_1.0.md), and [GsE 1.0](Documentation/FORMAT_GSE_1.0.md) formats
- [Native x86-64 ABI](Documentation/ABI_GS_PLUS_PLUS_X64_MS_V1.md)
- [Conformance matrix](Documentation/CONFORMITE_GS_PLUS_PLUS_1.0.md)
- [Self-hosted frontend 0.27](Documentation/FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md)
- [`0.27.0-alpha.10` validation](Documentation/Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.10.md)
- [Historical alpha.9 publication validation](Documentation/Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.9.md)
- [Roadmap](Documentation/FEUILLE_DE_ROUTE_GS_PLUS_PLUS.md)

All normative documentation is maintained in Markdown as its primary source.

## Current validation

- portable conformance: **20/20** on MSVC and GNU;
- alpha.10 source CTest: **5/5** on Windows and **6/6** on Linux
  (**4/4** and **5/5** for the published alpha.9 tranche);
- four successful smoke benchmark scenarios on each host;
- Windows and Linux GitHub CI;
- the single self-hosted `Frontend.GsE` image, which combines all four frontend
  stages, is compared across both validated toolchains;
- differential recursive typing for indexing, `&`, `*`, and indirect calls,
  including nested callbacks;
- differential selection of free operators, including overloads, qualified
  references, ambiguities, arities, and unary operators;
- differential validation of all twenty-four intrinsic operators, with literal
  adaptation;
- qualified reference bindings, inheritance conversions, assignments, and
  returns aligned with the bootstrap compiler;
- structural and numeric constraints for declarations, enumerations, and global
  initializers and explicit casts aligned with the bootstrap compiler,
  with **1,957 negative corpora** in development sources (**619** in the published
  alpha.10), whose code, line, and column are checked;
- callback references, nested signatures and deeply indirect pointer arrays
  covered by development differential tests;
- context-aware named types in signatures, distinct same-name types, calls
  through callback fields and preservation of the caller's input AST;
- function and callback signature constraints, parameter limits including
  implicit receivers, and free operators on structures/unions;
- canonical field-alias chains, including validation of unused aliases,
  cycles and unknown targets;
- canonical type, free-function and global aliases, including forward
  declarations, chains and qualified names, with rejection of unknown targets,
  cycles and ambiguous overloaded-function targets;
- calls through unbound method aliases with an explicit `Class&` receiver,
  argument checking and direct-call visibility checks, callback signatures and
  relocations targeting the canonical method;
- inheritance declarations and canonical bases, rejecting unknown, non-class
  or non-public bases, self-inheritance and indirect cycles, locally validated
  after alpha.10;
- virtual method, destructor and operator overrides, using canonical signatures
  and base-before-derived ordering; polymorphic object-array layouts compared
  with the bootstrap within the tested scope;
- reject repeated overload declarations using canonical parameters, including
  type aliases, methods, constructors, destructors and free or member operators;
  a different return type alone does not distinguish an overload;
- compare unbound signatures with the implicit receiver as their first parameter,
  including collisions between a method and a free function with the same
  fully qualified name in a namespace sharing the class name;
- reject computed link-symbol collisions between distinct overloads, with
  canonical types, aliases, callbacks, implicit receivers and qualified or UTF-8
  namespaces; bilingual diagnostic 119 aligned with the bootstrap;
- select qualified, object and pointer calls in groups mixing methods and free
  functions with the same fully qualified name; compare the selected declaration,
  return type and flags, checking visibility after selection and preserving
  ambiguity diagnostics;
- select unary and binary operators from mixed groups, including receiver,
  constness, references, inheritance, hiding and visibility;
- deterministic invalid-overload-group priority by first declaration, followed
  by link collisions and function bodies in source order, verified for the
  covered independent cases;
- priority across successive statements, nested blocks, condition expressions,
  branches and loops within one body;
- operand priority, indexed object before index, assignment target before value,
  and cast target type before source, within the differential test scope;
- call arguments in source order, with prior indirect-target and arity checks,
  rejection of groups without an acceptable arity and receiver, deferred
  selection and nested calls; discard candidates with an incompatible prefix
  in declaration order, within the tested scope, before visiting the next
  argument; report cast-type errors only when their expression is visited;
- analyze aggregate arguments using the selected signature's expected type,
  after selection and visibility for direct groups, in argument order for
  callbacks; scalar forms, structures, unions, nested field arrays and nested
  calls covered within the differential matrix;
- evaluate and range-check constant casts when visited, before later errors,
  preserving prior arity and candidate-abandonment checks; compare contextual
  aggregates and short-circuit behavior against the bootstrap;
- local initializers with shape and capacity checks before their elements,
  leaves analyzed in order and rejection before the next statement; references,
  callbacks and numeric ranges covered within the differential matrix;
- local constructions selected and planned before the next statement, including
  arity, candidate elimination, visibility and contextual aggregate arguments;
  constructor and destructor checks for objects, arrays, bases and subobjects
  compared with the bootstrap within the tested scope;
- recursive base and field constructions checked before the next initializer,
  without publishing extra steps; final plans retain canonical order and reuse
  selected field constructors, including `super()` for bases without their own
  constructor within the differential matrix;
- fully validate each global initializer in source order, typing all its leaves
  before its constant-value pass; structural default-field check priority
  covered within the differential test scope;
- self-hosted global data and function relocation emission, comparing bytes,
  alignments, targets, and caller-buffer bounds against the bootstrap.

## License

Gs++ is distributed under the [Mozilla Public License 2.0](LICENSE).
