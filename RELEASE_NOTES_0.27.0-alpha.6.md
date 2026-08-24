# Gs++ 0.27.0-alpha.6

Gs++ 0.27.0-alpha.6 est une préversion publique du langage, du compilateur et
de sa chaîne native x86-64. Elle poursuit le frontend auto-hébergé en
construisant en Gs++ l’AST interne complet des expressions prises en charge par
le bootstrap syntaxique.

## Nouveautés

- onze genres publics supplémentaires pour les littéraux, références de
  variables, expressions unaires et binaires, affectations, appels, accès aux
  membres, indexations, conversions et agrégats ;
- analyse récursive avec les mêmes priorités et associativités que le frontend
  bootstrap C++ ;
- parcours préordre et relations parent-enfant depuis les déclarations ou
  instructions jusqu’à chaque sous-expression ;
- conservation des valeurs entières, chaînes décodées, noms qualifiés,
  opérateurs et empreintes des types de conversion ;
- distinction publique des littéraux booléens, des accès membres via pointeur
  et des références à la base `parent` / `super` ;
- expressions rattachées aux globales, énumérateurs, champs, listes
  d’initialisation de constructeurs, retours, variables locales,
  conditionnelles, boucles et instructions d’expression ;
- comparaison différentielle sur des corpus français et anglais couvrant les
  onze genres, les six opérateurs unaires et les dix-huit opérateurs binaires ;
- trente-trois diagnostics syntaxiques positionnés comparés au bootstrap,
  dont les erreurs de conversion, d’appel, d’indexation, d’agrégat et de
  dépassement d’un entier 64 bits ;
- maintien de la propreté de l’arène mémoire et de la reproductibilité entre
  Windows et GNU/Linux.

Le contrat de nœud reste fixé à 64 octets. Les structures de requête et de
résultat restent respectivement à 80 et 48 octets. L’image produite ne demande
que les deux imports d’hôte explicites d’allocation et de libération.

## Périmètre encore partiel

Cette alpha termine l’AST syntaxique des expressions dans la tranche
auto-hébergée actuelle, mais ne prétend pas terminer le frontend 0.27. Les
premières résolutions sémantiques restent assurées par le bootstrap C++ et
doivent encore être migrées avant de déclarer le frontend complet.

Les formats `.GsObj`, `.GsA` et `.GsE` restent en version 1.0, avec ABI 1 et
signatures `GSOBJ:0`, `GSA:0` et `GSE:0`.

## Ressources

- `GsPlusPlus-0.27.0-alpha.6-Windows-x86_64.zip` ;
- `GsPlusPlus-0.27.0-alpha.6-Linux-x86_64.tar.gz` ;
- `SHA256SUMS.txt`.

Chaque archive contient les outils, le SDK, les bibliothèques Gs++, les trois
images GsE auto-hébergées, les exemples, le logo et la documentation Markdown.

## Vérification

Après téléchargement, comparez les empreintes avec `SHA256SUMS.txt`. Le commit
et le tag de publication sont signés. Cette version reste une préversion et ses
interfaces de frontend peuvent encore évoluer avant Gs++ 1.0.
