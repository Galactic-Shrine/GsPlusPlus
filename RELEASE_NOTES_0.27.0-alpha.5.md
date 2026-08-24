# Gs++ 0.27.0-alpha.5

Gs++ 0.27.0-alpha.5 est une préversion publique du langage, du compilateur et
de sa chaîne native x86-64. Elle fait progresser le frontend auto-hébergé en
construisant en Gs++ la hiérarchie des blocs et instructions des fonctions et
des membres exécutables de classes.

## Nouveautés

- blocs de fonction et blocs imbriqués dans l’AST auto-hébergé ;
- instructions `retourner`, instructions d’expression et variables locales ;
- conditionnelles `si`/`sinon` et boucles `tantque`, avec branches imbriquées ;
- relations parent-enfant en préordre entre fonction, bloc, contrôle et
  instruction ;
- conservation de la présence d’une expression, d’une branche `sinon`, d’un
  initialiseur local ou d’une construction explicite ;
- six genres de nœuds et trois drapeaux publics supplémentaires, sans changer
  la taille du contrat ABI ;
- alias bilingues génériques pour employer le nœud compact comme
  `NoeudSyntaxique` / `SyntaxNode` ;
- comparaison différentielle avec le frontend bootstrap sur des corps français
  et anglais structurellement équivalents ;
- vingt-deux diagnostics syntaxiques positionnés comparés au bootstrap ;
- maintien de la propreté de l’arène mémoire et de la reproductibilité entre
  Windows et GNU/Linux.

Le contrat de nœud reste fixé à 64 octets. Les structures de requête et de
résultat restent respectivement à 80 et 48 octets. L’image produite ne demande
que les deux imports d’hôte explicites d’allocation et de libération.

## Périmètre encore partiel

Cette alpha ne prétend pas terminer le frontend 0.27. Les déclarations et la
hiérarchie des instructions sont représentées, mais les expressions sont
encore seulement délimitées et signalées par des drapeaux. Leur AST interne et
les premières étapes sémantiques restent à migrer.

Les formats `.GsObj`, `.GsA` et `.GsE` restent en version 1.0, avec ABI 1 et
signatures `GSOBJ:0`, `GSA:0` et `GSE:0`.

## Ressources

- `GsPlusPlus-0.27.0-alpha.5-Windows-x86_64.zip` ;
- `GsPlusPlus-0.27.0-alpha.5-Linux-x86_64.tar.gz` ;
- `SHA256SUMS.txt`.

Chaque archive contient les outils, le SDK, les bibliothèques Gs++, les trois
images GsE auto-hébergées, les exemples et la documentation Markdown.

## Vérification

Après téléchargement, comparez les empreintes avec `SHA256SUMS.txt`. Le commit
et le tag de publication sont signés. Cette version reste une préversion et ses
interfaces de frontend peuvent encore évoluer avant Gs++ 1.0.
