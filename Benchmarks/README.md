# Banc de mesure Gs++ 0.27.0-alpha.8

Ce dossier contient le pilote pratique du protocole
[`PROTOCOLE-BENCHMARK-GS-PLUS-PLUS-0.25.md`](../Documentation/PROTOCOLE-BENCHMARK-GS-PLUS-PLUS-0.25.md).

Le pilote utilise seulement des sources et interfaces déjà couvertes par les
tests d'intégration. Il les copie dans une session sous `Construction/` avant
toute modification neutre. Il ne supprime ni ne modifie les fichiers du dépôt.
Il dépend uniquement de la bibliothèque standard de Python 3.10 ou plus récent.

## Exécution rapide

Sous Windows :

```powershell
pwsh.exe -NoProfile -ExecutionPolicy Bypass -File `
  Benchmarks/Invoke-GsPlusPlusBenchmark.ps1 `
  -Mode smoke
```

Sous GNU/Linux ou WSL :

```bash
Benchmarks/invoke-gsplusplus-benchmark.sh --mode smoke
```

Depuis la racine autonome de Gs++, le pilote localise par défaut les outils
dans les constructions permanentes de publication :

- Windows : `../Construction/GsPlusPlus-Development/VisualStudio/Release/Bin/` ;
- GNU/Linux : `../Construction/GsPlusPlus-Development/Ninja/Release/Bin/`.

Un autre chemin peut être fourni avec `--compiler` et `--loader`, ou avec les
paramètres PowerShell `-Compiler` et `-Loader`. Une version différente de
0.27.0-alpha.8 est refusée par défaut.

## Modes

| Mode | Conditions | Répétitions | Échauffements | Usage |
| --- | --- | ---: | ---: | --- |
| `smoke` | `cold_artifacts` | 1 | 0 | contrôle fonctionnel du banc |
| `pilot` | toutes | 5 | 1 | estimation initiale de la variance |
| `full` | toutes | 30 | 3 | campagne permettant une analyse statistique |

Les scénarios et conditions peuvent être filtrés, par exemple :

```powershell
pwsh.exe -NoProfile -ExecutionPolicy Bypass -File `
  Benchmarks/Invoke-GsPlusPlusBenchmark.ps1 `
  -Mode pilot `
  -Scenario m_separate_gse `
  -Condition cold_artifacts,leaf_edit,interface_edit `
  -Cpu 2
```

```bash
Benchmarks/invoke-gsplusplus-benchmark.sh \
  --mode pilot \
  --scenario m_separate_gse \
  --condition cold_artifacts \
  --condition leaf_edit \
  --condition interface_edit \
  --cpu 2
```

## Résultats

Chaque invocation crée une nouvelle session sans écraser les précédentes :

```text
../Construction/Benchmarks/GsPlusPlus/<plateforme>/<session>/
├── session.json
├── results.jsonl
├── summary.json
├── summary.csv
├── status.json
└── logs/
```

`session.json` contient les versions et empreintes des outils, l'hôte, les
sélections et les empreintes du corpus. `results.jsonl` conserve les mesures
brutes par étape et par pipeline. `summary.json` et `summary.csv` présentent la
médiane, l'IQR, la MAD et l'intervalle de confiance bootstrap de la médiane.

Les copies de travail sont supprimées après une session réussie seulement.
`--keep-work`/`-KeepWork` permet de les conserver. En cas d'échec, elles restent
toujours disponibles pour le diagnostic.

## Limites à respecter

- `cold_artifacts` signifie « sans artefact préexistant » ; le cache de fichiers
  du système d'exploitation n'est pas vidé.
- `warm_artifacts` répète la construction complète avec les artefacts présents ;
  ce n'est pas la preuve d'un cache interne à `gsppc`.
- les éditions incrémentales utilisent une copie et ajoutent seulement un
  commentaire neutre ;
- la sélection manuelle d'étapes du scénario séparé décrit le plan de mesure,
  pas un système automatique de dépendances du compilateur ;
- Gs++ 0.27.0-alpha.8 ne fournit pas de chronométrage séparé du lexeur, du parseur, de
  l'analyse sémantique, de la génération de code et de la liaison ;
- ce banc ne conclut pas que Gs++ est plus rapide que C++20 ou Rust.
