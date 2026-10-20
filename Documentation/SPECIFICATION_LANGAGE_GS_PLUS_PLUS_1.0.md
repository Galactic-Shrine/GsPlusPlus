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

Les groupes mêlant méthodes et fonctions libres comparent les candidats
compatibles selon les règles de surcharge actuelles, sans priorité automatique
de la méthode. Un référent constant exclut une référence scalaire mutable ;
un temporaire n'est pas lié à une référence. Une conversion prise en charge
vers une classe de base coûte davantage qu'une correspondance exacte avec
la classe dérivée. Si deux candidats ont le même meilleur score, l'appel reste
ambigu ; une référence scalaire n'est pas automatiquement préférée à une
valeur du même type, ni une référence mutable à une référence constante
compatible avec ce même argument mutable. Les qualifications des pointeurs
participent à leur compatibilité. Ces règles s'appliquent aussi aux appels
dans les champs par défaut, initialiseurs explicites, bases, membres et
délégations des constructeurs, dans le périmètre vérifié.

Les groupes mixtes d'opérateurs suivent les mêmes règles de sélection :
les paramètres référencés conservent le stockage du scalaire, de l'élément
de tableau ou du pointeur reçu. Un opérateur prenant `entier32*&` peut changer
la cible de ce pointeur ; avec `constante entier32*&`, cette redirection reste
autorisée sans permettre la mutation de la donnée pointée. Les expressions
imbriquées et les expressions des constructions conservent les qualifications
et contrôles de liaison. Dans les combinaisons vérifiées avec `&&` et `||`
intégrés, le court-circuit empêche l'exécution de l'opérateur contenu dans
l'opérande ignoré, mais pas son analyse sémantique : une expression invalide
reste refusée. Cette couverture ne revendique pas les mêmes propriétés pour
toutes les surcharges d'opérateurs logiques.

Dans les initialiseurs agrégés couverts, les expressions sont évaluées et
leurs valeurs stockées dans l'ordre des éléments. Deux éléments utilisant
le même référent capturent chacun sa valeur au moment de leur évaluation :
une mutation ultérieure ne réécrit pas l'élément déjà initialisé. Les éléments
omis sont initialisés à zéro dans les structures et tableaux vérifiés.
Le contrôle sémantique vérifie la forme et le nombre d'éléments à chaque
niveau d'agrégat avant ses feuilles, puis contrôle chaque élément selon son
type destination avant de passer au suivant. Cette priorité s'applique aussi
aux valeurs agrégées affectées ou retournées ; les interdictions concernant
la cible d'affectation restent vérifiées avant sa valeur.

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

Un retour de callback `entier32*&` désigne l'emplacement contenant le pointeur :
sa lecture copie la cible, tandis qu'une liaison ou prise d'adresse conserve
l'accès à cet emplacement. `constante entier32*&` désigne un pointeur vers des
données constantes : ce pointeur peut être réaffecté avec une valeur du même
type qualifié, mais les données pointées ne peuvent pas être modifiées.

Un callback retournant `pointeur_fonction<entier32(entier32)>&` peut fournir
un emplacement modifiable de callback : l'appel imbriqué lit sa fonction,
la liaison ou prise d'adresse conserve son stockage, et une copie garde sa
propre cible après remplacement de l'original. Contrairement au pointeur vers
des données constantes, `constante pointeur_fonction<entier32(entier32)>&`
protège la valeur du callback stocké : sa lecture et son appel restent autorisés,
mais son remplacement est refusé, directement ou via liaison, déréférencement,
champ ou indexation. Aucune conversion de qualification supplémentaire n'est
introduite ; les signatures restent contrôlées exactement.

Une signature de callback peut combiner paramètre et retour par référence,
par exemple `pointeur_fonction<entier32&(entier32&)>`, même lorsque ce callback
est lui-même obtenu par référence ou copié par valeur. Le paramètre reçoit
le stockage de l'argument, et le retour peut désigner ce même stockage.
La constance du callback n'ajoute pas de qualification à ses paramètres :
un callback constant acceptant `entier32&` peut modifier cet argument.
Un paramètre `constante entier32&` reste une référence de lecture ; les
temporaires et référents incompatibles restent refusés selon les règles de
liaison actuelles, sans liaison temporaire supplémentaire de style C++.

Le même contrat s'applique aux structures : `pointeur_fonction<P&(P&)>`
reçoit le stockage de `P`, y compris ses champs, tableaux et pointeurs, et peut
le retourner sans copie. Une copie par valeur de ce retour garde un stockage
indépendant. Avec `constante P&`, les champs et éléments restent protégés.
Un callback `pointeur_fonction<entier32*&(entier32*&, entier32*)>` reçoit
l'emplacement du pointeur, pas seulement sa cible ; il peut le rediriger et
retourner ce même emplacement. La variante
`pointeur_fonction<constante entier32*&(constante entier32*&, constante entier32*)>`
autorise également la redirection, mais interdit la mutation de la donnée
pointée. Les qualifications des arguments restent contrôlées exactement ;
ces formes n'introduisent ni conversion implicite supplémentaire ni liaison
de référence à un temporaire.

Une affectation de tableau entier ou de sous-tableau reste interdite. Cette
règle ne s'applique pas au déréférencement d'un pointeur extrait d'un tableau
de pointeurs : l'affectation vise alors le référent et suit ses propres règles
de type et de constance.

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

### Analyse auto-hébergée d'une interface en mémoire

L'entrée `GalacticShrine::GsPP::Autohebergement::AnalyserDeclarationsInterface`
et son alias `AnalyzeInterfaceDeclarations` acceptent un contenu UTF-8
**déjà préparé**, avec la même `RequeteAnalyseDeclarations` et les mêmes
capacités que `AnalyserDeclarationsSource`. Le mode s'applique à tout le
texte de cette requête ; il n'est pas déduit d'une extension de fichier.

Fonctions et globales deviennent externes implicitement, sans export public
de définition. Les membres de classes restent soumis à leur visibilité,
avec prototypes externes et absence de corps. Une globale initialisée, un
corps de fonction ou une liste d'initialisation de constructeur sont refusés.
Types, champs, énumérations, alias et utilisations d'espaces restent analysés.
Les contrôles de signature existants restent applicables aux prototypes.

Cette entrée ne lit aucun fichier, ne traite pas `#inclure` / `#include` ou
`#pragma once`, n'assemble pas les unités et ne normalise pas les prototypes
contre leurs définitions. Les positions retournées sont celles du texte fourni,
sans identité de fichier dans l'AST compact. L'assemblage préparé est décrit
ci-dessous, avec sa normalisation préparée. Une entrée additive décrite ensuite
analyse les origines des jetons d'une unité développée ; les entrées d'assemblage
avec origines décrites ensuite raccordent cette table à la normalisation et
à la sémantique par unité. L'analyse
sémantique complète actuelle garde notamment l'exigence d'au moins une fonction.

### Assemblage auto-hébergé de plusieurs unités préparées

`AssemblerDeclarationsPreparees` / `AssemblePreparedDeclarations` analyse
séparément les sources et interfaces préparées, puis produit un texte UTF-8,
un AST à racine unique et une table d'origines. Il retire uniquement le BOM
initial de chaque unité et ajoute un LF après chaque contenu, sans terminateur
nul. Parents, lignes et tranches nominales sont rebasés. Les trois tailles
sont interrogeables ; aucun tampon de sortie n'est écrit en cas d'erreur ou
de capacité insuffisante. Les anciens contrats restent inchangés.

`AnalyserSemantiqueUnites` / `AnalyzeUnitSemantics` reçoit cette table avec
la requête sémantique existante : les imports d'espaces directs/transitifs
restent limités à leur unité, et le diagnostic expose aussi le rang de
l'unité et ses positions locales. Une interface analysée comme unité
séparée n'est pas une inclusion textuelle dans son consommateur.

