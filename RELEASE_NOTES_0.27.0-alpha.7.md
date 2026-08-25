# Gs++ 0.27.0-alpha.7

Gs++ 0.27.0-alpha.7 est une préversion publique du langage, du compilateur et
de sa chaîne native x86-64. Elle ouvre la phase sémantique du frontend
auto-hébergé avec une première passe écrite en Gs++ et adopte le préfixe
d’espace de noms canonique `GalacticShrine::GsPP::`.

## Nouveautés

- ajout de `AnalyseurSemantique.GsE`, construit depuis un contrat et une
  implémentation Gs++ indépendants du bootstrap à l’exécution ;
- indexation des types, fonctions, variables globales, alias, champs, alias de
  champs, énumérateurs, paramètres et variables locales depuis l’AST compact ;
- résolution des paramètres, variables locales, globales simples ou
  qualifiées, récepteurs de classe `soi` / `this`, récepteurs de base `parent`
  / `super` et groupes de fonctions surchargées utilisés comme cibles d’appel ;
- détection positionnée des doubles déclarations et conflits de symboles, des
  noms inconnus, des adresses de surcharge ambiguës et de l’absence de toute
  fonction ;
- comparaison différentielle avec le bootstrap C++ sur des corpus bilingues
  valides et quinze familles d’erreurs sémantiques ;
- contrat à stockage fourni par l’appelant, avec interrogation des capacités,
  sorties partielles bornées et contrôle de l’arène mémoire ;
- migration sans alias de compatibilité des API Gs++ de `Gs::…` vers
  `GalacticShrine::GsPP::…` ;
- convention des commentaires de bloc multilignes sous la forme `/** … **/`,
  vérifiée par le corpus différentiel du lexeur.

Le contrat sémantique fixe `SymboleSemantique` à 48 octets,
`ResolutionSemantique` à 32 octets, `ResultatAnalyseSemantique` à 56 octets et
`RequeteAnalyseSemantique` à 120 octets. L’image demande seulement les deux
imports d’hôte d’allocation et de libération sous
`GalacticShrine::GsPP::Hote`.

## Rupture d’ABI des noms de symboles

Les projets étant encore locaux et reconstruisibles, cette préversion ne
conserve aucun alias `Gs::`. Toute source, bibliothèque ou image produite avec
une alpha antérieure doit être reconstruite. Les formats restent toutefois
inchangés : `.GsObj`, `.GsA` et `.GsE` sont en version 1.0, avec ABI 1 et les
signatures `GSOBJ:0`, `GSA:0` et `GSE:0`.

## Périmètre encore partiel

La première passe sémantique ne termine pas le frontend 0.27. La résolution
complète des types, la sélection typée des surcharges, les membres, les
conversions, les constructeurs, la visibilité, l’héritage et les règles de
durée de vie restent assurés par le bootstrap C++ ou à migrer.

## Ressources

- `GsPlusPlus-0.27.0-alpha.7-Windows-x86_64.zip` ;
- `GsPlusPlus-0.27.0-alpha.7-Linux-x86_64.tar.gz` ;
- `SHA256SUMS.txt`.

Chaque archive contient les outils, le SDK, les bibliothèques Gs++, les quatre
images GsE auto-hébergées, les exemples, le logo et la documentation Markdown.

## Vérification

Après téléchargement, comparez les empreintes avec `SHA256SUMS.txt`. Le commit
et le tag de publication sont signés. Cette version reste une préversion et ses
interfaces de frontend peuvent encore évoluer avant Gs++ 1.0.
