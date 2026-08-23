# Gs++ 0.27.0-alpha.2

Gs++ 0.27.0-alpha.2 est une préversion publique du langage, du compilateur et
de sa chaîne native. Elle poursuit l’auto-hébergement du frontend avec une
première représentation AST écrite en Gs++.

## Nouveautés principales

- nouvelle image `AnalyseurDeclarations.GsE` ;
- AST compact de 64 octets par nœud, à stockage fourni par l’appelant ;
- analyse des fonctions libres, paramètres et espaces de noms ;
- prise en charge des types natifs bilingues, types qualifiés, qualificatifs,
  pointeurs, références et tableaux fixes ;
- construction des jetons et de l’AST de travail dans l’arène hébergée ;
- interrogation de capacité et sorties bornées ;
- diagnostics positionnés et propagation distincte des erreurs lexicales ;
- comparaison différentielle avec l’analyseur C++ de bootstrap ;
- images `Lexeur.GsE` et `AnalyseurDeclarations.GsE` identiques entre les
  constructions MSVC et GNU.

## Validation

- Windows x64, Visual Studio 2026/v145 : **3/3 CTest** ;
- GNU/Linux x86-64 : **4/4 CTest** ;
- conformité portable : **20/20** sur les deux chaînes ;
- benchmark fonctionnel smoke : **4/4 scénarios** sur les deux chaînes ;
- `AnalyseurDeclarations.GsE` valide en format GsE 1.0, ABI 1, avec deux
  imports d’hôte explicites.

## Téléchargements

- `GsPlusPlus-0.27.0-alpha.2-Windows-x86_64.zip` ;
- `GsPlusPlus-0.27.0-alpha.2-Linux-x86_64.tar.gz` ;
- `SHA256SUMS.txt`.

Les paquets contiennent les outils natifs, le SDK, les bibliothèques Gs++, les
trois images auto-hébergées, les exemples, les README français et anglais, la
documentation Markdown et la licence MPL-2.0.

## Compatibilité

Les formats GsObj, GsA et GsE restent en version 1.0. Les champs ABI restent à
1 et la signature de liaison reste `GsAbi:x64-ms-v1`. Les signatures binaires
restent `GSOBJ:0`, `GSA:0` et `GSE:0`.

## Limites connues

Le frontend auto-hébergé demeure partiel. Cette version ne migre pas encore
les structures, classes, unions, énumérations, alias, variables globales,
instructions, expressions ni l’analyse sémantique. Les corps de fonctions
sont seulement délimités par leurs accolades dans la nouvelle tranche.

Cette préversion est destinée à l’évaluation et au développement. Elle ne doit
pas être présentée comme Gs++ 1.0 ni comme un compilateur entièrement
auto-hébergé.