L'assemblage brut n'effectue **pas la normalisation** des prototypes,
globales et alias répétés. Un prototype et sa définition sont tous deux
conservés dans l'AST assemblé. Cette API historique attend des textes déjà
développés, ne lit aucun fichier et ne reçoit pas de table
d'origines de jetons. Les entrées additives décrites ci-dessous la prennent en charge.
Voir le [contrat du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#assemblage-préparé-et-origines-des-unités--6-octobre-2026).

### Normalisation préparée des déclarations

`AssemblerDeclarationsNormalisees` / `AssembleNormalizedDeclarations` est une
entrée additive utilisant la même requête d'assemblage. Elle normalise les
fonctions/opérateurs libres et membres, constructeurs, destructeurs, globales
et alias racines après analyse de toutes
les unités, avec les règles du bootstrap dans ce périmètre. Les noms et types
sont comparés exactement avant résolution des alias ; une définition compatible
remplace les prototypes dans l'ordre de première déclaration des fonctions. Les doubles
définitions et incompatibilités sont refusées : fonctions, puis globales, puis
alias. Les positions restent celles de la déclaration choisie ou fautive.

Les besoins de capacité de l'AST sont ceux de la sortie normalisée. Aucun
des trois tampons n'est publié en cas d'erreur ou de capacité insuffisante.
L'assemblage brut conserve son comportement sans fusion. La clé d'une méthode
inclut son récepteur implicite `Classe&`, comparable au premier paramètre d'une
fonction libre de même nom complet. Les noms de constructeurs et destructeurs
sont distincts des noms des méthodes ordinaires. Les types et énumérations
restent distincts : les classes répétées ne sont pas fusionnées, même si leurs
fonctions sont compatibles ; leur conflit relève ensuite de la sémantique.

L'AST normalisé place d'abord les types et leurs enfants non fonctionnels,
dans leur ordre source, puis les déclarations restantes et les fonctions dans
leur ordre de première apparition. Les sous-arbres des fonctions restent
contigus ; les membres d'une classe ne sont pas nécessairement contigus à son
en-tête. Les consommateurs doivent utiliser `Parent`, réindexé vers un nœud
antérieur, et non déduire le propriétaire d'une fonction de sa position physique.
Les positions et tranches de noms restent celles de la déclaration retenue.
Cette entrée préparée ne développe pas les inclusions et ne remplace pas
encore le flux de compilation de fichiers de `gsppc`.
Voir le [contrat de normalisation étendu](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#normalisation-des-membres-et-groupes-mixtes--6-octobre-2026).

### Expansion en mémoire des inclusions textuelles

`DevelopperInclusionsDeclarations` / `ExpandDeclarationIncludes` reçoit une
`RequeteExpansionDeclarations` de **128 octets** : catalogue de
`FichierInclusionDeclarations` de **32 octets**, liens de
`LienInclusionDeclarations` de **32 octets**, indice racine, deux tampons de
sortie, capacités et options BOM/CRLF. Son résultat de **48 octets** expose le
code, détail lexical, fichier/ligne/colonne, nombres exacts d'octets/origines et
mémoire d'arène. Les contrats sont déclarés dans `ExpansionDeclarations.HGsPP`.

L'hôte lit les fichiers et résout les chemins. Chaque entrée du catalogue
porte source, taille, identité canonique, mode source/interface et réserve
nulle. Les identités, entre zéro et `NombreFichiers-1`, regroupent les alias
pour `once` et les cycles ; les indices distincts conservent leurs noms de
diagnostic. L'hôte garantit les identités, contenus, modes et cibles : le
frontend ne les vérifie pas contre le système de fichiers. Les liens sont
uniques et triés par `(IndexFichier, DebutDirective)`. Leur état est disponible
0, introuvable 1 ou extension incompatible 2 ; les deux derniers états portent
`IndexCible=NombreFichiers`. Une directive valide sans lien est un argument
invalide, pas un fichier réputé absent. Les liens ne remplacent pas le contrôle
lexical et grammatical des directives.

Le frontend applique les mêmes règles que le bootstrap : lexage **du fichier
entier avant ses directives**, directive au début d'une ligne logique, un seul
argument sur cette ligne et aucun jeton supplémentaire. `#inclure` / `#include`
exigent un chemin entre guillemets non vide, sans NUL/CR/LF décodé ; seul
`#pragma once` est accepté. `once` prend effet au point où il est rencontré,
avant les inclusions suivantes, et n'est pas préscanné. Une identité protégée
est ignorée avant toute réentrée, contrôle de profondeur ou lexage cible ; sinon
un cycle est signalé sur la directive appelante. La limite est **128 fichiers
actifs** ; une 129e entrée est refusée en position 1:1 du fichier cible. Les
fichiers non visités ne sont pas lexés. Les états `once`/actifs sont propres à
l'appel et ne persistent ni entre unités ni entre appels de mesure/publication.

L'expansion sélectionne les fragments originaux et appelle la préparation
lexicale ci-dessous : lexèmes conservés, séparateurs LF/CRLF, BOM optionnel,
origines locales, modes par fichier et un seul EOF, celui de la racine.
L'EOF conserve le mode zéro du bootstrap ; le mode global d'une unité
d'interface reste celui de sa requête d'analyse/assemblage.

Le texte et les origines sont **transactionnels**. Aucun préfixe n'est publié
sur refus, échec d'allocation ou capacité insuffisante ; cette dernière expose
les deux tailles exactes après expansion. Les erreurs d'argument, capacité ou
allocation n'incriminent aucun fichier : indice `NombreFichiers` et positions
nulles. Les refus lexicaux/de directives exposent leurs coordonnées originales.
Sur les autres refus, les deux nombres de sortie sont nuls ; la mémoire d'arène
reste une mesure du travail effectué. L'arène est libérée sur chaque sortie.
Entrées, requête et tampons doivent être indépendants, valides et stables.

Les codes sont succès 0, capacité 1, argument 2, allocation 3, lexage 4,
directive hors début de ligne 5, argument attendu 6, texte après directive 7,
pragma non pris en charge 8, chemin attendu 9, fichier introuvable 10,
extension incompatible 11, cycle 12, profondeur 13 et taille excessive 14.
Le catalogue est borné à un million de fichiers, les liens et jetons de sortie
à cent millions ; chaque source et le texte développé sont bornés à un milliard
d'octets. La sélection utilise une pile itérative et une arène, sans lecture
de fichier ni nouvel import d'hôte. L'entrée ne remplace pas encore le pilote
de compilation de fichiers de `gsppc`.

### Adaptateur hôte du catalogue de fichiers

Le bootstrap expose `GsPP::CreerCatalogueInclusions` dans
`Compiler/include/GsPP/CatalogueInclusions.hpp`. Son résultat propriétaire
`CatalogueInclusions` garde les textes, vues ABI, liens triés et noms de
diagnostic. Il n'est pas copiable ; ses déplacements conservent les pointeurs
vers les textes et vident l'objet déplacé. Les lectures utilisent des chemins
physiques `UniteSource::Chemin`, indépendants de `NomDiagnostic`. Le préfixe
`fichiersConnus` conserve ses indices ; les noms dupliqués dans ce préfixe et
un même nom de diagnostic associé à deux chemins distincts sont refusés.

Le graphe est parcouru itérativement, sans exécuter `once` ni sélectionner les
jetons. Chaque identité canonique est lue une fois ; les alias exposent le même
instantané, mais des indices et noms distincts. La normalisation d'identité
est celle du bootstrap : `weakly_canonical`, UTF-8 générique et réduction ASCII
de casse sur Windows. Il ne s'agit pas d'une nouvelle détection des identités
physiques par numéro de fichier. Les modes source/interface suivent les
extensions des fichiers ; le mode global d'analyse d'une unité reste séparé.

Les limites hôtes par défaut sont **4 096 fichiers**, **100 000 liens**,
**16 Mio par fichier** et **64 Mio de textes distincts**. Elles sont
configurables dans les bornes ABI et ne remplacent pas le contrôle de profondeur
de 128 fichiers actifs par le frontend. Les limites concernent le catalogue,
pas un budget global de mémoire du processus. Les erreurs de lecture ou de
limite lèvent une exception hôte, sans publier un catalogue partiel.

`PreparerSourceAvecOrigines` reçoit le catalogue et l'export d'expansion chargé,
avec la convention Microsoft x64 explicite sous GNU/Linux. Il mesure les
capacités, les contrôle, alloue le texte et les origines, puis appelle le même
export pour publier. `SourcePrepareeAvecOrigines` possède les deux sorties et
le résultat ABI. Sur refus du frontend elles sont vides, et le diagnostic est
conservé ; une incohérence de code/capacité/contrat lève une exception hôte.
Les bornes de sortie par défaut sont 64 Mio de texte et un million d'origines.
L'image contenant l'export doit rester chargée pendant les deux appels.

**Limite de l'entrée anticipée :** le lexeur C++ découvre les chemins candidats, et tout
le graphe accessible est lu avant l'expansion Gs++, y compris des fichiers que
`once` ou un refus de directive pourraient ensuite rendre inutiles. Un échec
d'E/S anticipé peut donc précéder un diagnostic de langue du parcours effectif.
Les erreurs lexicales des fichiers restent dans leurs textes pour être
diagnostiquées par le frontend. Cette entrée réutilisable, utilisée par les
matrices de syntaxe/assemblage/sémantique avec origines, ne remplace pas le
parcours par défaut de `gsppc`. Une résolution/lecture à la demande et le raccord
complet au pilote sont distincts ; l'entrée à la demande ci-dessous constitue
une première alternative, sans bascule du pilote par défaut.

### Expansion et lecture à la demande

`DevelopperInclusionsADemande` / `ExpandDeclarationIncludesOnDemand` reçoit une
`RequeteExpansionDeclarationsADemande` de **56 octets** : pointeur vers la
requête historique de 128 octets, tableau de disponibilités 0/1 de
`NombreFichiers` éléments et demande de **40 octets**. L'entrée historique
reste inchangée et ne renvoie pas de suspensions. L'entrée additive ajoute les
codes **15 résolution requise** et **16 lecture requise**, qui ne sont pas des
diagnostics de langue.

Chaque fichier indisponible porte source nulle et taille zéro, mais une identité
et un mode déjà résolus. Une disponibilité autre que 0/1, ou une source publiée
comme indisponible, est un argument invalide. Les métadonnées et liens restent
soumis aux contrôles de l'API historique ; la demande est remise à zéro avant
chaque appel, et seuls les codes 15/16 la renseignent. Les tableaux doivent
rester stables pendant l'appel ; l'hôte peut les reconstruire entre reprises.

La demande expose `Operation`, réserve nulle, `IndexFichier`, `DebutDirective`,
`DebutArgument` et `TailleArgument`. Une résolution (opération 1) porte le fichier
appelant, la position de `#` et la plage brute de la chaîne entre guillemets.
Elle n'intervient qu'après le lexage complet du fichier et la validation de la
directive. Une lecture (opération 2) porte uniquement le fichier à charger,
avec les trois plages nulles. Elle suit les contrôles once/cycle/profondeur,
avant le lexage cible. La racine elle-même est lue à la demande.

Sur suspension, les deux sorties sont intactes, leurs nombres sont zéro,
`IndexFichierErreur=NombreFichiers`, les coordonnées sont nulles et l'arène
est libérée. Après avoir satisfait une demande, l'hôte rappelle l'entrée :
le parcours est **relancé**, il n'existe pas d'état persistant dans l'image.
Les textes/identités/liens fournis doivent former le même instantané à travers
les reprises. La publication finale reste transactionnelle.

`GsPP::PreparerFichierAvecOrigines` implémente ce dialogue côté produit.
Il résout les plages choisies par Gs++ sans utiliser le lexeur C++ pour découvrir
les directives, lit seulement les fichiers demandés, conserve les instantanés
canoniques, contrôle les bornes et refuse les demandes incohérentes/répétées.
Son `ResultatPreparationFichier` possède catalogue et sorties, avec nombres de
lectures physiques, résolutions et appels. `CatalogueInclusions::Disponibles`
distingue les fichiers lus des métadonnées non visitées : un catalogue partiel
est refusé par l'adaptateur historique `PreparerSourceAvecOrigines`.

La reprise relance le lexage et la traversée des fichiers déjà visités : ce
coût reste à optimiser et à mesurer. Les limites hôtes et de sortie restent
applicables ; un fichier visité peut donc provoquer une erreur d'E/S ou de
limite. La priorité des refus de langue avant les fichiers non visités est
testée, mais la parité exhaustive des erreurs du système de fichiers et le
raccord au pilote par défaut ne sont pas revendiqués.

### Session facultative de cache lexical

`DevelopperInclusionsEnSession` / `ExpandDeclarationIncludesInSession` reçoit une
`RequeteExpansionDeclarationsEnSession` de **40 octets** : pointeur vers la
requête à la demande, pointeur opaque `Memoire`, opération/réserve 32 bits et deux
compteurs 64 bits. La requête doit être initialisée à zéro. L'opération **0**
développe avec le même protocole 15/16 ; **1** libère la session, même sans
requête d'expansion imbriquée, remet pointeur et compteurs à zéro et renvoie
succès. Libérer une session vide est permis. Réserve non nulle, opération
inconnue et requête imbriquée absente pour développer sont invalides.

La session conserve les tableaux de jetons des fichiers lexés avec succès,
par index de catalogue, dans une arène privée. Le tableau des caches grandit
géométriquement ; l'état once, les fichiers actifs et le parcours ne sont pas
conservés. La grammaire des directives, l'ordre des demandes et les priorités
des diagnostics sont donc rejoués. Les API historiques de 128/56 octets gardent
leur comportement sans état persistant. Aucun cache global de processus n'est
introduit ; deux sessions possèdent des mémoires indépendantes.

L'hôte **ne copie ni ne modifie le pointeur opaque**. Une session appartient à
un seul utilisateur, sans appels simultanés. Les indices déjà lexés, leurs
adresses de source et leurs contenus restent immuables jusqu'à la libération ;
les tableaux de catalogue/liens et les tampons de sortie peuvent se déplacer
entre appels et le catalogue peut grandir. Un changement d'adresse ou de taille
d'une source déjà cachée, ou son retour à l'état indisponible, est refusé au
moment de sa visite, sans sortie ni diagnostic de langue. Une modification
des octets en place viole le contrat :
il n'y a ni copie de validation ni hachage des sources à chaque reprise.
Pour un autre instantané, libérer puis créer une nouvelle session.

Les sorties restent transactionnelles sur suspension/refus. L'arène transitoire
de l'appel est libérée, mais l'arène des jetons **survit jusqu'à l'opération 1**,
y compris après un échec d'allocation ou un refus. L'hôte doit donc libérer sur
tous les chemins de sortie, avant de décharger l'image GsE ou de retirer les
sources. `NombreOctetsArene` mesure l'arène transitoire, pas la mémoire totale
retenue par la session. `NombreLexages` cumule les lexages complets de fichiers
réussis ; `NombreReutilisations` compte leurs réemplois entre appels. Ces
compteurs ne comptent pas le relexage des fragments par la préparation finale.

`GsPP::PreparerFichierAvecOriginesEnSession` possède cette durée de vie côté
produit : mêmes demandes, mêmes limites et mêmes instantanés stables que
l'adaptateur sans cache ; libération automatique sur succès, refus et exception
hôte. Son résultat conserve les deux compteurs avant destruction de la session.
Le parcours rejoué, les validations de catalogue et le relexage final des
fragments restent à optimiser ; aucun coût global linéaire n'est revendiqué.
Cette variante ne remplace pas encore le chemin par défaut de `gsppc`.

### Reprise persistante du parcours

`DevelopperInclusionsAvecReprise` / `ResumeDeclarationIncludes` reçoit une
`RequeteExpansionDeclarationsAvecReprise` de **48 octets** : pointeur vers la
requête à la demande, pointeur opaque `Memoire`, opération/réserve 32 bits et
trois compteurs 64 bits (`NombreLexages`, `NombreReutilisations`,
`NombreJetonsParcourus`). Initialiser à zéro ; opération **0** continuer,
**1** libérer, même sans requête imbriquée. La libération remet pointeur et
compteurs à zéro ; répéter la libération d'une session vide est permis.
Réserve non nulle, opération inconnue ou requête imbriquée absente pour
continuer sont invalides. Les entrées de 128/56/40 octets restent disponibles
avec leur comportement antérieur.

Cette variante conserve les jetons lexés, la pile itérative de **128 cadres**,
les états once/actifs et les fragments déjà sélectionnés. Le lexage complet
d'un fichier précède sa visite grammaticale. Une demande 15/16 ne valide pas
le déplacement du curseur de la directive : la reprise attend le lien ou la
source, puis reprend au même endroit, sans reparcourir les fichiers précédents.
La mesure exacte et les publications suivantes réutilisent la sélection
terminée, sans nouveau parcours ni allocation de sélection. La préparation
finale des fragments reste lexicale et transactionnelle.

Le catalogue peut **grandir sans retirer ni réordonner ses indices** ; ses
tableaux, les liens triés et les tampons de sortie peuvent changer d'adresse
entre appels. Les liens nouveaux peuvent s'insérer entre les liens connus.
En revanche :

- racine, BOM et mode LF/CRLF sont figés dès le démarrage du parcours ;
- une source déjà lue reste disponible, avec adresse, taille, identité canonique
  et mode inchangés ; ses octets restent immuables en place ;
- l'identité et le mode des fichiers déjà observés sont figés, **y compris un
  alias ignoré par once sans lecture de sa source** ; ceux de la cible d'une
  lecture en attente le sont aussi ;
- chaque lien déjà fourni garde sa clé, son état et, s'il est disponible, sa
  cible. La sentinelle d'un lien indisponible suit le nombre actuel de fichiers.

Un instantané incompatible est refusé avec arguments invalides, sans sortie
ni faux diagnostic de langue. Les instantanés sont contrôlés avant la reprise,
pas seulement à la prochaine visite du fichier. Modifier les octets d'une
source en place viole le contrat : aucune copie de validation ni aucun hachage
de contenu n'est ajouté. Pour un autre instantané, libérer puis recréer la
session. Ne pas copier ni modifier le pointeur opaque ; une session appartient
à un seul utilisateur, sans appels simultanés.

Les sorties restent intactes sur suspension, refus ou allocation impossible.
Le curseur, les fragments et la demande en attente survivent à un échec
d'agrandissement du catalogue : une nouvelle tentative avec l'instantané
compatible peut continuer. Toute la mémoire privée retenue survit jusqu'à
l'opération 1, **également après un refus**. L'hôte doit libérer sur tous les
chemins avant de retirer les sources ou de décharger l'image GsE.
`NombreOctetsArene` mesure ici l'arène retenue du parcours, sans l'arène
lexicale ni l'objet opaque : ce n'est pas la mémoire totale de la session.

`NombreLexages` compte les lexages complets réussis de fichiers et
`NombreReutilisations` leurs réemplois. `NombreJetonsParcourus` compte les
jetons dont le parcours a été **validé** : un jeton ordinaire ou EOF vaut un,
une directive et son argument valent deux. Les tentatives suspendues ou
échouées ne l'incrémentent pas ; ce compteur n'inclut ni les validations et
copies du catalogue, ni le relexage des fragments de préparation finale.
Il ne mesure donc ni tous les accès aux jetons ni le coût total. Les contrôles
et copies d'instantanés à chaque appel peuvent encore produire un coût
quadratique sur une longue suite de demandes : aucun coût global linéaire
n'est revendiqué.

`GsPP::PreparerFichierAvecReprise` utilise le même dialogue de lecture/résolution
et les mêmes limites que les adaptateurs précédents, sans découverte des
directives par le lexeur C++. Il possède la session, la libère automatiquement
sur succès, refus et exception, et conserve les compteurs dans son résultat.
Cette entrée reste facultative ; son raccord au pilote est explicite, décrit ci-dessous.

### Erreurs hôtes de sources et lecture contrôlée

Le bootstrap C++, le catalogue anticipé et les trois adaptateurs à la demande
partagent `IdentifierFichierSource`, `EstFichierSourceRegulier` et
`LireFichierSource`, dans `GsPP/FichiersSource.hpp`. Ces primitives sont des
services **hôtes**, pas de nouveaux exports ou codes d'erreur de l'ABI Gs++.
Les mêmes règles d'identité canonique et de casse ASCII Windows sont conservées.

`ErreurFichierSource`, dérivée de `std::runtime_error`, expose une opération
(`Identification`, `Statut`, `Ouverture`, `Lecture`), le chemin **physique**, un
`std::error_code` et les messages français/anglais avec `Message(LangueDiagnostic)`.
Les contrôles de système de fichiers conservent le code rendu par la bibliothèque
hôte. Les flux utilisent `errno` s'il est disponible, sinon `errc::io_error` :
ce code n'est pas une garantie de disposer du détail natif Windows ou POSIX.
`what()` reste français ; les interceptions générales `std::runtime_error` /
`std::exception` restent possibles. Les anciens types/messages d'erreurs hôtes
non uniformes sont donc harmonisés, sans changer les diagnostics de langue.

Il faut distinguer les situations suivantes :

- une inclusion absente, une cible non régulière (dont un répertoire) ou un
  chemin traversant un parent non répertoire restent le diagnostic **10**
  `FichierIntrouvable`, à la directive dans le fichier appelant ;
- une autre erreur de statut/identification est une exception hôte attachée
  au chemin examiné, pas un diagnostic de langue avec coordonnées inventées ;
- une racine qui ne peut pas s'ouvrir, ou une cible retirée/remplacée après
  sa résolution, échoue à l'ouverture ou à la lecture de sa cible physique ;
- un dépassement de limite d'octets reste `std::length_error`, distinct d'un
  échec du système de fichiers.

Le classement effectif du système de fichiers est conservé : par exemple un
nom très long peut être classé absent par MSVC/NTFS et provoquer une erreur de
statut avec GNU. Le code compare les chemins dans le même hôte ; il ne force
pas les catégories d'un OS sur l'autre. Il n'ajoute pas de garantie atomique
entre contrôle du statut, résolution et ouverture, ni de nouvelle tentative
automatique après une erreur d'E/S.

`LireFluxSource` lit depuis la position d'un flux emprunté, sans le fermer,
par blocs de 8 192 octets. EOF normal, source vide et lecture courte finale
sont acceptés, même avec `failbit`/`badbit` dans le masque d'exceptions du flux.
Un flux en échec sans EOF, `badbit` ou une `ios_base::failure` de lecture
provoque `ErreurFichierSource` : **aucun préfixe partiel n'est retourné**.
Les octets binaires restent inchangés et la limite est contrôlée avant ajout.
`LireFichierSource` ouvre en binaire, puis utilise cette même lecture contrôlée.
Le bootstrap ne traite donc plus une panne de lecture comme un texte partiel
ou une source vide valable.

À la demande, ces exceptions sortent au point choisi par l'expansion Gs++ ;
les sessions lexicales et à reprise sont libérées par leurs adaptateurs, même
si l'exception survient pendant la réponse hôte. Les priorités du lexage et de
la grammaire avant résolution, et once/cycle/profondeur avant lecture, restent
inchangées. Dans `gsppc`, l'erreur hôte de source reste **GS0001 / sortie 1**,
sans coordonnées fictives ; son texte suit `--langue-diagnostics francais|anglais`.
Le diagnostic de langue reste **GS1001 / sortie 2**.

La matrice locale couvre 15 corpus bilingues sur les trois adaptateurs, des
flux défectueux après un bloc, les limites/EOF, des incidents de retrait ou
remplacement après résolution et les verrous exclusifs Windows. Elle ne
revendique pas une couverture exhaustive des ACL, liens symboliques, volumes
distants, périphériques ou de toutes les courses concurrentes. Le chemin
de sélection par défaut du pilote reste celui du bootstrap.

### Expansion Gs++ facultative dans le pilote de fichiers

Dans les sources de développement après alpha.10, `gsppc` accepte
`--expanseur-inclusions <image.GsE>` et son alias anglais
`--include-expander <image.GsE>` pour les entrées directes sources/interfaces,
les projets `.GsPj` / `.GsProject` et les solutions `.GsPs`.
Sans cette option, la sélection des inclusions reste celle du bootstrap C++.
Aucune image n'est découverte automatiquement et aucun repli silencieux vers
le bootstrap n'a lieu si l'image ou son contrat échoue. L'option reste refusée
pour les entrées binaires objets et bibliothèques.

Pour un projet ou une solution, une seule image est chargée avant la
construction et reste possédée jusqu'à sa fin. Le préparateur facultatif est
transmis à `ConstructeurProjet`, à chaque projet de la solution, puis à ses
interfaces et sources, en compilation séparée comme agrégée. Chaque unité
garde un catalogue et une reprise indépendants ; les interfaces d'un projet
séparé sont donc préparées de nouveau pour chaque unité de traduction.
Le chemin de l'image passé en ligne de commande est relatif au répertoire
du processus, contrairement aux chemins XML relatifs au fichier qui les
contient. Aucun élément ni attribut XML de chargement n'est ajouté au schéma 1.0.
Les remplacements `-o` / `--object-directory` restent réservés à un projet ;
format, carte et point d'entrée restent définis par son XML. Ordre des projets,
noms des objets et liaison ne sont pas modifiés par le choix du préparateur.

La construction n'est pas une transaction couvrant toutes les sorties : une
erreur arrête le projet ou la solution, sans annuler les objets ou projets déjà
construits. Un refus avant le premier objet ne remplace pas les sorties
existantes ; un échec plus tardif peut laisser une bibliothèque reconstruite
et l'ancien exécutable du projet suivant. Il n'y a ni poursuite des projets
suivants, ni retour arrière automatique.

L'image fournie doit être **de confiance** : elle contient du code natif exécuté
dans le processus du compilateur, sans bac à sable ni vérification d'authenticité.
Le chargeur spécialisé vérifie GsE 1.0/ABI 1 avant chargement et exige les exports
`DevelopperInclusionsAvecReprise` / `ResumeDeclarationIncludes` du namespace
`GalacticShrine::GsPP::Autohebergement`, fonctions exécutables à la même adresse.
Seuls les imports `AllouerMemoire` et `LibererMemoire` du namespace
`GalacticShrine::GsPP::Hote` sont résolus. Les images sont bornées à 64 Mio de
fichier et 1 Gio de mémoire logique. Les trampolines x86-64 sont proches de
l'image pour les relocalisations relatif32 ; protections de segments W^X et
page de trampolines RX, sans appel du point d'entrée. Le chargement est limité
aux hôtes Windows/Linux x86-64, indépendamment des futures cibles de sortie.
Le format ne décrit pas toute la signature de fonction : la conformité de
l'image à l'ABI de requête de 48 octets reste une obligation du fournisseur.

Chaque unité possède son propre catalogue et sa propre reprise. L'expansion
Gs++ choisit les directives, once, cycles et profondeurs ; l'hôte ne fait que
répondre aux lectures et résolutions demandées. La reprise est libérée avant
déchargement, y compris sur refus ou exception. Le pont
`ConvertirPreparationEnJetons` vérifie tailles, nombre de jetons, indices de
fichiers disponibles, champs réservés, modes source/interface, plages ordonnées,
lexèmes et EOF. Il décode le texte préparé avec le lexeur C++, puis remplace
fichier/ligne/colonne/mode par les origines originales. Il ne redécouvre pas les
directives. Les jetons retournés possèdent leurs textes et noms, sans vues vers
le catalogue ou l'image. Les contrôles et relexages ont un coût supplémentaire ;
aucune accélération globale n'est revendiquée.

Les refus d'expansion sont traduits en diagnostics de langue bilingues. Pour
un refus lexical, le détail C++ est obtenu sur l'instantané original fautif,
sans nouvelle lecture, avec vérification des coordonnées renvoyées. Les erreurs
d'E/S restent hôtes. Un contrat incohérent ne devient pas un faux GS1001.
Les passes syntaxiques, normalisation, sémantique, génération x64 et écrivains
restent celles du pipeline C++, partagé par `AnalyserUnitesAvecPreparation`.
Cette option ne démontre donc pas un compilateur entièrement auto-hébergé.
Le bootstrap reste capable de construire `Frontend.GsE` sans dépendre de
l'existence de cette image.

### Préparation lexicale du texte développé et des origines

`PreparerDeclarationsAvecOrigines` / `PrepareOriginAwareDeclarations` reçoit
une `RequetePreparationDeclarations` de **112 octets** : une liste de
`FragmentJetonDeclarations` de **40 octets**, un nombre de fichiers du catalogue
hôte, deux tampons de sortie (texte et `OrigineJetonDeclarations`), leurs
capacités et deux options 0/1 (`MarqueUtf8`, `FinLigneCrlf`). Le résultat de
**48 octets** expose l'erreur, son détail lexical éventuel, les capacités exactes
en octets/origines et l'indice du fragment/fichier avec ses coordonnées locales.
Les contrats antérieurs ne changent pas.

Chaque fragment non final référence les octets d'un seul jeton original, sans
BOM, commentaire ni séparateur périphérique, avec l'indice du fichier, la ligne,
la colonne et le mode source/interface. Le dernier fragment est vide et décrit
l'EOF de l'unité racine. La préparation copie les lexèmes sans décoder/réencoder
les chaînes, puis ajoute LF ou CRLF après chaque jeton non final. Le BOM de sortie
est optionnel ; les plages d'origine n'incluent ni BOM ni séparateurs. L'EOF est
localisé exactement à la taille du texte, y compris pour une unité vide.

Tous les fragments sont validés lexicalement **avant toute publication**.
Les deux sorties sont transactionnelles : aucun préfixe n'est publié en cas
d'argument invalide, fragment mal formé, erreur lexicale, directive non développée,
taille excessive ou capacité insuffisante. L'appel de mesure valide aussi les
entrées ; sur capacité insuffisante, les deux nombres de sortie sont exacts,
les indices d'erreur valent les nombres de fragments/fichiers et les coordonnées
sont nulles. Un refus d'entrée annule ces deux nombres et identifie le fragment
fautif lorsque disponible. Les coordonnées lexicales sont rébasées dans le fichier
original sans troncature ; un dépassement de `naturel32` est refusé comme argument.

Les erreurs sont `Reussite=0`, `CapaciteInsuffisante=1`, `ArgumentInvalide=2`,
`ErreurLexicale=3`, `FragmentInvalide=4`, `DirectiveNonDeveloppee=5` et
`TailleExcessive=6`. Le nombre de fragments est compris entre 1 et 100 millions,
celui des fichiers entre 1 et 1 million ; le texte préparé ne dépasse pas
1 milliard d'octets. Les métadonnées et tailles sont contrôlées avant l'accès
au fragment ; la borne cumulée avant son lexage. Entrées, sorties et requête
doivent être des zones distinctes, valides et stables pendant l'appel.

Cette préparation n'alloue rien et n'utilise aucun service de fichier. Les
directives restantes `#inclure` / `#include` et `#pragma` sont refusées.
La sélection peut être fournie par l'hôte ou par l'entrée d'expansion ci-dessus ;
lecture et résolution des chemins restent côté hôte. Les indices et coordonnées ne sont pas
vérifiés contre un catalogue de fichiers par cette API. Les tests raccordent
cette préparation aux analyses avec origines et à l'assemblage/normalisation/
sémantique multi-unités ; le pilote de compilation de fichiers de `gsppc`
n'est pas encore remplacé.

### Analyse d'une unité développée avec origines de jetons

`AnalyserDeclarationsAvecOrigines` / `AnalyzeOriginAwareDeclarations` reçoit
une requête additive de 56 octets, référençant la requête syntaxique existante
de 80 octets et une table de `OrigineJetonDeclarations` de 40 octets par jeton.
Le texte est déjà développé, par l'hôte ou par l'entrée d'expansion. Chaque enregistrement indique sa plage
en octets dans ce texte, l'indice du fichier d'origine, sa ligne/colonne locales
et son mode source/interface. La table contient exactement tous les jetons,
y compris la fin de texte de taille nulle. Les plages sont comparées au lexage
réel avant publication de l'AST ; les indices, coordonnées, bornes, modes et
champs réservés sont contrôlés. Le catalogue des noms de fichiers reste à l'hôte.

Le mode dépend du jeton décisif de la déclaration, comme le bootstrap, même
lorsqu'une signature traverse une inclusion ; le mode global d'interface prime
sur les modes locaux. Une inclusion textuelle reste dans la même unité de
traduction que son consommateur, notamment pour les imports d'espaces.
L'AST garde ses positions dans le texte développé ; les refus syntaxiques
exposent en plus l'indice du fichier et les coordonnées locales.

`LocaliserOrigineDeclarationsPreparees` / `LocatePreparedDeclarationOrigin`
traduit une position synthétique de début de jeton, fin comprise, en position
locale, notamment après une analyse sémantique. Il exige la même table validée
et inchangée ; il contrôle ses bornes mais ne relance pas le lexage. Une position
dans un espace ou à l'intérieur d'un jeton n'est pas traduite. Chaque appel
réinitialise les résultats locaux ; sans origine, l'indice vaut `NombreFichiers`
et la ligne/colonne valent zéro. La localisation n'alloue rien.

Le contrat de capacité est celui de l'analyse syntaxique historique : mesure
exacte et préfixe d'AST possible si le tampon est partiel. Ce n'est pas le contrat
transactionnel à trois sorties de l'assemblage/normalisation. Lecture et résolution
des chemins restent côté hôte ; l'entrée d'expansion ci-dessus traite les directives,
`#pragma once` et cycles avant cette analyse.
Cette entrée ne normalise pas les déclarations ; les entrées d'assemblage
avec origines ci-dessous utilisent le même contrat de jetons pour chaque unité.
Voir les [preuves différentielles](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#origines-des-inclusions-préparées--6-octobre-2026).

### Adaptateur hôte d'AST syntaxique possédé

Les sources de développement après alpha.10 proposent
`AnalyserDeclarationsPreparees` dans `Compiler/include/GsPP/DeclarationsPreparees.hpp`.
Cette API hôte reçoit un `ResultatPreparationFichier` déjà développé et le
pointeur de l'export syntaxique avec origines. Elle vérifie la préparation avec
le pont lexical existant, **sans appeler l'analyseur syntaxique C++**, puis
effectue une mesure et une publication de l'AST Gs++.

`ExpanseurInclusionsCharge::AnalyseurDeclarations()` vérifie à la demande la
paire d'exports `AnalyserDeclarationsAvecOrigines` /
`AnalyzeOriginAwareDeclarations` : même adresse, fonctions, section de code,
taille non nulle et plage dans un segment exécutable. Ces exports ne sont pas
exigés par le chargement pour l'expansion seule. L'image native doit être de
confiance et rester chargée pendant les appels ; il n'y a pas de bac à sable.
Les miroirs ABI restent des nœuds de 64 octets, résultats de 48 octets,
requêtes syntaxiques de 80 octets et requêtes avec origines de 56 octets.

Le résultat `DeclarationsPreparees` possède le texte développé, les nœuds
compacts et les noms/positions originales de chaque nœud. Les coordonnées dans
les nœuds restent synthétiques ; `Origines` identifie le fichier original,
l'unité de traduction, le rang de jeton et son mode source/interface, auquel
prime le mode global d'interface. La racine synthétique du programme reçoit
le nom et la position 1:1 de l'unité racine. `Nom(index)` donne une vue bornée
dans le texte possédé, valable pendant la vie de ce résultat.
Le mode d'origine d'un nœud décrit son jeton de position ; il ne remplace pas
le mode du jeton décisif utilisé pour classer une déclaration.

Les entrées natives sont des copies privées du texte et des origines. Leurs
contenus et pointeurs sont vérifiés après chaque appel. La publication exige
les mêmes capacités mesurées, des métadonnées cohérentes, un arbre enraciné
avec parents antérieurs, genres/drapeaux connus, noms bornés et positions de
jetons existantes. Un parcours du texte et des recherches binaires retrouvent
les origines ; aucune lecture de fichier ni parcours depuis le début par nœud.
Les défauts de contrat restent des exceptions hôtes, pas de faux GS1001.

Un refus syntaxique publie uniquement le résultat numérique et une position
originale vérifiée, **sans texte ni arbre partiel**. `DeclarationsPreparees::ExigerValide()`
traduit ce refus en `ErreurCompilation` bilingue à partir du code et du contexte
`Detail`, sans lecture ni appel natif supplémentaire ; aucun effet sur un succès.
Les erreurs d'expansion et
lexicales antérieures gardent le traitement du pont ; une allocation impossible
est propagée comme exception hôte. Limites hôtes par défaut : 64 Mio de texte,
un million de jetons et un million de nœuds. Les deux premières sont contrôlées
avant appel natif ; celle des nœuds après mesure, avant allocation de sortie.
Elles ne constituent pas un quota ni une isolation du code natif ou de son arène.

L'AST et ses noms restent disponibles après destruction du catalogue ou
déchargement de l'image, ainsi qu'après copie/déplacement du résultat.
`HachageType` reste opaque : cet arbre ne remplace pas encore le `Programme`
riche du backend C++. L'API n'est pas activée par une nouvelle option CLI et
ne change ni le bootstrap par défaut ni le contrat de `--expanseur-inclusions`.
Elle n'établit pas le raccord complet des passes ni un gain de performances.

#### Messages syntaxiques possédés

Le champ existant `ResultatAnalyseDeclarations.Detail` porte, pour les codes
syntaxiques 5 à 30, les contextes additifs 1 à 43 de
`DetailDiagnosticDeclarations`. Les noms, paramètres, signatures de pointeur
de fonction, tableaux, délimiteurs, alias, utilisations d'espaces et initialiseurs
de constructeurs distinguent ainsi leurs messages. La valeur 0 conserve le
message générique de la catégorie historique ; une ancienne image sans détails
ne restitue donc pas nécessairement le message précis du bootstrap.
Les tailles des miroirs ABI, les codes `Erreur` et la convention d'appel restent
inchangés. Les erreurs lexicales antérieures conservent la signification lexicale
de leur détail et sont traitées par le pont avant l'analyse syntaxique.

Un détail inconnu ou incompatible avec son code est refusé comme contrat hôte,
avant toute publication d'un résultat de langue. `ExigerValide()` utilise la
position originale possédée, pas la position synthétique ni un texte relu ;
ses deux messages restent disponibles après copie/déplacement du résultat et
déchargement de l'image. Ils peuvent être rendus par le contrat `GS1001` actuel.
L'API ne s'active pas automatiquement dans `gsppc` et ne rend pas les inclusions
ou constructions transactionnelles. Les comparaisons exactes et les priorités
restent limitées aux corpus validés, pas une preuve de parité de toute entrée.

### Adaptateur hôte d'assemblage multi-unités possédé

`AssemblerDeclarationsPreparees`, dans
`Compiler/include/GsPP/AssemblageDeclarationsPreparees.hpp`, reçoit un ensemble
ordonné de pointeurs non nuls vers des `ResultatPreparationFichier` et un export
brut ou normalisé avec origines. Le pont lexical valide toutes les préparations
avant tout appel natif ; les modes globaux viennent des racines des catalogues.
Les catalogues locaux sont remappés dans un catalogue commun privé, sans lecture
ni résolution de fichier supplémentaire. Le frontend Gs++ réanalyse les textes
préparés ; cette entrée ne consomme pas les AST mono-unités déjà produits et
**n'appelle pas le parseur syntaxique C++**.

`ExpanseurInclusionsCharge::AssembleurDeclarations(false)` vérifie la paire
d'exports d'assemblage brut ; `true` (par défaut) choisit celle de normalisation.
Mêmes contrôles de fonction, adresse FR/EN, section, taille et segment exécutable
que l'entrée syntaxique. Les exports restent facultatifs pour l'expansion seule.
Les miroirs ABI d'assemblage sont centralisés : unité de 24 octets, plage de
32 octets, résultat de 64 octets, requête de 128 octets, table de 16 octets et
requête avec origines de 40 octets. Aucune nouvelle ABI native.

`DeclarationsAssemblees` possède `Source`, `Noeuds`, `Unites` et les origines
par nœud. `Unites` décrit les plages de texte après suppression des BOM par
unité et ajout d'un LF après chaque unité, dernière comprise. La normalisation
ne change pas ce texte. `Origines` garde fichier, ligne, colonne, nom d'unité,
rang du jeton et mode de son jeton de position ; `IndexUnite` identifie l'unité
retenue après sélection/reclassement. La racine commune, qui n'a pas de position
originale, utilise une position vide et l'indice sentinelle `Unites.size()`.
`Nom(index)` fournit une vue bornée dans le texte possédé, liée à la durée de vie
du résultat. Les positions synthétiques des nœuds restent celles du texte assemblé.

La mesure puis la publication utilisent des copies privées du texte, des tables
et des origines. Entrées inchangées, nombres mesurés, texte attendu, plages exactes,
parents antérieurs, genres/drapeaux connus, noms bornés dans leur unité et positions
de jetons sont contrôlés avant publication. Chaque sortie a une garde supplémentaire.
Le compteur `NombreOctetsArene` reste informatif : l'assemblage brut peut allouer
davantage en publication qu'en mesure, contrairement aux capacités exactes des sorties.

Sur un refus situé, seuls `Resultat` et `PositionErreur` sont publiés. Pour
`Erreur == 4`, `Detail` porte le code syntaxique et le champ historique nommé
`DetailLexical` porte son contexte syntaxique, validé comme dans l'API mono-unité.
Pour les conflits 6 à 10, cette entrée historique reste numérique avec la position
originale et le rang d'unité. L'entrée additive décrite ci-dessous fournit aussi
le nom qualifié et les messages bilingues. Les codes/positions incohérents restent des exceptions
hôtes, tout comme les défauts de contrat ; une allocation impossible conserve
`std::bad_alloc`.

Limites cumulées par défaut : 64 Mio d'entrées (BOM inclus et un octet séparateur
par unité), un million de jetons/nœuds/fichiers et 1 024 unités. Les entrées sont
bornées avant appel ; la limite de nœuds s'applique après mesure, avant allocation
des sorties. Ces bornes ne constituent pas un quota de l'arène native ni un bac
à sable. L'image de confiance doit rester chargée pendant les appels. Les sorties
possédées restent ensuite valables indépendamment des fichiers, des catalogues et
de l'image ; la copie/déplacement est vérifiée sur les résultats normalisés.

API de développement seulement : `HachageType` opaque, aucun remplacement de
`Programme`, aucune nouvelle option CLI ni activation automatique du frontend.
Le raccord sémantique possédé décrit ci-dessous prolonge cette API ; le backend
reste à réaliser. Voir le
[bilan de validation](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#assemblage-multi-unités-possédé--7-octobre-2026).

### Diagnostics d'assemblage possédés

L'export additif `AssemblerDeclarationsNormaliseesAvecDiagnostic` /
`AssembleNormalizedDeclarationsWithDiagnostic` reçoit `RequeteDiagnosticAssemblage`
de 40 octets : pointeur vers l'assemblage avec origines, tampon `Nom`,
`CapaciteNom`, `NombreOctetsNom` et `Reserve` obligatoirement nul. Les structures,
codes et signatures historiques restent inchangés, en ABI 1.

Sur les conflits 6 à 10, `NombreOctetsNom` mesure le nom qualifié UTF-8 exact,
sans terminateur. Un tampon nul ou insuffisant reste intact et ne change pas
le code du conflit. Un tampon suffisant reçoit seulement les octets du nom ;
les trois sorties d'assemblage restent intactes sur tout refus. Succès, mesure
d'AST et refus syntaxique donnent une taille de nom nulle. Les noms proviennent
des chemins lexicaux natifs, avant destruction de l'arène : espaces, classes,
fonctions, globales, alias, noms canoniques `$constructeur`, `$destructeur`
et `operator` suivi du symbole. Aucun nom n'est reconstruit par un parseur C++.

Le chargeur de production vérifie cette paire à la demande avec
`AssembleurDeclarationsAvecDiagnostic()`. L'adaptateur hôte
`AssemblerDeclarationsNormaliseesAvecDiagnostic` possède `NomConflit` et
la position originale dans `DeclarationsAssemblees`. Un conflit impose une
seconde analyse, après les contrôles de la première, pour copier le nom dans
un tampon borné et gardé. Code, coordonnées, origine et taille doivent rester
cohérents entre appels. La limite de nom est de 1 Mio par défaut, distincte
du budget de texte ; zéro ou plus de 999 999 999 octets est invalide.
Pointeurs/capacités modifiés, taille incohérente, garde écrasée, nom vide,
contrôles ASCII, espaces et UTF-8 invalide restent des défauts de contrat hôte.

`DeclarationsAssemblees::ExigerValide()` ne fait ni lecture ni appel natif.
Un succès ne lève rien ; un refus syntaxique réutilise ses contextes bilingues ;
un conflit avec nom lève l'`ErreurCompilation` exacte dans le fichier original.
Un conflit numérique historique sans nom ne peut pas produire ce diagnostic
exact et lève une erreur de contrat hôte. Messages, noms et positions restent
possédés après destruction des préparations et déchargement de l'image.
Cette API de développement ne remplace toujours pas les passes du pilote.

### Sémantique assemblée possédée

`AnalyserSemantiqueAssemblee` reçoit un `DeclarationsAssemblees` réussi et
l'export de confiance `AnalyserSemantiqueUnites` / `AnalyzeUnitSemantics`.
Le getter `ExpanseurInclusionsCharge::AnalyseurSemantiqueUnites()` vérifie cette
paire à la demande ; l'expansion seule ne requiert pas cet export. Les contrats
natifs historiques restent inchangés : symboles de 48 octets, résolutions de
32 octets, résultat de 56 octets, requêtes de 120 et 40 octets, ABI 1.

Le résultat `SemantiqueAssemblee` possède une copie complète des déclarations
d'entrée, puis les symboles et résolutions publiés après une mesure sans sorties.
Leurs indices référencent cet AST possédé. L'appel utilise des copies privées
du texte, de l'arbre et des plages d'unités ; il ne relit aucun fichier, ne
requiert aucun catalogue et ne lance aucune passe syntaxique/sémantique C++.
Il n'effectue pas de normalisation implicite : l'appelant fournit normalement
l'assemblage normalisé. Les unités de traduction gardent leur isolation.

Un refus sémantique numérique conserve l'entrée complète et sa position
originale, retrouvée dans les origines possédées par nœud ; les éventuels
préfixes natifs de symboles/résolutions ne sont jamais exposés. Aucun message
sémantique bilingue complet ni `Programme` C++ n'est reconstruit à ce stade.
`HachageType` reste opaque ; sur une résolution marquée `EtapeDureeVie`
(32768), il représente un décalage et non le type du symbole.

Le code 20, `FonctionAbsente`, est un refus global à la racine (1,1), comme
dans le bootstrap ; aucune position de fichier n'est inventée. Dans ce cas
`PositionErreur` est absente et `IndexUniteErreur` vaut le nombre d'unités.
Le code de résultat, et non la présence d'une position, distingue refus et succès.
Source vide, espace seul et import valide sans fonction restent donc refusés ;
un import absent est toujours diagnostiqué avant l'absence de fonction.

`SemantiqueAssemblee::PeutTraduireDiagnostic()` indique si un message bilingue
invariant, contextuel ou avec nom possédé est disponible ; ce n'est pas un
contrôle de validité du résultat. `ExigerValide()` contrôle le résultat possédé : retourne sur succès,
lève `ErreurCompilation` pour un refus pris en charge, ou une erreur hôte pour
un contrat incohérent ou une famille encore numérique. La traduction ne relit
aucun fichier et n'appelle ni export natif ni analyse syntaxique/sémantique C++ ;
elle reste utilisable après copie, déplacement et déchargement de l'image. Les positions des refus
situés correspondent aux origines de l'AST ; le code 20 reste sans fichier.

Les messages invariants pris en charge couvrent les codes 16, 20, 39, 42–44, 47–52,
54, 60–61, 63–68, 70–72, 74, 76–80, 83, 85–89, 94–97, 101, 113, 115 et 122–125.
Les codes 39/85–88/101/113/115 couvrent le constructeur requis pour les champs
par défaut, les valeurs d'énumération non entières/non constantes/hors plage
et le débordement de la suivante, les références de tableaux interdites,
l'héritage non public et l'auto-héritage. Le message 113 reste indépendant de
la version du produit ; seule la mention historique `Gs++ 0.22` est supprimée
du bootstrap FR/EN, sans changement de la règle d'héritage public. Aucune
résolution hôte de type ou de nom n'est nécessaire à ces messages fixes.

Le **code 16** traduit le message invariant d'un paramètre déclaré plusieurs
fois. Les **codes 12/13/15/17** nomment respectivement le champ, l'alias de champ,
l'énumérateur ou le local répété. Leur `Detail` désigne le nœud AST de la
déclaration précédente : genres, même unité, ordre antérieur et portée
structurelle doivent correspondre au refus. Pour un local, la cible est un
paramètre ou un local visible dans la même fonction ; les blocs frères et
branches indépendantes ne sont pas des portées ancêtres. Les deux tranches
de noms simples sont bornées, lexées et comparées exactement, puis contrôlées
par leurs hachages. Ces contrôles vérifient le choix natif sans rechercher de
déclaration candidate ni résoudre à nouveau les noms ou types. Ils ne prouvent
pas l'authenticité d'un résultat modifié par l'appelant. Pour une énumération,
le doublon est refusé avant son propre initialiseur ; les erreurs des valeurs
précédentes gardent leur priorité, comme dans le bootstrap.

Le **code 14** traduit `alias de champ en conflit avec un champ : NOM` et
`field alias conflicts with a field: NAME`. L'origine du refus est celle de
l'alias (nœud 11) ; `Detail` désigne le champ direct (nœud 7) du même agrégat,
même lorsqu'il est déclaré après l'alias. Les deux noms simples possédés doivent
être identiques après lexage et contrôle des hachages ; les tranches doivent
être distinctes et rester dans la même unité. Aucun champ candidat n'est
recherché dans l'hôte, ni dans les bases ; un champ homonyme d'un autre agrégat
est rejeté comme cible incohérente. Par agrégat, les noms de champs sont
contrôlés avant les noms d'alias, puis les alias dans leur ordre de déclaration :
le conflit avec un champ précède le doublon du même alias courant. La résolution
des cibles intervient ensuite. Ce contexte exige l'hôte et l'image native
courants ensemble, sans changer la disposition d'ABI ni les formats.

Le **code 107** traduit `cycle d’alias de champ détecté pour NOM` et
`field alias cycle detected for NAME`. L'origine du refus désigne l'alias
revisité dans le cycle (nœud 11), dont le nom simple possédé est borné, lexé
et contrôlé par son hachage. `Detail` est le **rang de symbole de l'alias
d'entrée**, pas un indice AST ; il est converti selon l'ordre natif des genres
symboliques, sans conserver les préfixes de symboles sur refus. L'entrée peut
être un alias d'un préfixe acyclique, distinct de l'alias revisité. Son nom
doit également être cohérent et appartenir à la même unité et au même agrégat.
La traduction ne résout aucune cible et ne reconstruit pas le graphe des alias ;
elle ne constitue pas une preuve d'authenticité du cycle déclaré par le résultat.
Le **code 108** traduit `champ cible d’alias introuvable : NOM` et
`unknown field alias target: NAME`. L'analyseur de déclarations ajoute un
enfant nominal **37**, `CibleAliasChamp` / `FieldAliasTarget`, immédiatement
après l'alias de champ 11. Ce nœud possède la tranche et le hachage du nom cible,
sa position et son espace, sans créer de symbole ou d'expression évaluée.
`Detail` désigne cet **indice AST**, tandis que la position du refus reste
celle de l'alias dont la cible est introuvable, même dans une chaîne.
L'hôte exige cet enfant unique, feuille du même agrégat et de la même unité,
ses drapeaux/type nuls et un hachage de nom identique au `HachageType` de l'alias.
Les noms simples de l'alias et de sa cible sont bornés et lexés séparément ;
leurs origines sont cohérentes et le rang du jeton cible suit les trois jetons
`alias`, nom et `=`. Aucun champ candidat ou champ de base n'est recherché,
aucun nom n'est reconstitué depuis un hachage. Un ancien AST sans enfant nominal
conserve `Detail 0` et le refus numérique, sans traduction approximative.
Les contrôles de cohérence ne prouvent pas l'authenticité d'un résultat modifié.
Les priorités couvertes entre doublons, conflits champ/alias, cycles et cibles
introuvables sont comparées au bootstrap, sans nouvelle règle globale entre
toutes les passes. L'ajout nominal modifie l'image native et le contenu de l'AST,
pas la taille de ses nœuds (64 octets), les formats ou l'ABI native.

Le **code 109** traduit `cycle d’alias détecté pour NOM` et
`alias cycle detected for NAME` avec le nom complet de l'alias revisité (nœud 10),
à la racine ou dans un namespace. Son nom possédé peut être qualifié,
comme `API::Vue`. Les identifiants du namespace sont des feuilles nominales
**39**, `SegmentEspaceAlias` / `AliasNamespaceSegment`, enfants de l'alias après
sa cible 38, dans l'ordre extérieur vers intérieur. Chaque `HachageEspace`
porte le préfixe cumulé. Les identifiants possédés sont joints par `::` et les
hachages des noms/préfixes contrôlés ; aucun symbole ni expression n'est ajouté.
`Detail` est le **rang de symbole de l'entrée**, pas un indice AST.
Entrée et alias revisité peuvent différer et appartenir à deux unités ou espaces
différents. L'hôte contrôle leurs genres, parents racines, enfant nominal 38
canonique éventuel (ou ancien AST sans enfant), segments 39 contigus sans descendants,
drapeaux, tranches et origines dans la même unité, lexicalement avant l'alias,
sans rechercher leurs cibles ni reconstruire le
graphe. Un ancien AST sans segments 39 reste numérique pour un alias signalé
dans un namespace, avec le même rang et la même position ; les alias à la racine
restent traduisibles. Aucune inversion de hachage
ni analyse syntaxique/sémantique hôte ne restitue ce nom absent.
Le code 111 reste numérique. Les priorités couvertes sont comparées
au bootstrap ; pas de nouvelle règle globale entre toutes les passes ni de
preuve d'authenticité d'un résultat modifié. L'ajout 39 modifie l'image native
et le contenu AST, pas les nœuds de 64 octets, les formats 1.0 ni les tailles ABI.

Le **code 110** traduit `cible d’alias introuvable : NOM` et
`unknown alias target: NAME` à la position possédée de l'alias courant.
Le nom **écrit** de sa cible vient d'un enfant nominal **38**, `CibleAlias` /
`AliasTarget`, immédiatement après l'alias 10 ; `Detail` est l'indice AST de
cet enfant, pas un rang de symbole. Cette forme fonctionne aussi pour un alias
global déclaré dans un namespace : aucun préfixe résolu n'est nécessaire au
message de cible introuvable. Elle n'ajoute ni symbole ni expression évaluée.
Genre, parent, drapeaux/type nuls, hachages, tranches et origines sont contrôlés,
avec la même unité et aucun enfant du nœud nominal. Le rang lexical cible suit
celui du mot-clé `alias` de 3 + 2 par `::` dans le nom d'alias. Les noms possédés sont
lexés pour concaténer identifiants et `::`, en ignorant espaces/commentaires ;
aucun import, cible, type ou graphe n'est résolu par l'hôte. Un ancien AST sans
enfant 38 conserve `Detail 0` et le refus numérique. Les contrôles de cohérence
ne prouvent pas l'authenticité d'un résultat modifié. Les priorités couvertes
restent comparées au bootstrap, sans nouvelle règle globale entre les passes.
Le nouveau contexte nominal modifie l'image native et le contenu AST, pas les
nœuds de 64 octets, les codes/exports, formats 1.0 ou tailles ABI.

Le **code 112** traduit `un alias de fonction surchargée est ambigu : NOM` et
`an overloaded function alias is ambiguous: NAME` depuis la cible écrite dans
l'enfant nominal 38 de l'alias à l'origine possédée. `Detail` reste le nombre
de surcharges : il doit être au moins 2 et ne pas dépasser le nombre total de
nœuds de fonction. La borne et les noms possédés ne prouvent pas l'authenticité
des candidats annoncés ; aucune fonction ou surcharge n'est recherchée par
l'hôte. Genre, parent, tranches, hachages, unités et rang lexical sont contrôlés
comme pour le code 110. L'ancien AST sans enfant 38 reste numérique avec son
nombre de surcharges inchangé. Pas de nouvel enfant AST, code, export, format
ou ABI ; traduction hôte uniquement, sans changement de l'image native.
Le code 111 reste numérique faute de matrice source différentielle établie pour
sa traduction ; les contrôles de conflits et d'imports gardent leurs priorités.

Le **code 120, Detail 0** traduit `espace utilisé introuvable : NOM` et
`used namespace not found: NAME` à la position originale du mot-clé
`utilisant` / `using`. Le nom **écrit** vient du nœud existant **36**,
`UtilisationEspace` / `UsingNamespace` : il peut être relatif, qualifié,
commenté ou UTF-8 et n'exige pas un nom de namespace résolu. L'hôte contrôle
une feuille racine unique à l'origine du refus, les drapeaux/type nuls, les
tranches dans l'unité, les origines et le hachage du nom, en lexant uniquement
sa tranche possédée. Cette phase a déjà indexé tous les symboles, éventuellement
aucun, mais n'a produit aucune résolution. `Detail` doit rester nul.
Aucune recherche de namespace ou résolution d'import, relecture de fichier,
inversion de hachage ni reconstruction de `Programme` n'est effectuée par l'hôte.
Une incohérence reste une erreur hôte ; les contrôles n'authentifient pas un
refus modifié par l'appelant. Les priorités couvertes restent celles du bootstrap,
sans nouvelle règle globale entre toutes les passes. Traduction hôte uniquement :
nœud 36, image native, codes/exports, formats 1.0 et tailles ABI inchangés.

Le **code 121, Detail 0** traduit `nom importé ambigu : NOM` et
`ambiguous imported name: NAME` depuis un nom **écrit** possédé : cible d'alias
global dans l'enfant 38, à la position de l'alias, ou référence d'expression 24
à la position du nom. La cible d'alias suit les contrôles du code 110, avec
rang lexical, tranche, unité, origines et hachages ; la référence exige une
feuille unique sans drapeaux/type, un parent antérieur, des origines cohérentes
et une tranche dans l'unité. Les ancêtres partageant la position, notamment
l'appel et sa cible, sont distingués des références concurrentes.
Le nombre de symboles est contrôlé ; `Detail` reste nul, sans compter les candidats.
Contrairement au code 120, cette famille ne suppose pas l'absence de résolutions.
L'hôte lexe uniquement les noms possédés, sans rechercher d'import ou reconstruire
les candidats. En héritage, le nœud nominal **40, BaseClasse / ClassBase** est
l'enfant immédiat de la classe 6 : tranche écrite qualifiée, hachage du nom égal
au `HachageType` de la classe, même espace, drapeaux et type nuls, sans symbole
ni enfant évalué. Le **code 121, Detail = index du nœud de genre 40** désigne cette base et
conserve la position de la classe. Parent racine/public, feuille, tranche, unité,
origines et nombre complet de symboles sont contrôlés, dans la phase d'héritage
sans résolutions. Le lexage du nom et de la séparation `:` / `: publique` ou
`: public` vérifie aussi le rang lexical exact, commentaires compris ; il ne
reconstruit aucune recherche de type ou d'import. Les anciens AST sans enfant
40 gardent `Detail 0` et un diagnostic numérique.
Les types nommés directs des déclarations 1/2/3/7/12/15/19 possèdent désormais
un enfant immédiat **41, NomTypeDeclare / DeclaredTypeName**, avant paramètres,
corps, initialiseur ou arguments. Sa tranche écrite est qualifiée ; le hachage
du nom est distinct de `HachageType`, empreinte opaque du type source complet
copiée depuis le parent, avec qualificatifs/pointeurs/dimensions. Même espace,
aucun enfant ni symbole évalué. `Drapeaux` conserve le décalage lexical depuis
le premier jeton de la déclaration ; ce n'est pas un masque de propriétés.
Le **code 121, Detail = index du nœud de genre 41** nomme ce type et conserve
la position de sa déclaration, y compris méthodes et paramètres de constructeurs.
Parent/feuille, bornes dans l'unité, nom/hachages, origines, nombre de symboles
et différence des rangs sont contrôlés ; un lexage borné du préfixe déjà possédé
vérifie également le nombre exact de jetons. Pas de résolution de type/import hôte.
Une ambiguïté de globale attend sa déclaration, après signatures et contrôles
des classes ; chaque globale est ensuite traitée entièrement avant la suivante.
Globales et locales peuvent donc avoir des résolutions antérieures ; les autres
déclarations typées restent dans une phase sans résolutions. Le nœud 41 n'est
pas compté comme expression ou argument d'initialiseur/construction.
Les types nommés dans une signature de callback sont conservés dans des feuilles
**42, NomTypeCallback / CallbackTypeName**, enfants directs contigus du propriétaire
1/2/3/7/12/15/19, avant ses autres enfants. Ordre lexical retour puis paramètres,
callbacks imbriqués compris. Tranche qualifiée, hachage du nom, même espace,
empreinte opaque du type source complet du propriétaire et décalage lexical
`Drapeaux` sont conservés. Les feuilles n'ont ni enfant ni symbole évalué ; elles
ne comptent pas comme expressions/arguments. Elles sont ordonnées sans chevauchement,
avec rangs strictement croissants et prédécesseurs cohérents. Le **code 121,
Detail = index du nœud de genre 42** désigne le nom ambigu exact, même après des
noms valides, à la position de la déclaration. Le lexage borné du préfixe vérifie
le rang et la présence du contexte callback, sans résolution hôte de type/import.
La résolution native conserve le premier échec : retour puis paramètres,
récursivement, sans remplacement par un échec plus tardif. Les autres priorités
et clés de types restent inchangées. Les anciens AST sans feuilles 42 gardent
`Detail 0` et un refus numérique à la même origine.
Les types nommés des cibles de `convertir<…>` / `cast<…>` utilisent les feuilles
**43, NomTypeConversion / CastTypeName**, enfants directs contigus de l'expression
31 avant son unique opérande. Même collecte récursive que les callbacks, nom
qualifié, hachage, espace, empreinte opaque du type cible complet et décalage
lexical depuis le mot-clé de conversion. Aucun symbole, enfant ou opérande évalué
n'est ajouté par ces feuilles. Le **code 121, Detail = index du nœud de genre 43**
nomme le type ambigu exact à la position de la conversion, même dans un callback
imbriqué ou après des noms valides. Parent sans nom propre, unité/bornes,
feuilles/prédécesseurs/ordre, origines/rangs et opérande unique ultérieur sont
contrôlés. Le lexage borné du préfixe confirme mot-clé, chevron et rang exact,
sans résolution hôte de type/import. Les ancêtres partageant la position du
refus sont autorisés, les expressions concurrentes ne le sont pas.
Le type cible est validé avant son opérande ; les autres priorités internes
des expressions restent inchangées. Un ancien AST sans feuilles 43 conserve
`Detail 0`, un diagnostic numérique et la même origine.
Les autres contextes et anciens AST sans enfant 41/42/43 ou cible d'alias 38
restent numériques : le nom d'une déclaration
ne remplace pas le nom ambigu de son type. Aucun contexte nominal 41 n'est
émis pour les types primitifs, `vide` ou le type callback lui-même.
Contrôles de cohérence, pas d'authentification d'un refus modifié. Traduction
hôte sans nouvelle règle de recherche ou de priorité. L'ajout du contexte nominal
40/41/42/43 change l'AST produit et l'image native, mais conserve les codes/exports,
formats 1.0 et la disposition publique de 64 octets du nœud AST.

Le **code 18, Detail 0** peut traduire `variable ou fonction introuvable : NOM`
et `unknown variable or function: NAME` depuis une référence unique (nœud 24)
à la position possédée du refus. Le nom vient de sa tranche `DebutNom` /
`TailleNom`, bornée dans le texte et l'unité possédés, jamais d'une inversion de
hachage. Le lexeur hôte traite uniquement cette tranche pour concaténer les
identifiants et `::`, en ignorant espaces et commentaires ; `soi` / `this`
devient `soi`, comme dans le bootstrap. Le hachage vérifie ensuite la cohérence
du texte ; il ne choisit pas un nom. Aucun fichier ni catalogue n'est requis.

Une tranche lexiquement invalide, un nom vide ou non conforme, un hachage
incohérent ou plusieurs références candidates ne produisent pas de diagnostic
source. `PeutTraduireDiagnostic()` ne fait pas ce lexage ni une validation du
contrat ; `ExigerValide()` peut donc refuser des métadonnées altérées même si
la capacité est annoncée. Les références réservées `parent` / `super` (drapeau
65536) restent numériques : leurs messages de contexte ne sont pas ceux d'un
nom introuvable. Cette traduction ne résout pas les types, les symboles ou les
imports, et ne reconstruit pas `Programme`.

Le **code 19** réutilise cette tranche possédée pour le diagnostic d'adresse de
fonction surchargée ambiguë. `Detail` compte les fonctions candidates : au
moins deux, sans dépasser le nombre de symboles du résultat. La référence ne
peut pas être le premier enfant cible d'un appel (nœud 28) ; un argument de
callback peut en revanche être une prise d'adresse ambiguë. Le message reprend
le nom écrit par l'appelant, y compris sa qualification, pas un nom résolu
inventé depuis les symboles. Les lexèmes `soi` / `this` ne sont pas des noms de
fonctions candidates pour cette traduction.

La règle du langage ne change pas : une prise d'adresse ne sélectionne pas
automatiquement une surcharge, même si le type de callback destinataire est
connu ; les appels directs continuent de sélectionner leur surcharge à partir
des arguments. Le nombre de candidats et le contexte sont des contrôles de
cohérence, pas une nouvelle résolution de surcharges ni une preuve
d'authenticité d'un résultat modifié par l'appelant.

Le **code 24, Detail 0** traduit les membres introuvables depuis un accès membre
unique (nœud 29) à la position du refus. Son nom est un identifiant simple,
vérifié par lexage de la tranche possédée et par son hachage ; une qualification,
un mot-clé ou des drapeaux autres que l'accès par pointeur (32768) ne conviennent
pas à ce contexte. Le message conserve le nom écrit, en UTF-8, et la position
de `.` / `->`, y compris pour une méthode absente ou un accès chaîné. L'échec
du récepteur, d'un membre intermédiaire ou d'une autre famille conserve sa
priorité native : cette traduction ne résout pas de champ ni de méthode.
Plusieurs candidats indépendants ou un contexte altéré restent une erreur hôte,
jamais un choix de membre inventé. Le code 23 dispose des variantes
contextuelles ci-dessous. Ni les règles
d'accès, ni les formats, ni l'ABI ne changent par cette traduction.

Le **code 25** conserve dans `Detail` l'index du symbole inaccessible. La table
native enregistre une entrée par nœud de genre symbolique, dans l'ordre de l'AST.
L'adaptateur retrouve la déclaration avec cette seule indexation, en vérifiant
le nombre complet de symboles, les bornes et le genre de la cible ; les préfixes
de symboles/résolutions restent jetés sur refus. Ce n'est pas une nouvelle
résolution d'accès, de surcharge, de nom ou de type.

Les variantes traduites distinguent :

- un **champ** (nœud 7) de structure, union ou classe : nom écrit dans l'accès
  membre 29, y compris un alias
  de champ ; un champ callable inaccessible reste un champ, pas une méthode ;
- une **méthode** (nœud 12) : cible membre ou référence nommée, premier enfant
  d'un appel 28 ; le message utilise le nom simple de la déclaration, même pour
  un appel qualifié ou via un alias de fonction ;
- un **opérateur membre** (nœud 15) : expression unaire/binaire 25/26 ; message
  invariant « opérateur membre inaccessible », après vérification des signes
  possédés de l'expression et de la déclaration.

Les deux noms sont bornés dans leurs unités respectives, qui peuvent être
distinctes. Le lexage reste limité à leurs tranches possédées ; noms UTF-8,
commentaires et qualifications sont contrôlés par leurs hachages. Un index
natif hors table ou vers un autre genre de déclaration est une erreur de
contrat hôte. Contexte absent, candidats indépendants ou données altérées ne
permettent pas de fabriquer un diagnostic source. Ces contrôles ne prouvent
pas l'authenticité d'un résultat modifié par l'appelant. Les règles de visibilité
et la priorité de sélection des surcharges restent celles de la passe native.
Ni les exports ni la disposition de l'ABI ne changent dans cette traduction
hôte. L'analyseur Gs++ des déclarations accepte aussi les sections de visibilité
des champs de structures/unions, comme le bootstrap ; ce correctif distinct
modifie l'image native, sans autoriser de méthodes dans ces deux genres.

Les **codes 34 et 35** réutilisent `Detail`, index du champ canonique déjà
résolu, pour traduire un champ initialisé plusieurs fois ou hors de l'ordre
de déclaration. Le nœud initialiseur 35 et le champ 7 doivent être reliés à
la même classe 6, par un constructeur 13. Les deux noms sont bornés dans leur
unité et vérifiés par lexage et hachage. Le message nomme la déclaration du
champ, même si l'initialiseur écrit un alias ; il ne résout pas cet alias à
nouveau. Les indices natifs incohérents, notamment un champ d'une autre classe,
sont rejetés comme erreurs hôtes. La priorité reste celle de la passe native,
y compris devant un argument inconnu et entre plusieurs constructeurs invalides.
Aucun code, export, format, règle de langage ni disposition d'ABI ne change.
Le **code 33** traduit aussi le nom écrit dans l'initialiseur 35. Son `Detail`
est un marqueur de contexte, pas un index de symbole : **0** pour un nom absent,
**1** pour un champ/alias hérité qu'un constructeur ne peut pas initialiser
directement. La passe Gs++ détermine ce contexte avec sa recherche de membres ;
l'hôte ne recherche ni champ, ni alias, ni base. Le nom écrit reste conservé,
même pour un alias hérité, avec lexage et hachage limités à sa tranche possédée.
Le constructeur 13 et sa classe 6 doivent être cohérents, le nombre de symboles
complet et le marqueur borné ; le contexte hérité exige une base déclarée.
Ces contrôles portent sur la cohérence, pas sur l'authenticité d'un marqueur
valide modifié par l'appelant ; la passe native de confiance produit sa variante.
L'héritage non public reste refusé avant ces initialiseurs, comme dans le bootstrap.
Les alias bilingues `ChampInitialiseurHeriteInterdit` /
`InheritedInitializerFieldForbidden` partagent le code 33 existant.
Ce contexte enrichit le résultat natif et modifie l'image Gs++, sans nouveau code,
export, format, règle de langage ni changement de disposition d'ABI.

Les **codes 38 et 40** traduisent la valeur par défaut interdite sur un champ
objet classe et le champ constant non initialisé. `Detail` indexe déjà le champ
canonique 7 ; sa classe 6, son origine exacte et son nom possédé doivent être
cohérents. Le drapeau de valeur par défaut 4 doit être présent pour 38, absent
pour 40. Le nombre complet de symboles, les bornes de l'unité, les lexèmes et le
hachage du nom sont vérifiés. Un autre champ, même dans la même classe, ne peut
pas remplacer la cible sans correspondre à l'origine du refus natif. Aucun
type opaque, alias ou plan de constructeur n'est résolu à nouveau dans l'hôte ;
ces contrôles de cohérence ne prouvent pas l'authenticité d'un résultat modifié
par l'appelant. Cette traduction reste hôte : image native, codes, exports,
formats, règles du langage et ABI inchangés.

Le **code 36** traduit le nombre d'expressions incorrect d'un initialiseur de
champ explicite, et le **code 41 explicite** son argument de tableau non agrégé.
Le message nomme le champ canonique, même lorsqu'un alias est écrit. Le nœud
initialiseur 35, son constructeur 13 et le champ 7 doivent appartenir à la même
classe 6. Leurs noms possédés, origines et indices sont contrôlés. Les enfants
directs de l'initialiseur sont des expressions 22..32 : leur nombre est différent
de un pour 36 ; pour 41, il est exactement un et l'expression n'est pas un agrégat 32.
Ces vérifications structurelles ne refont aucune résolution de champ/alias/type.
Dans la passe Gs++ courante, **`Detail` du code 36 indexe désormais le champ
canonique, et non plus le nombre d'arguments**, toujours disponible dans l'AST.
L'hôte et l'image Gs++ de cette tranche doivent être utilisés ensemble ; ce
contexte modifie l'image native, sans nouveau code, export, format, règle de
langage ni changement de disposition d'ABI.

Le **code 41 par défaut** reste numérique : le bootstrap donne un message de
types d'initialisation incompatibles, pas le message de champ tableau explicite.
Le champ désigné par `Detail`, son drapeau de valeur par défaut et sa première
expression non agrégée permettent de reconnaître le contexte, pas de restituer
les types résolus. Aucun nom de type n'est déduit d'un hachage opaque et aucun
message source approximatif n'est fabriqué. Comme pour les autres familles,
ces contrôles portent sur la cohérence, pas l'authenticité d'un résultat modifié
par l'appelant.

Les codes 23, 46 et 62 disposent aussi de variantes contextuelles possédées :

- **23, Detail 0** : le nœud membre 29, son drapeau 32768 (`->`) et son rôle
  de premier enfant cible d'un appel 28 distinguent quatre messages :
  `'.' exige une structure`, `'->' exige un pointeur vers structure`,
  `'.' exige une classe` et `'->' exige un pointeur vers classe`. Les variantes « classe »
  concernent la cible d'appel, même si aucun membre n'a encore été trouvé.
  Un accès unique, son récepteur expression et ses origines doivent être
  cohérents ; drapeaux inconnus, récepteur absent ou candidats indépendants
  ne permettent pas de traduction. Aucun lexage du nom ni résolution du type
  opaque ne sont nécessaires pour choisir ce message ;
- **46** : le nœud 19 indique une variable locale et le nœud 3 une globale ;
  `Detail` conserve le nombre de dimensions, non nul. Leurs messages diffèrent ;
- **62, Detail 0** : opération logique intrinsèque `&&` ou `||` non scalaire,
  identifiée par son nœud d'expression binaire ;
- **62, Detail 1** : condition non scalaire, sur la première expression enfant
  d'un nœud `si` (20) ou `tantque` (21). Les alias `ConditionNonScalaire` /
  `NonScalarCondition` partagent le code historique 62.

Le marqueur natif de 62 distingue une erreur à l'intérieur d'une opération
logique d'une expression binaire surchargée valide dont le résultat est ensuite
refusé comme condition (par exemple `==` renvoyant une structure). `&&` et `||`
restent intrinsèques : ce changement ne les rend pas surchargeables.
Un appel et ses descendants expressions peuvent partager la position de la
condition ; ils ne sont pas des contextes indépendants. Le marqueur n'est pas
déduit du type opaque. Un marqueur inconnu,
un contexte incomplet ou des nœuds candidats ambigus ne produisent pas de
diagnostic source. `PeutTraduireDiagnostic()` peut donc être faux pour 23/46/62
si les métadonnées possédées ne permettent pas cette traduction. Ces contrôles
ne constituent pas une nouvelle analyse sémantique ni une preuve d'authenticité
d'un résultat modifié par l'appelant.

Dans la passe Gs++, un type nommé d'énumération n'est pas un récepteur de champ
ou de méthode : l'accès via `.` / `->` est refusé au code 23 avant la recherche
de membre, comme dans le bootstrap. Les noms qualifiés d'énumérateurs (`E::Zero`)
restent indépendants de ces accès. Ce contrôle corrige le résultat natif, sans
modifier la valeur des codes, les exports ni l'ABI.

Les autres familles conservent leurs valeurs et positions, mais
`ExigerValide()` lève `diagnostic sémantique non traduit : code=N` : noms
qualifiés, types résolus et variantes contextuelles ne sont pas déduits de
hachages opaques. Cette API ne revendique pas des diagnostics sémantiques
complets et ne change pas le chemin par défaut du pilote.

Dans la passe Gs++ multi-unités, les conditions de `si` / `tantque` sont
résolues puis contrôlées avant leurs branches ou leur corps. Elles doivent
être scalaires ; le refus non scalaire utilise le code contextuel 62, à la
position de l'expression. Une erreur interne à la condition, telle qu'un nom
introuvable, précède ce contrôle ; une condition non scalaire précède les
erreurs du corps. Ce contrôle n'ajoute ni conversion implicite d'agrégat ni
nouvelle règle au bootstrap.

Les limites par défaut sont 64 Mio de texte, un million de nœuds, 1 024 unités,
un million de symboles et un million de résolutions. Les entrées et plages sont
contrôlées avant appel, les tailles de sorties avant allocation. Les limites
doivent être positives et rester dans les bornes hôtes ; elles ne bornent pas
l'arène native. Mesure/publication cohérentes, pointeurs/capacités inchangés,
indices/genres/drapeaux des sorties et gardes sont contrôlés. Allocation,
exception ou contrat incohérent ne publient aucun résultat partiel. Une image
native de confiance reste indispensable ; ces contrôles ne sont pas un sandbox.

API de développement locale : aucun changement du frontend par défaut, du
pilote, de la version produit ni des formats. Backend et traduction complète
des diagnostics sémantiques restent à raccorder.

### Assemblage, normalisation et sémantique avec origines d'inclusions

`AssemblerDeclarationsAvecOrigines` / `AssembleOriginAwareDeclarations` et
`AssemblerDeclarationsNormaliseesAvecOrigines` /
`AssembleNormalizedOriginAwareDeclarations` reçoivent
`RequeteAssemblageAvecOrigines` de 40 octets. Elle référence la requête
d'assemblage historique de 128 octets, un catalogue commun de fichiers par
indices et une table de `TableOriginesUniteDeclarations` de 16 octets par unité.
Chaque table référence les origines de tous les jetons de cette unité développée,
EOF compris. Les sources, modes globaux et nombre d'unités restent ceux de la
requête historique. Les anciens contrats ne changent pas.

L'assemblage brut ne fusionne rien. La normalisation applique les règles
déjà décrites, y compris membres et groupes mixtes. Tous les textes sont
analysés avant la recherche des conflits de normalisation ; les modes locaux
et les plages exactes sont validés. Les trois tampons historiques restent
transactionnels : aucun texte, AST ou table d'unités partiel en cas de refus,
d'allocation impossible ou de capacité insuffisante. Les diagnostics historiques
gardent leur rang d'unité et leurs positions locales synthétiques ; la requête
additive expose en plus l'indice du fichier original et ses coordonnées.

`LocaliserOrigineAssemblagePrepare` / `LocatePreparedAssemblyOrigin` traduit
un début de jeton ou EOF d'une unité, avant rebasage dans le texte assemblé.
Les tables validées et entrées doivent rester inchangées. Il n'alloue rien et
ne relit aucun fichier. Une même inclusion peut apparaître dans plusieurs
unités avec le même indice de fichier : unité de traduction et fichier
d'origine sont deux identités distinctes.

`AnalyserSemantiqueUnitesAvecOrigines` / `AnalyzeOriginAwareUnitSemantics`
reçoit `RequeteSemantiqueUnitesAvecOrigines` de 40 octets, référençant une
requête sémantique et le contexte d'un assemblage avec origines réussi.
La requête sémantique doit utiliser les mêmes tampons de texte et AST, avec
leurs tailles publiées. Le frontend vérifie les tables, les plages lexicales
et l'égalité du texte assemblé avec les unités, BOM retirés, avant d'appeler
la sémantique par unité. Les imports restent isolés entre unités séparées,
pas entre fichiers inclus dans une même unité. Le diagnostic expose le rang
d'unité et le fichier original, sans modifier le contexte d'assemblage.

La sémantique conserve son contrat historique de préfixes de symboles/résolutions,
distinct de l'assemblage transactionnel. Une incohérence détectée pendant la
validation des origines ne publie aucun de ces tampons. Les erreurs de capacité,
d'argument ou d'allocation ne sont pas attribuées à un fichier fautif : les
indices valent leurs nombres d'unités/fichiers et les coordonnées sont nulles.
Les tables de jetons restent celles des unités d'entrée ; aucun nouveau tampon
de jetons concaténés n'est imposé. Lecture et chemins restent côté hôte et ces
entrées ne remplacent pas encore le flux de compilation de fichiers de `gsppc`.
Voir le [bilan du raccordement](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#assemblage-des-inclusions-et-diagnostics-originaux--6-octobre-2026).

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
