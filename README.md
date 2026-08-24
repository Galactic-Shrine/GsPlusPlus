<div align="center">

# Gs++

**Un langage système natif bilingue, du code source au code machine.**

[![Validation Gs++](https://github.com/Galactic-Shrine/GsPlusPlus/actions/workflows/validation.yml/badge.svg)](https://github.com/Galactic-Shrine/GsPlusPlus/actions/workflows/validation.yml)
[![Version](https://img.shields.io/github/v/release/Galactic-Shrine/GsPlusPlus?include_prereleases&label=version)](https://github.com/Galactic-Shrine/GsPlusPlus/releases)
[![Plateformes](https://img.shields.io/badge/plateformes-Windows%20%7C%20Linux-5865f2)](#construction)
[![Licence MPL-2.0](https://img.shields.io/badge/licence-MPL--2.0-blue.svg)](LICENSE)

[Français](README.md) · [English](README.en.md)

</div>

## Qu’est-ce que Gs++ ?

Gs++ est un langage de programmation système natif créé par
**⋞Galactic-Shrine⋟**. Il est destiné aux logiciels proches du matériel, aux
bibliothèques système et aux applications natives qui demandent une maîtrise
explicite des données, de la mémoire, de l’ABI et de la durée de vie des objets.

Le compilateur `gsppc` transforme directement les sources Gs++ en code machine.
Gs++ n’est pas un transpileur vers C++ : il possède son propre frontend, son
générateur x86-64, son éditeur de liens et ses formats binaires GsObj, GsA et
GsE.

Le français est la syntaxe canonique du langage. Les mots-clés anglais
documentés sont des alias officiels avec la même sémantique et la même
génération de code.

> **État actuel — `0.27.0-alpha.4`**
>
> Cette préversion permet d’évaluer et de développer avec la chaîne Gs++
> actuelle. Les formats binaires 1.0 et l’ABI 1 sont validés, mais le frontend
> auto-hébergé reste en développement. Le lexeur et l’AST des déclarations,
> y compris les méthodes, constructeurs, destructeurs et opérateurs de classes,
> sont désormais écrits et validés en Gs++. Les instructions et expressions
> restent à migrer.

## Principes du langage

| Principe | Ce que Gs++ fournit |
|---|---|
| Compilation native | Production directe de code machine x86-64 |
| Syntaxe bilingue | Français canonique et alias anglais équivalents |
| Programmation système | Pointeurs, structures, unions, tableaux, globales et atomiques |
| Modèle objet | Classes, visibilité, héritage simple, virtualité, constructeurs et destructeurs |
| Durée de vie explicite | Initialisation ordonnée, RAII et destruction déterministe |
| Compilation séparée | Interfaces, objets GsObj, bibliothèques GsA et contrôle ABI à la liaison |
| Profils d’exécution | Profil freestanding minimal et services hébergés explicitement liés |
| Auto-hébergement progressif | Composants du compilateur réécrits et validés en Gs++ |
| Reproductibilité | Formats versionnés, cartes de liens et matrice de conformité portable |

## Un premier programme

```cpp
espace Shrine::Exemples
{
    publique entier32 Additionner(entier32 gauche, entier32 droite)
    {
        retourner gauche + droite;
    }

    publique entier32 Principal()
    {
        entier32 résultat = Additionner(20, 22);

        si (résultat == 42)
        {
            retourner résultat;
        }

        retourner 0;
    }
}
```

La même API peut être écrite avec les alias anglais tels que `namespace`,
`public`, `return`, `if` et `else`.

## Chaîne de production

```text
Sources et interfaces
  .Gs++ / .GsPP / .GsPlusPlus
  .HGs++ / .HGsPP / .HeaderGsPlusPlus
                │
                ▼
              gsppc
                │
                ├── .GsObj  objet natif Gs++
                ├── .GsA    bibliothèque native Gs++
                └── .GsE    image exécutable Gs++
```

Les signatures canoniques sont `GSOBJ:0`, `GSA:0` et `GSE:0`. Les trois
formats binaires sont en version 1.0 et leurs champs ABI valent 1. La cible
actuelle utilise la signature de liaison `GsAbi:x64-ms-v1`.

## Extensions

| Usage | Extensions |
|---|---|
| Sources | `.Gs++`, `.GsPP`, `.GsPlusPlus` |
| Interfaces | `.HGs++`, `.HGsPP`, `.HeaderGsPlusPlus` |
| Projets | `.GsPj`, `.GsProject` |
| Solutions | `.GsPs` |
| Objets | `.GsObj` |
| Bibliothèques | `.GsA` |
| Exécutables | `.GsE` |

Les projets et solutions utilisent un schéma XML strict en version 1.0 :

```xml
<?xml version="1.0" encoding="UTF-8"?>
<GsProjet Version="1.0" Nom="Bonjour" Type="executable">
    <Source Chemin="Bonjour.Gs++" />
    <Construction Sortie="Construction/Bonjour.GsE" />
</GsProjet>
```

Le vocabulaire XML anglais équivalent utilise `GsProject`, `Source Path` et
`Build Output`.

## Construction

### Prérequis

- CMake 4.2 ou plus récent sous Windows pour le générateur Visual Studio 2026 ;
- CMake 3.20 ou plus récent sous Linux ;
- un compilateur C++20 ;
- Python 3 pour la conformité ;
- Ninja, Bash et les outils GNU usuels pour l’intégration Linux.

### Windows — Visual Studio 2026

```powershell
cmake --preset windows-release
cmake --build --preset windows-release --target espace_travail
ctest --preset windows-release
```

### Linux — GNU et Ninja

```bash
cmake --preset linux-release
cmake --build --preset linux-release --target espace_travail
ctest --preset linux-release
```

Les outils sont placés dans `Construction/.../Bin` et les bibliothèques Gs++
dans `Construction/.../Artefacts/GsPlusPlus`.

Après une construction Windows :

```powershell
Construction/VisualStudio/Release/Bin/gsppc.exe `
  Exemples/Bonjour.Gs++ `
  --format gsobj `
  -o Bonjour.GsObj
```

Sous Linux :

```bash
Construction/Ninja/Release/Bin/gsppc \
  Exemples/Bonjour.Gs++ \
  --format gsobj \
  -o Bonjour.GsObj
```

## Télécharger une préversion

La [release `0.27.0-alpha.4`](https://github.com/Galactic-Shrine/GsPlusPlus/releases/tag/v0.27.0-alpha.4)
propose des paquets x86-64 pour Windows et Linux. Chaque paquet contient les
outils, les en-têtes SDK, les bibliothèques Gs++, les exemples et la
documentation Markdown. Le fichier `SHA256SUMS.txt` permet de vérifier les
téléchargements.

## Organisation du dépôt

```text
GsPlusPlus/
├── Compiler/          compilateur natif, éditeur de liens et outils GsE
├── SDK/               en-têtes des formats et contrats publics
├── Bibliotheques/     bibliothèques système et hébergée
├── AutoHebergement/   composants écrits en Gs++
├── Exemples/          programmes de découverte
├── Tests/             tests unitaires, intégration et conformité
├── Benchmarks/        mesures reproductibles
└── Documentation/     spécifications et preuves de validation
```

## Documentation

- [Spécification candidate du langage 1.0](Documentation/SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md)
- [Format XML des projets et solutions 1.0](Documentation/FORMAT_PROJETS_GS_PLUS_PLUS_1.0.md)
- [Formats GsObj 1.0](Documentation/FORMAT_GSOBJ_1.0.md), [GsA 1.0](Documentation/FORMAT_GSA_1.0.md) et [GsE 1.0](Documentation/FORMAT_GSE_1.0.md)
- [ABI native x86-64](Documentation/ABI_GS_PLUS_PLUS_X64_MS_V1.md)
- [Matrice de conformité](Documentation/CONFORMITE_GS_PLUS_PLUS_1.0.md)
- [Frontend auto-hébergé 0.27](Documentation/FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md)
- [Validation de `0.27.0-alpha.4`](Documentation/Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.4.md)
- [Feuille de route](Documentation/FEUILLE_DE_ROUTE_GS_PLUS_PLUS.md)

Toute la documentation normative est maintenue en Markdown comme source
principale.

## Validation actuelle

- conformité portable : **20/20** sous MSVC et GNU ;
- CTest : **3/3** sous Windows et **4/4** sous Linux ;
- quatre scénarios de benchmark smoke réussis sur chaque hôte ;
- CI GitHub Windows et Linux ;
- `Lexeur.GsE` et `AnalyseurDeclarations.GsE` reproductibles entre les deux
  chaînes validées.

## Licence

Gs++ est distribué sous la [Mozilla Public License 2.0](LICENSE).
