# Gs++ 0.27.0-alpha.8

Gs++ 0.27.0-alpha.8 est une préversion publique du langage, du compilateur et
de sa chaîne native x86-64. Elle étend la première passe sémantique
auto-hébergée d’alpha.7 jusqu’au typage récursif des membres, constructeurs,
agrégats et compositions d’expressions déjà déterminables dans l’AST compact.

## Frontend auto-hébergé unifié

- regroupement du classificateur, du lexeur, de l’analyseur syntaxique et de
  l’analyseur sémantique dans l’unique image publique `Frontend.GsE` ;
- conservation de leurs quatre `.GsObj` comme modules internes de construction,
  sans installer plusieurs applications spécialisées ;
- exposition des quatre points d’entrée depuis l’image unifiée pour maintenir
  les comparaisons différentielles de chaque étape ;
- maintien de seulement deux imports d’hôte : allocation et libération de
  mémoire sous `GalacticShrine::GsPP::Hote`.

## Résolution sémantique étendue

- sélection typée des surcharges libres et membres depuis les paramètres,
  variables, littéraux, conversions et valeurs temporaires déjà connus ;
- résolution des champs, alias, méthodes et opérateurs par `.` ou `->`, avec
  héritage simple et contrôle des accès publics, protégés ou privés ;
- résolution surchargée des constructeurs locaux, délégations `soi(...)`,
  appels de base `parent(...)` et initialiseurs de champs ;
- construction implicite des bases et champs objets, validation des valeurs de
  champs par défaut et initialisation obligatoire des champs constants ;
- reconstruction des dimensions de tableaux et validation récursive des
  agrégats imbriqués, structures, unions et valeurs scalaires ;
- propagation des types de retour des fonctions, méthodes, opérateurs membres,
  comparaisons et opérateurs logiques à travers les expressions imbriquées ;
- typage récursif des indexations de tableaux ou pointeurs, de l’adresse `&`,
  du déréférencement `*` et des appels par pointeur de fonction ;
- prise en charge des tableaux de callbacks et des callbacks retournant un
  autre callback, y compris les signatures imbriquées fermées par `>>`.

Le corpus différentiel compte maintenant quatre-vingt-quatre familles d’erreurs
sémantiques positionnées, comparées au bootstrap C++ pour le code, la ligne et
la colonne.

## Contrats préservés

Cette préversion ne modifie ni les formats natifs ni l’ABI publique :

- `.GsObj`, `.GsA` et `.GsE` restent en version 1.0 ;
- leurs champs ABI restent fixés à 1 ;
- leurs signatures restent `GSOBJ:0`, `GSA:0` et `GSE:0` ;
- le préfixe d’espace de noms public reste `GalacticShrine::GsPP::` ;
- les tailles de `NoeudDeclaration`, `SymboleSemantique`,
  `ResolutionSemantique`, des requêtes et des résultats publics restent
  inchangées.

## Validation de la publication

La publication est validée sous Visual Studio 2026 et GNU/Linux avec :

- CTest complet sur les deux chaînes ;
- conformité portable 20/20 ;
- quatre scénarios de benchmark smoke sur chaque chaîne ;
- comparaison bit à bit de `Frontend.GsE` ;
- vérification du format GsE 1.0 et de ses imports/exports ;
- extraction et utilisation indépendante de chaque paquet.

La preuve détaillée se trouve dans
`Documentation/Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.8.md`.

## Périmètre encore partiel

Alpha.8 ne termine pas le frontend 0.27. Restent notamment à migrer ou à
valider exhaustivement :

- les contraintes complètes des valeurs adressables et des indices ;
- l’arité et les paramètres de tous les appels indirects ;
- les opérateurs libres et toutes les combinaisons intrinsèques ;
- les conversions implicites, qualifications et liaisons de références ;
- les plans exécutables complets de construction, destruction et durée de vie ;
- la reconstruction fonctionnelle d’une génération suivante du compilateur.

## Ressources

- `GsPlusPlus-0.27.0-alpha.8-Windows-x86_64.zip` ;
- `GsPlusPlus-0.27.0-alpha.8-Linux-x86_64.tar.gz` ;
- `SHA256SUMS.txt`.

Chaque archive contient les outils, le SDK, les bibliothèques Gs++, les
exemples, le logo et la documentation Markdown. Après téléchargement, comparez
les empreintes avec `SHA256SUMS.txt`. Le commit et le tag de publication sont
signés. Cette version reste une préversion et ses interfaces de frontend
peuvent encore évoluer avant Gs++ 1.0.
