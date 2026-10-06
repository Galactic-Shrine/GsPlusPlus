# Spécification du langage Gs++ 1.0

**CANDIDAT NORMATIF — établi en 0.24 et étendu par Gs++ 0.26.0.**

Ce document fixe le périmètre candidat du langage Gs++ 1.0. Les documents de
fonctionnalité liés restent normatifs pour les détails de syntaxe et de
disposition. Toute divergence doit être résolue avant la sortie 1.0 ; le code
et les tests déterminent l’état réellement implémenté pendant la convergence.

## Identité et objectifs

Gs++ est un langage système natif de Galactic-Shrine. Il produit directement
du code machine ; ce n’est pas un transpileur C++. Il vise :

- le code freestanding, les noyaux, chargeurs et pilotes ;
- les bibliothèques système ;
- les outils et applications hébergés ;
- l’écriture et l’auto-hébergement de sa propre toolchain.

Le français est canonique. Les mots-clés anglais documentés sont des alias
officiels et doivent conduire à la même sémantique et à la même génération.

## Extensions

| Usage | Extensions actuelles |
| --- | --- |
| source | `.Gs++`, `.GsPP`, `.GsPlusPlus` |
| interface | `.HGs++`, `.HGsPP`, `.HeaderGsPlusPlus` |
| projet | `.GsPj`, `.GsProject` |
| solution | `.GsPs` |
| objet | `.GsObj` |
| bibliothèque | `.GsA` |
| exécutable | `.GsE` |

`.GsPH`, `.GsO`, `.GsPPH` et `.GsPlusPlusHeader` sont obsolètes. `.Gs#`,
`.GsS` et `.GsSharp` sont réservées à Gs# et doivent être routées hors de
`gsppc`. Gs# ne possède aucun fichier d’en-tête.

## Unités et noms

- une unité source contient des espaces de noms, types, globales et fonctions ;
- une interface expose les déclarations nécessaires à la compilation séparée ;
- les noms qualifiés utilisent `::` ;
- les API livrées par Gs++ utilisent le préfixe canonique
  `GalacticShrine::GsPP::` ;
- `GsPP::` seul n'est pas un espace de noms public équivalent et ne participe
  pas à l'ABI des bibliothèques Gs++ ;
- les alias applicatifs peuvent cibler fonctions, globales, types et champs ;
- les cycles, conflits et cibles absentes sont des erreurs ;
- les symboles publics sont contrôlés entre unités par leur signature ABI.

La présentation des sources maintenues par le projet est définie dans les
[`CONVENTIONS_CODE_GS_PLUS_PLUS_1.0.md`](CONVENTIONS_CODE_GS_PLUS_PLUS_1.0.md).

### Un fichier par structure ou énumération

Une structure ou une énumération peut avoir son propre fichier. Pour partager
sa déclaration entre des sources compilées séparément, utiliser une interface
`.HGsPP` et la déclarer explicitement dans le projet XML :

```xml
<Interface Chemin="Point.HGsPP" />
<Interface Chemin="Etat.HGsPP" />
<Source Chemin="Principal.GsPP" />
```

Le constructeur de projets du bootstrap fournit toutes les interfaces du projet
à chaque unité source. Il n’y a pas de découverte automatique des types par nom
de fichier ; aucun `#include` n’est nécessaire pour ces interfaces déclarées.
Une déclaration dans un fichier `.GsPP` séparé doit, elle, être compilée avec ses
consommateurs dans la même unité, par exemple en mode de compilation `agregee`.
La compilation séparée de sources ne rend pas automatiquement leurs types
visibles les unes aux autres.

L’exemple [TypesParFichier](../Exemples/TypesParFichier/Application.GsPj) contient
une structure `Point`, une énumération `Etat` et une source qui les utilise.
`gsppc Exemples/TypesParFichier/Application.GsPj` construit son exécutable ;
son point d’entrée retourne **42**. Cette preuve concerne l’orchestration du
bootstrap, pas une orchestration de projets par le frontend auto-hébergé.

