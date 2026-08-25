# Validation Gs++ 0.27.0-alpha.7

**VALIDÉ — Visual Studio 2026 et GNU/Linux — 25 août 2026.**

Cette preuve porte sur la tranche `0.27.0-alpha.7` : première passe sémantique
auto-hébergée, préfixe d’espace de noms `GalacticShrine::GsPP::` et convention
des commentaires de bloc multilignes `/** … **/`. Elle ne déclare ni le
frontend 0.27 complet ni Gs++ 1.0 stables.

## Périmètre vérifié

- compilation native du compilateur, du chargeur et du vérificateur ;
- construction des bibliothèques `GsSysteme.GsA` et `GsHebergee.GsA` ;
- construction et exécution réelle des quatre images auto-hébergées ;
- comparaison différentielle du lexeur, de l’AST et de la première passe
  sémantique avec le bootstrap C++ ;
- formats GsObj/GsA/GsE 1.0 et ABI 1 ;
- matrice de conformité portable 20/20 ;
- intégration GNU/Bash ;
- benchmark smoke à quatre scénarios ;
- reproductibilité des images auto-hébergées entre les deux chaînes ;
- création, extraction et utilisation des paquets Windows et Linux.

## Environnements

| Chaîne | Environnement | Résultat CTest |
| --- | --- | ---: |
| MSVC | Visual Studio 2026, SDK Windows 10.0.26100.0, x64 Release | 3/3 |
| GNU | Ubuntu WSL, GCC 11.4.0, Ninja, x86-64 Release | 4/4 |

La quatrième épreuve GNU est l’intégration Bash, volontairement absente de la
matrice Windows.

## Première passe sémantique

`AnalyseurSemantique.GsE` consomme l’AST compact sans le modifier. Il indexe
les types, fonctions, globales, alias, champs, alias de champs, énumérateurs,
paramètres et variables locales, puis résout les références couvertes dans les
portées lexicales, les noms qualifiés, les récepteurs `soi` / `this` et
`parent` / `super`, et les groupes de surcharges appelés.

Le test différentiel valide deux corpus bilingues et quinze corpus négatifs.
Pour chaque refus, le code, la ligne et la colonne sont comparés au bootstrap.
Les interrogations de capacité, les deux sorties partielles, les requêtes
invalides, un AST invalide et l’équilibre exact de l’arène sont également
contrôlés.

| Contrat ABI | Taille |
| --- | ---: |
| `SymboleSemantique` | 48 octets |
| `ResolutionSemantique` | 32 octets |
| `ResultatAnalyseSemantique` | 56 octets |
| `RequeteAnalyseSemantique` | 120 octets |

Le vérificateur décrit l’image finale comme GsE 1.0, ABI 1, deux segments,
sept sections, deux imports et trente-cinq exports. Les deux imports sont
`GalacticShrine::GsPP::Hote::AllouerMemoire` et
`GalacticShrine::GsPP::Hote::LibererMemoire`.

## Namespace et commentaires

Les sources, interfaces, bibliothèques, composants auto-hébergés, points
d’entrée, intrinsics et tests actifs utilisent le préfixe canonique
`GalacticShrine::GsPP::`. Aucun alias de compatibilité `Gs::` n’est conservé ;
les anciennes preuves et notes de version restent inchangées comme archives.
La conformité vérifie notamment les cinq imports hébergés sous
`GalacticShrine::GsPP::Hote` et l’absence d’import hôte dans `GsSysteme.GsA`.

Les commentaires de bloc multilignes du code Gs++ actif suivent la forme :

```text
/**
 * texte
 **/
```

Cette écriture est incluse dans le corpus différentiel et acceptée avec les
mêmes jetons par les lexeurs bootstrap et auto-hébergé.

## Conformité et benchmarks

| Contrôle | Windows | GNU/Linux |
| --- | ---: | ---: |
| CTest | 3/3 | 4/4 |
| Conformité portable | 20/20 | 20/20 |
| Benchmark smoke | 4/4 | 4/4 |
| État | réussi | réussi |

Les rapports JSON annoncent tous deux `Gs++ 0.27.0-alpha.7`, zéro échec et les
formats `.GsObj`, `.GsA` et `.GsE` en version 1.0 avec ABI 1.

## Reproductibilité des images auto-hébergées

Les fichiers MSVC et GNU sont identiques bit à bit :

| Image | Taille | SHA-256 |
| --- | ---: | --- |
| `ClassificateurMotsCles.GsE` | 12 894 | `30c01437bcfca0f2aea449fea6d2fe0140e86967d9fa4ed99bc864c1229c9fa6` |
| `Lexeur.GsE` | 42 163 | `25402c05c9d8af94bcf3ededb17f8b81f9f85c1a726e65d3d560e4b3392683a3` |
| `AnalyseurDeclarations.GsE` | 106 051 | `520f45c94da7099ae0381bd456b53fd4c023142ed4c6e412c86cd84dc4a5466b` |
| `AnalyseurSemantique.GsE` | 38 758 | `daadf1bba2828fd6a341f778fb19bfbd7ab5fc5b2dd2bdff162d31472358b7b6` |

Les GsObj d’exemple construits depuis les paquets peuvent conserver une taille
différente entre hôtes à cause des chemins de provenance. Les images liées
auto-hébergées ci-dessus ne présentent pas cette différence.

## Paquets validés

Les archives finales sont :

- `GsPlusPlus-0.27.0-alpha.7-Windows-x86_64.zip` ;
- `GsPlusPlus-0.27.0-alpha.7-Linux-x86_64.tar.gz`.

Chaque archive a été extraite dans un dossier neuf. Depuis chaque paquet :

- `gsppc --version` annonce `Gs++ Compiler 0.27.0-alpha.7` ;
- `gsechargeur --version` annonce `Chargeur GsE 0.27.0-alpha.7` ;
- `gseverifier` accepte l’analyseur sémantique livré ;
- `gsppc` compile l’exemple `Bonjour.Gs++` en GsObj.

Les empreintes finales ne sont volontairement pas incorporées dans le présent
document, lui-même inclus dans les archives. Le fichier externe
`SHA256SUMS.txt`, généré après le dernier paquetage, constitue la source de
vérité. Les deux archives et ce fichier sont consolidés sous
`Construction/GsPlusPlus-0.27.0-alpha.7/Packages/`.

## Limites conservées

- frontend auto-hébergé encore partiel ;
- pas encore de résolution complète des types ni de sélection typée des
  surcharges ;
- membres, conversions, constructeurs, visibilité, héritage et durée de vie à
  migrer dans les passes sémantiques suivantes ;
- cible de génération actuelle limitée à x86-64 et à la signature
  `GsAbi:x64-ms-v1`.
