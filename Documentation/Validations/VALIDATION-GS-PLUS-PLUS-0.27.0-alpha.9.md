# Validation Gs++ 0.27.0-alpha.9

**VALIDÉ — Visual Studio 2026, GNU/Linux et paquets extraits — 20 septembre 2026.**

Cette matrice remplace celle d’alpha.8 dans le dépôt actif ; l’ancienne reste
accessible dans l’historique Git. Elle porte sur la préversion alpha.9, pas sur
un frontend 0.27 complet ni sur Gs++ 1.0 stable.

## Périmètre

- lexeur, AST compact et analyse sémantique auto-hébergés dans `Frontend.GsE` ;
- expressions indirectes, opérateurs, références, affectations et retours ;
- plans de construction, destruction et tables virtuelles ;
- contraintes des globales, constantes numériques, énumérations, données et
  relocalisations de fonctions émises en mémoire ;
- conversions explicites scalaires, signatures, qualificatifs et plages ;
- 257 corpus sémantiques négatifs comparés au bootstrap (code, ligne, colonne),
  dont 54 cas supplémentaires français/anglais pour les conversions ;
- sorties bornées, absence d’écriture partielle pour l’émission des globales,
  sentinelles, déterminisme et équilibre des allocations ;
- formats GsObj/GsA/GsE 1.0, signatures canoniques et ABI machine 1.

## Résultats locaux

| Contrôle | Visual Studio 2026, x64 Release | GNU/Linux WSL, x86-64 Release |
| --- | ---: | ---: |
| CTest | 4/4 | 5/5 |
| Conformité portable | 20/20 | 20/20 |
| Benchmark smoke | 4/4 | 4/4 |
| Vérification de Frontend.GsE | réussi | réussi |

Le test supplémentaire GNU est l’intégration Bash. Les sources sont compilées
par MSVC (MSBuild 18.10.1) et GCC 11.4.0 ; les formats générés conservent le
contrat `GsAbi:x64-ms-v1`. Exécuter les outils sous Linux ne signifie pas
qu’une ABI System V ni une sortie native ELF sont déjà implémentées.

Commandes depuis la racine du dépôt :

```powershell
cmake --build --preset windows-release --target espace_travail --parallel
ctest --preset windows-release
pwsh.exe -NoProfile -ExecutionPolicy Bypass -File Benchmarks/Invoke-GsPlusPlusBenchmark.ps1 -Mode smoke
```

```bash
cmake --build --preset linux-release --target espace_travail --parallel 4
ctest --preset linux-release
bash Benchmarks/invoke-gsplusplus-benchmark.sh --mode smoke
```

Sessions smoke (un contrôle fonctionnel, pas une campagne de performances) :

- Windows : `20260920T140004.189547Z-6bb52e8c` ;
- GNU/Linux : `20260920T140006.931516Z-6decb2d4`.

## Image et ABI

Les deux images `Frontend.GsE` sont identiques bit à bit :

- taille : **334 318 octets** ;
- format GsE 1.0, ABI 1, trois segments, huit sections ;
- **75 exports**, deux imports d’hôte : allocation et libération ;
- SHA-256 :
  `aef86685f1444466f78951725e0b08cbfe81f70dcb9c9ba5f38eb69cc1597c95`.

L’AST occupe toujours 64 octets par nœud et la requête d’analyse sémantique
120 octets. La requête distincte d’émission des globales occupe 136 octets ;
ses descripteurs et relocalisations occupent respectivement 48 et 32 octets.
Les diagnostics de conversion 94–99 n’altèrent aucun de ces contrats.

## Paquets

Les paquets de destination sont :

- `GsPlusPlus-0.27.0-alpha.9-Windows-x86_64.zip` ;
- `GsPlusPlus-0.27.0-alpha.9-Linux-x86_64.tar.gz`.

Ils sont produits par CPack, puis extraits dans un répertoire neuf. Les
contrôles de distribution portent sur la version des outils, la vérification
du frontend livré, la compilation de `Bonjour.Gs++`, la compilation avec les
interfaces publiques livrées, puis la compilation et l’exécution d’un petit
programme GsE utilisant des conversions.

Ces contrôles passent sur les deux paquets : les outils annoncent alpha.9,
le programme retourne **42** et le pilote différentiel exécute avec succès
le `Frontend.GsE` extrait de chaque archive. Les paquets définitifs sont
reconstruits depuis le commit de publication propre, puis contrôlés à nouveau.

Les interfaces du frontend sont installées sous
`share/GsPlusPlus/AutoHebergement`, en plus des interfaces des bibliothèques.
La documentation et les notes bilingues sont incluses dans les archives.

Le manifeste externe `SHA256SUMS.txt` est généré après le dernier paquetage,
afin d’éviter une référence circulaire dans ce document inclus dans les
archives. Les livrables sont conservés sous
`Construction/GsPlusPlus-0.27.0-alpha.9/Packages/`.

## Limites conservées

- frontend encore partiel : conversions implicites composées, qualifications
  et autres familles sémantiques à compléter ;
- données globales produites en mémoire, pas encore de fichier objet écrit
  par un backend auto-hébergé ;
- génération de code machine, formats et liaison encore assurés par le bootstrap ;
- cible actuelle x86-64 / `GsAbi:x64-ms-v1` ; PE, ELF, SDK de destination et
  matrice de compilation croisée restent prévus dans les jalons suivants.