### Inclusion textuelle et utilisation d'espaces de noms

**VALIDÉ dans le bootstrap `gsppc`, développement local après alpha.10.**
Les archives alpha.10 déjà publiées ne contiennent pas cet ajout.
Le fonctionnement distingue l'inclusion d'un fichier de la recherche des noms :

```cpp
#inclure "Types/Point.HGsPP"
// Alias anglais : #include "Types/Point.HGsPP"

espace GalacticShrine::GsPP::Application {

    utilisant espace GalacticShrine::GsPP::Types;
    // Alias anglais : using namespace GalacticShrine::GsPP::Types;
}
```

- `#inclure` / `#include` insère les jetons du fichier à l'endroit de la
  directive. Un chemin relatif est résolu depuis le dossier du fichier
  contenant cette directive, y compris dans une inclusion imbriquée.
- Les chemins sont des chaînes UTF-8 entre guillemets ; utiliser `/` comme
  séparateur portable. Les chemins absolus sont également acceptés.
  Les extensions Gs# et obsolètes sont refusées.
- Une directive commence une ligne, éventuellement précédée d'espaces ou de
  commentaires, et ne peut contenir de jetons supplémentaires après son
  argument. Les commentaires de fin de ligne sont acceptés.
- `#pragma once` empêche de relire le même fichier pendant la préparation de
  cette unité. Il n'existe pas de déduplication implicite. Les cycles sans
  cette protection sont diagnostiqués et la profondeur d'inclusion est bornée
  à 128 fichiers actifs.
- Les positions des types, fonctions et diagnostics conservent le fichier
  inclus, sa ligne et sa colonne. Une interface `.HGsPP` incluse conserve la
  sémantique des prototypes d'interface ; sa définition peut être fournie par
  une source ou une bibliothèque.
- `utilisant espace N;` / `using namespace N;`, **sans `#`**, permet de
  rechercher les noms de `N` sans recopier leur préfixe. Il n'inclut aucun
  fichier et ne lie aucune bibliothèque. L'espace doit être déclaré avant
  la directive dans cette unité, ou fourni par une autre unité d'interface.
- Cette première implémentation accepte ces utilisations au niveau global et
  dans un espace de noms. Elles agissent après leur déclaration, dans cet
  espace et ses descendants, sans se propager aux autres unités.
- Types, alias applicatifs, valeurs d'énumération, globales et fonctions peuvent
  être recherchés. Les surcharges importées sont regroupées avant de choisir
  une fonction. Les imports transitifs et cycliques sont parcourus sans boucle.
  Une ambiguïté sur un nom utilisé est une erreur ; des imports inutilisés ne
  déclenchent pas à eux seuls un diagnostic. Les noms explicitement qualifiés
  restent disponibles pour lever l'ambiguïté.
- Une variable locale masque les noms importés. Pour les noms d'espaces, les
  déclarations du niveau le plus proche priment sur les niveaux externes ;
  les utilisations participent au niveau de l'ancêtre commun de leurs espaces.

La recherche lexicale des noms non qualifiés remonte aussi les espaces parents
**sans directive `utilisant`**. Une méthode de classe peut ainsi utiliser une
globale déclarée dans l'espace contenant sa classe. Les types nommés, alias,
valeurs d'énumération, fonctions et opérateurs libres suivent également cette
remontée. Le premier niveau contenant un nom masque les niveaux externes :
une surcharge incompatible à ce niveau ne fait pas essayer une fonction
homonyme d'un espace plus éloigné. Les paramètres et variables locales restent
prioritaires pour les expressions.

Dans un corps de méthode, de constructeur ou de destructeur, la recherche
commence à la portée de la classe avant de remonter ses espaces parents. Les
champs par défaut évalués pour un constructeur suivent le même contexte ;
ses paramètres restent prioritaires. Une méthode homonyme masque ainsi une
fonction de l'espace parent ou importée. Les formes non liées conservent leur
récepteur explicite `Classe&` : `Lire(soi)` ou `soi.Lire()` peuvent appeler une
méthode sans autre argument, mais `Lire()` n'ajoute pas automatiquement `soi`.
Une qualification comme `N::Lire()` reste disponible pour désigner la fonction
de l'espace parent. Cette règle est comparée au bootstrap C++ dans la matrice
des méthodes du frontend ; elle n'introduit pas de nouvelles règles d'appel.

