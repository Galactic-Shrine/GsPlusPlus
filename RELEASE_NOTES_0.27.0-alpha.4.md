# Gs++ 0.27.0-alpha.4

Gs++ 0.27.0-alpha.4 est une préversion publique du langage, du compilateur et
de sa chaîne native x86-64. Elle achève la représentation compacte des
déclarations du frontend auto-hébergé en ajoutant les membres exécutables des
classes aux fonctions et données déjà couvertes.

## Nouveautés

- méthodes, constructeurs, destructeurs et surcharges d’opérateurs dans l’AST
  écrit en Gs++ ;
- relations parent-enfant entre classes, membres exécutables et paramètres
  explicitement présents dans la source ;
- genres de nœuds dédiés et conservation de la visibilité, de la virtualité et
  du remplacement ;
- description des initialisations de base, de champs et des constructeurs
  délégués par des drapeaux stables ;
- délimitation bornée des corps et des arguments d’initialisation imbriqués ;
- diagnostics positionnés supplémentaires et comparaison différentielle avec
  le frontend bootstrap sur des corpus français et anglais ;
- maintien de la propreté de l’arène mémoire et de la reproductibilité entre
  Windows et GNU/Linux.

Le paramètre `soi` que le bootstrap synthétise pour son traitement sémantique
n’est pas présenté comme un paramètre source. Le contrat
`NoeudDeclaration` reste fixé à 64 octets. Les structures de requête et de
résultat restent respectivement à 80 et 48 octets. L’image produite ne demande
que les deux imports d’hôte explicites d’allocation et de libération.

## Périmètre encore partiel

Cette alpha ne prétend pas terminer le frontend 0.27. Toutes les déclarations
sont maintenant représentées, mais les corps et initialiseurs sont seulement
délimités. Leur AST d’instructions et d’expressions reste à migrer, avant les
premières étapes sémantiques auto-hébergées.

Les formats `.GsObj`, `.GsA` et `.GsE` restent en version 1.0, avec ABI 1 et
signatures `GSOBJ:0`, `GSA:0` et `GSE:0`.

## Ressources

- `GsPlusPlus-0.27.0-alpha.4-Windows-x86_64.zip` ;
- `GsPlusPlus-0.27.0-alpha.4-Linux-x86_64.tar.gz` ;
- `SHA256SUMS.txt`.

Chaque archive contient les outils, le SDK, les bibliothèques Gs++, les trois
images GsE auto-hébergées, les exemples et la documentation Markdown.

## Vérification

Après téléchargement, comparez les empreintes avec `SHA256SUMS.txt`. Le commit
et le tag de publication sont signés. Cette version reste une préversion et ses
interfaces de frontend peuvent encore évoluer avant Gs++ 1.0.
