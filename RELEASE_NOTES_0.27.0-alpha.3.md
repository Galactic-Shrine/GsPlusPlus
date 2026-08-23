# Gs++ 0.27.0-alpha.3

Gs++ 0.27.0-alpha.3 est une préversion publique du langage, du compilateur et
de sa chaîne native x86-64. Elle étend le frontend auto-hébergé aux
déclarations de données, après le lexeur et les fonctions libres livrés par les
deux premières préversions 0.27.

## Nouveautés

- variables globales publiques, externes, initialisées et en tableau dans
  l’AST écrit en Gs++ ;
- structures, unions et classes de données avec champs, visibilité et héritage
  simple ;
- énumérations, énumérateurs, alias de déclarations et alias de champs ;
- nœuds parents-enfants et empreintes de types normalisées entre les syntaxes
  française et anglaise ;
- délimitation bornée des corps et initialiseurs imbriqués ;
- diagnostics positionnés supplémentaires et contrôle de la propreté de
  l’arène mémoire ;
- comparaison différentielle avec le frontend bootstrap sur Windows et Linux.

Le contrat `NoeudDeclaration` reste fixé à 64 octets. Les structures de requête
et de résultat restent respectivement à 80 et 48 octets. L’image produite ne
demande que les deux imports d’hôte explicites d’allocation et de libération.

## Périmètre encore partiel

Cette alpha ne prétend pas terminer le frontend 0.27. Les méthodes,
constructeurs, destructeurs et opérateurs de classes sont encore refusés par
l’analyseur auto-hébergé. Les corps et initialiseurs sont délimités, mais leur
AST d’instructions et d’expressions reste à migrer.

Les formats `.GsObj`, `.GsA` et `.GsE` restent en version 1.0, avec ABI 1 et
signatures `GSOBJ:0`, `GSA:0` et `GSE:0`.

## Ressources

- `GsPlusPlus-0.27.0-alpha.3-Windows-x86_64.zip` ;
- `GsPlusPlus-0.27.0-alpha.3-Linux-x86_64.tar.gz` ;
- `SHA256SUMS.txt`.

Chaque archive contient les outils, le SDK, les bibliothèques Gs++, les trois
images GsE auto-hébergées, les exemples et la documentation Markdown.

## Vérification

Après téléchargement, comparez les empreintes avec `SHA256SUMS.txt`. Le commit
et le tag de publication sont signés. Cette version reste une préversion et ses
interfaces de frontend peuvent encore évoluer avant Gs++ 1.0.
