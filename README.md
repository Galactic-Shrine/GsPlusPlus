# Gs++

[![Validation Gs++](https://github.com/Galactic-Shrine/GsPlusPlus/actions/workflows/validation.yml/badge.svg)](https://github.com/Galactic-Shrine/GsPlusPlus/actions/workflows/validation.yml)
[![Licence MPL-2.0](https://img.shields.io/badge/licence-MPL--2.0-blue.svg)](LICENSE)

Ce projet contient le langage Gs++ indépendamment de Sanctuaire SE. Sa
construction, ses tests et ses paquets publics n’utilisent pas `Noyau.GsE`.

- `Compiler/` : compilateur natif et outils `.GsE` ;
- `SDK/` : formats et contrats partagés ;
- `Bibliotheques/` : bibliothèques système et hébergée ;
- `AutoHebergement/` : composants écrits en Gs++ ;
- `Exemples/` : exemples du langage ;
- `Tests/` : tests unitaires et d’intégration ;
- `Benchmarks/` : protocole exécutable et scripts de mesure reproductibles ;
- `Documentation/` : spécifications et validations historiques.

Gs++ 0.27.0-alpha.1 est une préversion publique destinée aux essais. Elle
conserve le profil hébergé 0.26 nécessaire à la future migration de son
compilateur : chaînes UTF-8 propriétaires, conteneurs dynamiques, arène stable,
table de symboles, chemins et fichiers alloués. Ces services utilisent cinq
imports d’hôte explicites et un modèle d’erreur sans exception ;
`GsSysteme.GsA` reste indépendant de ces imports. Les formats GsObj/GsA/GsE
restent en 1.0 et l’ABI reste à 1. La conformité portable compte désormais
vingt exigences. Le lexeur auto-hébergé 0.27 est présent, mais le frontal
auto-hébergé complet reste en développement : cette version est donc une alpha,
pas une version 1.0 prête pour la production.

## Construire Gs++ seul

Prérequis : CMake 3.20 ou plus récent, un compilateur C++20 et Python 3 pour la
suite de conformité. Sous Linux, Bash et les outils GNU usuels sont nécessaires
au test d’intégration.

Sous Windows avec Visual Studio 2022 :

```powershell
cmake --preset windows-release
cmake --build --preset windows-release --target espace_travail
ctest --preset windows-release
```

Sous Linux ou WSL avec Ninja :

```bash
cmake --preset linux-release
cmake --build --preset linux-release --target espace_travail
ctest --preset linux-release
```

Les exécutables sont produits dans `Construction/.../Bin` et les bibliothèques
Gs++ dans `Construction/.../Artefacts/GsPlusPlus`. Aucune arborescence
`SanctuaireSE` n’est configurée par ces commandes.

La référence stable servant de base à cette alpha est
[`Documentation/REFERENCE-GS-PLUS-PLUS-0.26.md`](Documentation/REFERENCE-GS-PLUS-PLUS-0.26.md),
le contrat consolidé est
[`Documentation/SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md`](Documentation/SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md)
et la matrice exécutable est décrite dans
[`Documentation/CONFORMITE_GS_PLUS_PLUS_1.0.md`](Documentation/CONFORMITE_GS_PLUS_PLUS_1.0.md).
Le schéma XML normatif des `.GsPj`, `.GsProject` et `.GsPs` est
[`Documentation/FORMAT_PROJETS_GS_PLUS_PLUS_1.0.md`](Documentation/FORMAT_PROJETS_GS_PLUS_PLUS_1.0.md).
Le protocole de performance, qui ne revendique encore aucun avantage sur C++
ou Rust, est
[`Documentation/PROTOCOLE-BENCHMARK-GS-PLUS-PLUS-0.25.md`](Documentation/PROTOCOLE-BENCHMARK-GS-PLUS-PLUS-0.25.md),
dont les règles restent applicables au pilote 0.26.

Les sources Gs++ utilisent `.Gs++`, `.GsPP` ou `.GsPlusPlus` et les interfaces
`.HGs++`, `.HGsPP` ou `.HeaderGsPlusPlus`. `.GsPH`, `.GsO`, `.GsPPH` et
`.GsPlusPlusHeader` sont refusées comme extensions obsolètes. Les extensions
Gs# sont réservées à leur propre compilateur et n’ont pas de fichiers
d’en-tête. Les projets `.GsPj`/`.GsProject` et solutions `.GsPs` utilisent XML
1.0.

La base binaire canonique utilise les signatures `GSOBJ:0`, `GSA:0` et
`GSE:0`, les formats GsObj/GsA/GsE 1.0 et l’ABI
`GsAbi:x64-ms-v1` avec les champs ABI à 1. Les artefacts produits avec les
anciennes numérotations locales doivent être reconstruits.

Toute la documentation normative est maintenue d’abord en Markdown. Les
éventuels fichiers `.docx` sont des copies secondaires de consultation et
peuvent être régénérés ultérieurement depuis les `.md`.
