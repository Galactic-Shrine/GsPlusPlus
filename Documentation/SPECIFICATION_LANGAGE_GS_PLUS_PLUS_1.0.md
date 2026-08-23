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
- les alias applicatifs peuvent cibler fonctions, globales, types et champs ;
- les cycles, conflits et cibles absentes sont des erreurs ;
- les symboles publics sont contrôlés entre unités par leur signature ABI.

Le contrat détaillé des alias se trouve dans
[`ALIAS_GS_PLUS_PLUS_0.11.md`](ALIAS_GS_PLUS_PLUS_0.11.md).

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

La référence normative détaillée est
[`TYPES_SYSTEME_GS_PLUS_PLUS_0.12.md`](TYPES_SYSTEME_GS_PLUS_PLUS_0.12.md).

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

Les valeurs structurées et leurs règles de passage sont décrites dans
[`VALEURS_STRUCTUREES_GS_PLUS_PLUS_0.15.md`](VALEURS_STRUCTUREES_GS_PLUS_PLUS_0.15.md).

## Fonctions et callbacks

- fonctions globales, méthodes et surcharges ;
- au plus quatre paramètres ordinaires dans le sous-ensemble courant ;
- callbacks typés et signatures récursives ;
- retours scalaires, pointeurs, références prises en charge et valeurs
  structurées ;
- imports et exports explicites ;
- symboles publics compatibles entre unités uniquement si leur signature ABI
  est identique.

Les callbacks sont définis dans
[`POINTEURS_FONCTION_GS_PLUS_PLUS_0.14.md`](POINTEURS_FONCTION_GS_PLUS_PLUS_0.14.md).

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

Les contrats détaillés sont :

- [`MODELE_OBJET_GS_PLUS_PLUS_0.18.md`](MODELE_OBJET_GS_PLUS_PLUS_0.18.md) ;
- [`HERITAGE_GS_PLUS_PLUS_0.19.md`](HERITAGE_GS_PLUS_PLUS_0.19.md) ;
- [`INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md`](INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md) ;
- [`INITIALISEURS_CHAMPS_GS_PLUS_PLUS_0.21.md`](INITIALISEURS_CHAMPS_GS_PLUS_PLUS_0.21.md) ;
- [`CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md`](CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md) ;
- [`TABLEAUX_OBJETS_CLASSES_GS_PLUS_PLUS_0.23.md`](TABLEAUX_OBJETS_CLASSES_GS_PLUS_PLUS_0.23.md) ;
- [`INITIALISATION_DUREE_VIE_GS_PLUS_PLUS_0.25.md`](INITIALISATION_DUREE_VIE_GS_PLUS_PLUS_0.25.md) ;
- [`BIBLIOTHEQUE_HEBERGEE_GS_PLUS_PLUS_0.26.md`](BIBLIOTHEQUE_HEBERGEE_GS_PLUS_PLUS_0.26.md).

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

Le contrat est détaillé dans
[`COMPILATION_SEPAREE_GS_PLUS_PLUS_0.13.md`](COMPILATION_SEPAREE_GS_PLUS_PLUS_0.13.md).

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