Pour une expression surchargée, le groupe associé au type de l'opérande gauche
(ou de l'unique opérande unaire), y compris ses bases, est recherché avant le
groupe lexical. À défaut, la recherche suit la portée de la classe appelante
puis ses espaces parents et imports. Un groupe trouvé mais incompatible ne
fait pas essayer un groupe parent. Les arguments de la surcharge restent les
opérandes de l'expression : aucun récepteur de la classe appelante n'est ajouté.
Les accès privés et protégés restent contrôlés. Un même champ par défaut peut
sélectionner des surcharges différentes selon le type du paramètre de chaque
constructeur ; ses résolutions ne sont pas réutilisées entre ces contextes.
Ces règles existantes sont couvertes par la matrice des opérateurs dans les
méthodes, y compris le passage d'un opérateur surchargé à une forme intrinsèque
dans un autre constructeur.

Les types écrits dans les conversions d'une valeur de champ par défaut suivent
également le contexte du constructeur qui l'évalue. Un alias de cette portée
peut donc masquer un alias d'un espace parent ou importé, y compris dans une
signature de callback imbriquée. Cela ne change pas le contexte du type déclaré
du champ. Un champ explicitement initialisé ne fait pas analyser sa valeur par
défaut remplacée ; un constructeur délégué ne l'évalue pas une seconde fois.
Les conversions des arguments de `parent(...)`, `soi(...)` et des initialiseurs
de champs sont également comparées au bootstrap dans la matrice des constructions.

La priorité existante d'un nom explicitement qualifié correspondant à un nom
complet est conservée ; sinon sa qualification est recherchée relativement
dans les espaces parents. Une classe n'est pas un espace importable par
`utilisant espace`. Les valeurs d'énumération déclarées plus tard ne deviennent
pas visibles dans les initialiseurs des valeurs précédentes.

Les projets XML restent responsables des sources compilées, bibliothèques,
sorties et options. Pour une interface donnée, choisir sa fourniture par XML
ou son inclusion textuelle : ne pas ajouter aussi une entrée `<Interface>`
pour un type déjà inclus, sous peine de redéclaration. `#pragma once` est
local à chaque unité préparée, pas à l'ensemble des entrées XML.

**VALIDÉ dans le périmètre testé du frontend auto-hébergé :** le classificateur,
le lexeur, l'analyseur de déclarations et la résolution sémantique de
`Frontend.GsE` reconnaissent les utilisations d'espaces. Les tests différentiels
couvrent types, alias, énumérations, globales, surcharges et opérateurs libres,
imports transitifs et cycles, masquage, portée et ambiguïtés.
La lecture et l'expansion des fichiers sont effectuées par le bootstrap hôte.
Cette tranche ne prend pas en charge les utilisations dans un bloc de
fonction, `using Type = ...`, `using N::Nom`, les macros, `#define`, les
conditions `#if` / `#ifndef`, les chemins `<...>` ni les répertoires `-I`.
Elle n'est donc pas une implémentation complète du préprocesseur ou de la
recherche des noms C++.

L'exemple [Directives](../Exemples/Directives/Application.GsPj) ne déclare
aucune interface dans son XML : les sources française et anglaise incluent
leurs types et retournent **42**. La matrice de régression couvre aussi les
chemins UTF-8, inclusions répétées, prototypes, cycles, ambiguïtés et positions
des diagnostics dans les fichiers inclus.

## Types fondamentaux

Le contrat candidat 1.0 comprend :

- `vide`/`void` ;
- booléens ;
- octets et caractères ;
- entiers signés et non signés de 8, 16, 32 et 64 bits ;
- pointeurs ;
- références locales, paramètres et receveurs selon les formes implémentées ;
- pointeurs de fonction typés ;
- structures, unions et énumérations ;
- tableaux fixes multidimensionnels ;
- classes.

`constante`/`constant`, `const` et `volatile` participent aux règles de type.
Les conversions implicites ne peuvent pas supprimer un qualificateur. Les
conversions explicites utilisent `convertir<T>`/`cast<T>` et restent limitées
aux catégories prises en charge.

## Expressions et contrôle

Sont inclus dans le périmètre candidat :

- littéraux entiers, booléens et chaînes UTF-8 ;
- accès, indexation et déréférencement ;
- appels directs, indirects et méthodes ;
- opérateurs arithmétiques, logiques, comparatifs et binaires implémentés ;
- court-circuit réel de `&&` et `||` ;
- conditions, boucles, blocs et retours ;
- initialisations agrégées de structures et unions ;
- copie et affectation structurées.

## Fonctions et callbacks

- fonctions globales, méthodes et surcharges ;
- au plus quatre paramètres ordinaires dans le sous-ensemble courant ;
- callbacks typés et signatures récursives ;
- retours scalaires, pointeurs, références prises en charge et valeurs
  structurées ;
- imports et exports explicites ;
- symboles publics compatibles entre unités uniquement si leur signature ABI
  est identique.

Une signature de callback peut retourner une référence : son appel constitue
alors une valeur gauche liée au stockage renvoyé, et non une copie temporaire.
La lecture charge la valeur référencée ; la liaison, la prise d'adresse et
l'affectation utilisent son adresse. Les qualifications `constante` interdisent
les mutations et liaisons mutables correspondantes ; l'adresse d'un champ ou
élément constant reste qualifiée. Une structure retournée par référence n'utilise
pas le mécanisme de retour de structure par valeur.

Lors de la prise d'adresse d'un champ ou élément, les qualifications `volatile`
et `constante volatile` héritées de l'objet sont également conservées, y compris
après un accès par flèche. Les champs de type pointeur ou callback conservent les
qualifications de leur propre type déclaré, sans ajouter celles de leur objet
contenant au pointeur stocké. `volatile` ne fournit pas de garantie d'atomicité
ou de synchronisation ; les primitives atomiques restent distinctes.

Ce fonctionnement est vérifié avec des callbacks C++ fournis par l'hôte dans
la matrice de développement. Il n'autorise pas encore les fonctions ordinaires
Gs++ à retourner une référence : leurs déclarations restent refusées dans le
sous-ensemble courant. Le stockage de l'hôte doit rester valide durant son
utilisation, sans prolongement automatique de sa durée de vie.

## Modèle objet

Le périmètre candidat 1.0 actuellement validé comprend :

- classes et visibilité publique, protégée et privée ;
- constructeurs et destructeurs ;
- surcharge de fonctions et d’opérateurs ;
- RAII sur les sorties normales de blocs, branches, boucles et retours ;
- méthodes virtuelles optionnelles ;
- héritage simple public ;
- `remplacer`/`override` obligatoire pour un virtuel hérité ;
- conversions dérivée vers base par pointeur ou référence ;
- refus du slicing implicite par valeur ;
- `parent(...)`/`super(...)` et appels directs à l’implémentation de base ;
- initialisateurs ordonnés de champs ;
- valeurs par défaut des champs de classes, remplacées par un initialiseur
  explicite lorsqu’il existe ;
- délégation exclusive avec `soi(arguments)`/`this(arguments)`, sans cycle ;
- durée de vie récursive des champs objets classes ;
- tableaux fixes de champs et variables locales objets classes ;
- arguments de construction uniformes, réévalués pour chaque élément.

La convergence exécutable de ce modèle objet et de sa durée de vie est suivie
dans le document du
[`frontend auto-hébergé 0.27`](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md).

Une valeur de champ par défaut est analysée dans le contexte du constructeur
qui l'évalue. Les arguments agrégés de ses appels utilisent le type de la
signature retenue dans ce contexte. Un refus interne à l'expression conserve
son diagnostic et sa position ; le diagnostic d'incompatibilité avec le champ
ne s'applique qu'après analyse réussie de l'expression, si sa valeur finale
ne peut pas initialiser ce champ. Une valeur remplacée explicitement n'est
pas évaluée.

## Durée de vie

- une variable locale de classe est construite à sa déclaration ;
- les bases sont construites de la racine vers la dérivée ;
- les champs sont construits dans l’ordre de déclaration ;
- les éléments de tableaux sont construits en ordre d’indices croissant ;
- les arguments uniformes de tableaux sont réévalués pour chaque élément ;
- un constructeur délégué laisse sa cible initialiser entièrement l’objet avant
  d’exécuter son propre corps ;
- le corps du destructeur courant s’exécute avant les champs directs ;
- les champs sont détruits en ordre inverse ;
- les éléments de tableaux sont détruits en ordre d’indices inverse ;
- la base est détruite après les champs de la dérivée ;
- la valeur d’un retour est évaluée avant les destructions de portée.

Aucun déroulement d’exception ni objet de classe global n’appartient au contrat
courant. Les globales sérialisables non classes restent prises en charge sans
initialisation cachée.

## Compilation séparée

Une interface et ses consommateurs doivent produire des déclarations
compatibles. GsObj contient les signatures nécessaires au contrôle de type et
de disposition. GsA regroupe des GsObj valides. L’éditeur de liens refuse les
symboles dupliqués, les cibles absentes et les signatures incompatibles.

Les conteneurs et métadonnées de cette compilation sont définis par les
formats [GsObj 1.0](FORMAT_GSOBJ_1.0.md) et
[XML de projet 1.0](FORMAT_PROJETS_GS_PLUS_PLUS_1.0.md).

## Profils d’exécution

Le même langage et la même ABI prennent en charge deux profils :

- freestanding : aucune dépendance hébergée ou initialisation cachée ;
- hébergé : fichiers, flux, conteneurs et diagnostics explicitement liés.

Les règles complètes se trouvent dans
[`PROFILS_GS_PLUS_PLUS_1.0.md`](PROFILS_GS_PLUS_PLUS_1.0.md).

## Formats et ABI

- [`FORMAT_GSOBJ_1.0.md`](FORMAT_GSOBJ_1.0.md) ;
- [`FORMAT_GSA_1.0.md`](FORMAT_GSA_1.0.md) ;
- [`FORMAT_GSE_1.0.md`](FORMAT_GSE_1.0.md) ;
- [`ABI_GS_PLUS_PLUS_X64_MS_V1.md`](ABI_GS_PLUS_PLUS_X64_MS_V1.md).

Les trois formats restent en version 1.0 et les champs ABI valent 1.

## Décisions requises avant la sortie 1.0

Les fonctions suivantes ne sont pas implicitement promises. Chaque élément
doit être soit implémenté et testé, soit explicitement exclu du contrat final :

- arguments ou agrégats distincts par élément de tableau d’objets ;
- copie implicite de tableaux d’objets classes ;
- méthodes virtuelles pures ;
- conversions descendantes ;
- RTTI ;
- héritage multiple ou virtuel ;
- exceptions du langage.

Les valeurs par défaut de champs, les constructeurs délégués et les arguments
uniformes de tableaux sont inclus depuis 0.25. Les objets de classe globaux
sont explicitement exclus du contrat 1.0 courant afin de préserver le profil
freestanding sans runtime caché.

L’héritage multiple, l’héritage virtuel, la RTTI et les exceptions ne sont pas
des conditions automatiques de Gs++ 1.0. Leur absence peut être normative si
elle est diagnostiquée et n’empêche pas l’auto-hébergement.

## Conformité

Une fonction n’est `VALIDÉE` que si elle possède une preuve exécutable actuelle
dans la suite unitaire, d’intégration ou de conformité. La structure et les
identifiants de conformité sont définis dans
[`CONFORMITE_GS_PLUS_PLUS_1.0.md`](CONFORMITE_GS_PLUS_PLUS_1.0.md).
