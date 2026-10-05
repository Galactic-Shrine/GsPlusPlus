# Frontend auto-hébergé Gs++ 0.27

**EN COURS — lexeur, AST syntaxique, indexation, sélection typée, contraintes
des expressions couvertes, alias racines, déclarations d'héritage, remplacements
virtuels, doublons de surcharges, signatures non liées, collisions de symboles
de liaison, appels et opérateurs de groupes mixtes, priorités indépendantes,
entre instructions, dans les expressions, appels, abandons de candidats et
arguments agrégés contextuels, conversions constantes lors de leur visite et
initialiseurs locaux contextuels, champs par défaut par constructeur,
constructions locales, plans de durée de vie et priorités des bases/champs couverts,
émission des globales
VALIDÉS dans le périmètre testé — 5 octobre 2026.**

Les sources actuelles annoncent `0.27.0-alpha.10`.
La [matrice alpha.10](Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.10.md)
regroupe les résultats de la publication, avec 619 refus différentiels.
Le développement après alpha.10, décrit plus bas, en vérifie 1 957 ; ces tranches
ne sont pas incluses dans les paquets alpha.10 publiés. Les sections de jalons ci-dessous conservent
leurs versions, empreintes et limites au moment de chaque validation ; les
mentions de `VERSION` resté à alpha.9 décrivent ces étapes historiques. Les
mentions de tranches non commitées ou non publiées relatent également leur
état à la date de validation, pas l’état actuel de la branche de développement.

Gs++ 0.27 a pour objectif de migrer le frontend du compilateur depuis le
bootstrap C++ vers Gs++. Le lexeur constitue la première tranche achevée,
l’alpha.2 ajoute les fonctions libres et leurs paramètres, l’alpha.3 étend le
même AST compact aux déclarations de données, l’alpha.4 couvre les méthodes,
constructeurs, destructeurs et opérateurs de classes, puis l’alpha.5 construit
la hiérarchie des blocs et instructions. L’alpha.6 ajoute l’AST interne des
expressions avec les mêmes priorités et associativités que le bootstrap. Le
jalon alpha.7 ajoute l’indexation des symboles et la première résolution des
noms. L’alpha.8 sélectionne les surcharges libres et membres à partir des types
déjà déterminables dans l’AST compact, contrôle la visibilité, résout les
constructeurs et leurs initialiseurs, puis propage les types à travers les
agrégats et expressions imbriqués, y compris les indexations, adresses,
déréférencements et appels indirects.
Le développement suivant l’alpha.8 rend leurs contraintes explicites et
différentielles sans changer l’ABI publique.
Le frontend 0.27 complet n’est pas encore validé : la résolution exhaustive
des types et les vérifications sémantiques suivantes restent à migrer.

## Lexeur auto-hébergé validé

Les fichiers canoniques sont :

- `AutoHebergement/Lexeur/Lexeur.HGsPP` pour le contrat public ;
- `AutoHebergement/Lexeur/Lexeur.GsPP` pour l’implémentation Gs++ ;
- `Tests/AutoHebergement/AutoHebergement.cpp` pour la comparaison
  différentielle avec le bootstrap C++.

Le lexeur couvre le même périmètre que `Compiler/src/Lexeur.cpp` :

- validation UTF-8 stricte et BOM UTF-8 ;
- séparations ASCII, commentaires de ligne et commentaires de bloc ;
- identifiants ASCII/UTF-8 et classification bilingue des mots-clés ;
- entiers avec séparateurs `_` ;
- chaînes et échappements `\\`, `\"`, `\n`, `\r`, `\t` et `\0` ;
- ponctuation, opérateurs simples et opérateurs doubles ;
- lignes et colonnes identiques au bootstrap ;
- diagnostics explicites pour UTF-8 invalide, commentaire ou chaîne non
  terminé, échappement incomplet ou inconnu et caractère inattendu.

## Contrat mémoire et ABI du lexeur

L’export public est :

```text
GalacticShrine::GsPP::Autohebergement::AnalyserSource(RequeteLexage*) -> ErreurLexage
```

La structure de requête regroupe la source, le stockage fourni par l’appelant,
la capacité et le résultat. Un appel avec une capacité nulle analyse la source
sans écrire hors limites et retourne la capacité exacte requise. Un second
appel remplit le tableau de `JetonLexe`.

Ce modèle respecte la limite actuelle de quatre paramètres du langage, évite
toute allocation cachée et stabilise la signature exportée. Chaque jeton porte
son genre, sa position, sa tranche source, la taille de son texte sémantique et
un hachage FNV-1a 64 bits du texte décodé.

Le composant utilise la validation UTF-8 de `GsHebergee.GsA`. L’image GsE
résultante porte seulement les deux imports d’allocation et de libération
amenés par l’unité UTF-8 de la bibliothèque ; le lexeur lui-même ne déclenche
aucune allocation.

## Preuve différentielle actuelle

Le test charge réellement `Frontend.GsE`, sélectionne son export de lexage,
résout ses imports avec l’ABI `GsAbi:x64-ms-v1` et compare chaque résultat à
`GsPP::Lexeur` :

- six corpus valides, dont source vide, programme accentué, commentaires,
  totalité des opérateurs, chaînes échappées et BOM ;
- six familles d’erreurs lexicales ;
- genre, ligne, colonne, décalage source, texte décodé, taille et hachage de
  chaque jeton ;
- interrogation de capacité, capacité insuffisante et arguments invalides ;
- absence de libération invalide et d’allocation résiduelle.

Résultats de la construction publique autonome, qui n’inclut ni Sanctuaire SE
ni `Noyau.GsE` :

```text
MSVC    : 3/3 CTest réussis, conformité 20/20
GNU/WSL : 4/4 CTest réussis, conformité 20/20
```

La préversion publique `0.27.0-alpha.1` étend la conformité à 20/20 avec le
format XML des projets et solutions. Le test différentiel du lexeur est
également obligatoire dans `gspp_autohebergement_tests` sur les deux chaînes.
Ce résultat valide la tranche livrée, pas le frontend 0.27 complet.

Les images GsE MSVC et GNU sont identiques :

```text
taille  : 42 163 octets
SHA-256 : 25402c05c9d8af94bcf3ededb17f8b81f9f85c1a726e65d3d560e4b3392683a3
```

Le vérificateur confirme une image GsE 1.0 valide, ABI 1, avec trois segments,
huit sections, deux imports et soixante-sept exports.

Les GsObj intermédiaires ont la même taille mais pas encore le même hash entre
Windows et WSL : leur table de symboles conserve les chemins absolus
`D:/GSLSE/...` ou `/mnt/d/GSLSE/...`. Le contenu fonctionnel lié aboutit malgré
tout au même GsE. La canonicalisation des chemins de provenance reste un point
explicite du durcissement et de la reproductibilité 0.29.

## AST auto-hébergé — tranches alpha.2 à alpha.6

Les fichiers canoniques de la deuxième tranche sont :

- `AutoHebergement/AnalyseurDeclarations/AnalyseurDeclarations.HGsPP` pour le
  contrat ABI public ;
- `AutoHebergement/AnalyseurDeclarations/AnalyseurDeclarations.GsPP` pour
  l’implémentation Gs++ ;
- `Tests/AutoHebergement/AutoHebergement.cpp` pour la comparaison avec les
  objets `Programme`, `Fonction`, `Parametre`, `VariableGlobale`, `Structure`,
  `ChampStructure`, `Enumeration`, `Enumerateur`, les deux catégories d’alias
  les membres exécutables de classes, les instructions et les expressions du
  bootstrap C++.

L’export public est :

```text
GalacticShrine::GsPP::Autohebergement::AnalyserDeclarationsSource(
    RequeteAnalyseDeclarations*) -> ErreurAnalyseDeclarations
```

Comme pour le lexeur, un premier appel sans stockage retourne la capacité
exacte, un appel trop petit remplit seulement le préfixe disponible et un
appel correctement dimensionné retourne l’AST complet. Chaque
`NoeudDeclaration` occupe 64 octets et porte :

- le genre programme, fonction, paramètre, variable globale, structure, union,
  classe, champ, énumération, énumérateur, alias, alias de champ, méthode,
  constructeur, destructeur, surcharge d’opérateur, bloc, retour, instruction
  d’expression, variable locale, conditionnelle, boucle `tantque`, littéral,
  référence de variable, expression unaire ou binaire, affectation, appel,
  accès membre, indexation, conversion, agrégat, initialiseur de constructeur
  délégué, initialiseur de base ou initialiseur de champ ;
- la ligne et la colonne du début de la déclaration ;
- le parent et les drapeaux de visibilité, caractère externe, définition ou
  initialiseur présent, héritage, virtualité, remplacement et forme de liste
  d’initialisation d’un constructeur ;
- la tranche source et le hachage FNV-1a du nom ;
- l’empreinte de l’espace de noms ;
- une empreinte de type normalisée entre les mots-clés français et anglais.

Les expressions sont émises en préordre sous la déclaration ou l’instruction
qui les porte. Les enfants d’une expression unaire, binaire, d’une affectation,
d’un appel, d’un accès membre, d’une indexation, d’une conversion ou d’un
agrégat sont rattachés au nœud compact correspondant. Les valeurs entières sont
conservées dans `HachageType`, les chaînes sous forme de hachage de leur texte
décodé et les conversions sous forme d’empreinte normalisée du type cible. Des
drapeaux distinguent les littéraux booléens, les accès via pointeur et les
références canoniques à la base `parent` / `super`.

L’empreinte de type couvre les types natifs, les types qualifiés, `constante`/
`const`, `volatile`, les pointeurs, les références et les dimensions de
tableaux fixes. Les espaces simples, imbriqués et écrits sous forme qualifiée
`A::B` produisent la même identité canonique.

Les jetons et les nœuds de travail sont alloués dans `AreneMemoire`, fournie
par la bibliothèque hébergée 0.26. L’image GsE conserve seulement les deux
imports explicites d’allocation et de libération. Le test vérifie que chaque
appel détruit l’arène, qu’aucune adresse invalide n’est libérée et qu’aucune
allocation active ne subsiste.

### Preuve différentielle de la tranche

Le test charge réellement `Frontend.GsE`, sélectionne son export syntaxique et
compare chaque nœud au programme produit par `GsPP::AnalyseurSyntaxique` :

- paires de corpus français/anglais structurellement équivalents pour les
  fonctions, les déclarations de données, les membres de classes et les
  instructions ;
- fonctions publiques, externes et avec corps ;
- paramètres, espaces qualifiés, tableaux multidimensionnels et types
  qualifiés ;
- globales publiques, externes, initialisées, en tableau et avec agrégat
  d’initialisation ;
- structures, unions, classes de données, champs et sections de visibilité ;
- héritage simple, initialiseurs de champs de classes et tableaux de champs ;
- énumérations avec valeurs implicites, explicites et virgule terminale ;
- alias de déclarations et alias de champs ;
- méthodes, constructeurs, destructeurs et surcharges d’opérateurs ;
- paramètres explicites des membres sans matérialiser le paramètre synthétique
  `soi` comme s’il provenait de la source ;
- membres virtuels ou de remplacement et trois formes de listes
  d’initialisation de constructeur ;
- blocs de fonction ou imbriqués, retours avec ou sans expression, instructions
  d’expression et variables locales initialisées ou construites explicitement ;
- conditionnelles avec branches simples ou imbriquées et boucles `tantque` ;
- relations parent-enfant en préordre entre fonctions, blocs, contrôles et
  branches ;
- expressions des globales, énumérateurs, champs, listes d’initialisation de
  constructeurs, retours, variables locales, conditions et boucles ;
- onze genres d’expressions, six opérateurs unaires, dix-huit opérateurs
  binaires, affectation associative à droite, appels, membres directs ou via
  pointeur, indexations, conversions, agrégats et noms qualifiés ;
- relations parent-enfant en préordre entre chaque porteur syntaxique et toutes
  ses sous-expressions ;
- interrogation de capacité, capacité partielle et requêtes invalides ;
- trente-trois diagnostics syntaxiques avec ligne et colonne identiques au
  bootstrap ;
- propagation distincte des erreurs lexicales ;
- relations parent-enfant vérifiées entre classes, membres exécutables et
  paramètres explicites.

Les constructions MSVC et GNU produisent un `Frontend.GsE` identique bit à
bit. La dernière matrice publique conservée est celle de
[`0.27.0-alpha.9`](Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.9.md).

Cette tranche construit l’AST des corps, de leurs instructions et de leurs
expressions. Les classes sont couvertes pour leurs données, leur héritage, leurs
membres exécutables et leurs corps. Ce périmètre reste volontairement annoncé
comme `PARTIEL` : il décrit la tranche syntaxique et ne suffit pas, à lui seul,
à former un frontend complet.

## Première passe sémantique — alpha.7

Les fichiers canoniques de cette tranche sont :

- `AutoHebergement/AnalyseurSemantique/AnalyseurSemantique.HGsPP` pour le
  contrat ABI public ;
- `AutoHebergement/AnalyseurSemantique/AnalyseurSemantique.GsPP` pour
  l’implémentation Gs++ ;
- `Tests/AutoHebergement/AutoHebergement.cpp` pour l’exécution réelle de
  l’image et la comparaison différentielle avec le bootstrap C++.

L’export public est :

```text
GalacticShrine::GsPP::Autohebergement::AnalyserSemantique(
    RequeteAnalyseSemantique*) -> ErreurAnalyseSemantique
```

La passe consomme le tableau de `NoeudDeclaration` sans le modifier. Elle
indexe les types, fonctions, variables globales, alias, champs, alias de
champs, énumérateurs, paramètres et variables locales, puis produit une entrée
de résolution pour chaque référence de variable couverte. Les noms qualifiés,
les portées imbriquées, le récepteur de classe `soi` / `this`, le récepteur de
base `parent` / `super` et les groupes de surcharges utilisés comme cibles
d’appel sont distingués explicitement.

Le stockage de sortie appartient à l’appelant. Une interrogation sans tampon
retourne les deux capacités exactes ; les sorties partielles restent bornées et
signalent séparément une capacité insuffisante de symboles ou de résolutions.
Les tailles ABI sont :

| Structure | Taille |
| --- | ---: |
| `SymboleSemantique` | 48 octets |
| `ResolutionSemantique` | 32 octets |
| `ResultatAnalyseSemantique` | 56 octets |
| `RequeteAnalyseSemantique` | 120 octets |

L’image conserve uniquement les imports explicites
`GalacticShrine::GsPP::Hote::AllouerMemoire` et
`GalacticShrine::GsPP::Hote::LibererMemoire`, utilisés par l’arène de travail.
Le test vérifie l’équilibre exact des allocations et libérations.

### Preuve différentielle de la sémantique

Les corpus français et anglais valides vérifient :

- les mêmes nombres de symboles et de résolutions dans les deux syntaxes ;
- une résolution pour chaque référence de variable du corpus ;
- la cohérence entre nœud, symbole cible, genre et bornes des index ;
- les paramètres, variables locales, portée de bloc, globale qualifiée,
  récepteurs `soi` / `this` et `parent` / `super`, et groupe de surcharges
  appelé ;
- l’interrogation de capacité, les deux sorties partielles et les requêtes ou
  AST invalides.

Quinze corpus négatifs comparent le code, la ligne et la colonne au bootstrap
C++ : doubles déclarations ou conflits de types, globales et alias ; doublons
de champs, alias de champs, énumérateurs, paramètres ou variables locales ;
symbole inconnu ; adresse ambiguë d’une fonction surchargée ; programme sans
fonction. Ces tests valident la tranche annoncée, pas l’analyse sémantique
complète du langage.

La matrice publique conservée, ses empreintes reproductibles et les contrôles
des paquets extraits sont consignés dans la validation de
[`0.27.0-alpha.9`](Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.9.md).

## Première sélection typée des surcharges — alpha.8

La passe auto-hébergée sélectionne désormais une fonction libre ou une méthode
précise lorsqu’une cible d’appel désigne plusieurs surcharges. Le choix tient
compte :

- du nombre de paramètres ;
- des empreintes de type des paramètres et des arguments déjà résolus ;
- des paramètres, variables locales et globales utilisés comme arguments ;
- des conversions explicites présentes dans l’AST ;
- du type canonique des littéraux booléens, chaînes et entiers ;
- de l’adaptation d’un littéral entier lorsque sa valeur est représentable
  dans le type du paramètre ;
- des paramètres par référence et de l’exigence d’une valeur gauche ;
- des agrégats temporaires, refusés pour une référence et classés derrière une
  correspondance de type exacte ;
- des conversions d’héritage `Dérivée → Base&` et
  `Dérivée* → Base*` ;
- du score minimal de conversion, avec détection des ex æquo.

Les accès par `.` et `->` déterminent le type du récepteur, recherchent les
champs, alias de champs et groupes de méthodes, puis parcourent la chaîne de
base lorsque le membre n’est pas déclaré par le type courant. Les drapeaux
`Membre`, `MembreHerite` et `Methode` décrivent la résolution produite.

Une résolution de surcharge conserve le drapeau `GroupeSurcharges`, mais son
`IndexSymbole` désigne la fonction ou méthode effectivement choisie et son
`HachageType` son type de retour. Les corpus bilingues sélectionnent
différentiellement les variantes `entier32` et `entier64` de `Choisir` et
`Transformer`, ainsi que les variantes compatibles avec une référence, un
agrégat et une conversion d’héritage.

Les diagnostics `AucuneSurchargeCompatible`, `AppelSurchargeAmbigu`,
`RecepteurMembreInvalide` et `MembreIntrouvable` sont comparés au bootstrap
C++. Les variantes libres et membres portent le total courant à vingt et un
corpus sémantiques négatifs.

Cette tranche est validée localement par la reconstruction réelle de
`Frontend.GsE`, la sélection de son export sémantique et les suites complètes
MSVC et GNU. Les étapes objet qui prolongent cette sélection sont décrites dans
la section suivante.

## Visibilité, constructeurs locaux et opérateurs membres — alpha.8

La passe auto-hébergée applique désormais les sections `publique`, `protégée`
et `privée` aux champs et méthodes sélectionnés. Un membre privé est accessible
depuis sa classe propriétaire ; un membre protégé l’est également depuis une
classe dérivée. La règle est appliquée après la sélection de surcharge, comme
dans le bootstrap C++.

Les variables locales de type classe sont reliées au constructeur exact :

- une déclaration sans parenthèses sélectionne le constructeur sans argument
  lorsqu’un constructeur est déclaré ;
- une déclaration avec parenthèses sélectionne la surcharge selon les mêmes
  règles typées que les fonctions et méthodes ;
- la résolution distingue `Constructeur`, `ConstructionExplicite` et
  `GroupeSurcharges` ;
- une classe sans constructeur déclaré reste constructible implicitement sans
  argument, mais la forme explicite ou tout argument est refusé ;
- la visibilité du constructeur sélectionné est contrôlée dans le contexte de
  la fonction englobante.

Les expressions unaires et binaires dont l’opérande gauche est une classe
recherchent maintenant un opérateur membre direct ou hérité. Le récepteur
implicite `soi` n’est pas compté parmi les paramètres explicites de l’AST. Le
score distingue les variantes `entier32` et `entier64`, les ex æquo restent
ambigus et les opérateurs `&` ou `*` conservent leur sens intrinsèque. Une
résolution d’opérateur porte les drapeaux `Membre`, `Methode` et `Operateur`,
ainsi que `MembreHerite` ou `GroupeSurcharges` lorsque nécessaire.

Les nouveaux diagnostics sont :

| Code | Diagnostic | Signification |
| ---: | --- | --- |
| 25 | `MembreInaccessible` | champ, méthode ou opérateur non accessible |
| 26 | `ConstructeurInaccessible` | surcharge trouvée mais non accessible |
| 27 | `ConstructeurNonDeclare` | construction explicite sans constructeur déclaré |
| 28 | `OperateurIntrouvable` | aucun opérateur membre correspondant dans la hiérarchie |
| 29 | `InitialiseurClasseInterdit` | utilisation interdite de `Classe objet = expression` |

`AucuneSurchargeCompatible` et `AppelSurchargeAmbigu` restent communs aux
fonctions, méthodes, constructeurs et opérateurs. Douze nouveaux corpus
négatifs contrôlent les accès privés ou protégés, l’initialisation de classe
avec `=`, les constructeurs absents, inaccessibles, incompatibles ou ambigus,
et les opérateurs absents, inaccessibles, incompatibles ou ambigus. Le total
courant est de trente-trois corpus sémantiques négatifs, tous comparés au
bootstrap C++ pour le code, la ligne et la colonne.

Le corpus bilingue valide sélectionne deux constructeurs et trois opérateurs
distincts, puis vérifie les accès privé dans la classe propriétaire et protégé
depuis une classe dérivée. Les tailles ABI restent inchangées : 48 octets pour
un symbole, 32 pour une résolution, 56 pour le résultat et 120 pour la requête.

## Initialiseurs explicites de constructeurs — alpha.8

L’AST compact ne confond plus les arguments de la liste d’initialisation avec
les enfants directs du constructeur. Trois genres bilingues, numérotés sans
modifier les genres 0 à 32, matérialisent désormais chaque entrée :

| Genre | Français | Anglais | Contenu |
| ---: | --- | --- | --- |
| 33 | `InitialiseurConstructeurDelegue` | `DelegatingConstructorInitializer` | arguments de `soi(...)` / `this(...)` |
| 34 | `InitialiseurConstructeurBase` | `BaseConstructorInitializer` | arguments de `parent(...)` / `super(...)` |
| 35 | `InitialiseurChampConstructeur` | `ConstructorFieldInitializer` | nom source du champ et arguments |

Chaque nœud est enfant direct du constructeur ; ses expressions sont ses
propres enfants. Les catégories délégation et base sont anonymes et restent
normalisées entre les syntaxes française et anglaise. L’initialiseur de champ
conserve sa tranche source et son hachage. `NoeudDeclaration` reste strictement
à 64 octets.

La passe sémantique sélectionne la surcharge de constructeur de la classe
courante pour une délégation, puis celle de la base directe pour un
initialiseur explicite. Elle contrôle l’accès public ou protégé au constructeur
de base, refuse une délégation directe et parcourt la chaîne complète pour
détecter les cycles. Une résolution porte alors `Constructeur`,
`ConstructionExplicite` et respectivement `DelegationConstructeur` ou
`InitialisationBase`.

Un initialiseur de champ est limité aux champs déclarés directement dans la
classe du constructeur. Les alias de champs sont normalisés vers le stockage
canonique ; les champs inconnus ou hérités, les doublons et un ordre différent
de l’ordre de déclaration sont refusés. La résolution cible le symbole du champ
et porte `Membre | InitialisationChamp`.

Les diagnostics supplémentaires sont :

| Code | Diagnostic | Signification |
| ---: | --- | --- |
| 30 | `InitialiseurBaseSansClasseBase` | `parent(...)` / `super(...)` utilisé dans une classe racine |
| 31 | `DelegationConstructeurDirecte` | un constructeur se sélectionne lui-même comme cible directe |
| 32 | `CycleDelegationConstructeur` | la chaîne de délégation revient sur un constructeur déjà visité |
| 33 | `ChampInitialiseurIntrouvable` | aucun champ direct ou alias canonique ne correspond |
| 34 | `ChampInitialisePlusieursFois` | deux entrées ciblent le même champ canonique |
| 35 | `OrdreInitialisationChampInvalide` | les champs ne suivent pas leur ordre de déclaration |

Huit nouveaux corpus négatifs portent le total à quarante et un. Le corpus
positif bilingue vérifie séparément une délégation, une construction de base et
deux initialisations de champs, en plus des constructions locales déjà
couvertes.

## Champs objets, valeurs scalaires et base implicite — alpha.8

La résolution des initialiseurs de champs couvre maintenant leur première
sémantique de valeur :

- un champ scalaire exige exactement une expression ;
- le type calculable de cette expression doit correspondre au type du champ,
  après retrait des qualificatifs de valeur pour les types non-adresse ;
- un littéral entier est accepté seulement s’il est représentable dans le type
  de destination ;
- une conversion d’héritage de pointeur reste admise selon les mêmes règles que
  pour les paramètres ;
- un champ objet de classe direct sélectionne sa surcharge de constructeur et
  en contrôle la visibilité ;
- le nœud d’initialisation conserve une résolution vers le champ et reçoit une
  seconde résolution vers le constructeur lorsqu’il existe.

Lorsqu’un constructeur de classe dérivée ne porte ni initialiseur explicite de
base ni délégation, la passe sélectionne désormais le constructeur sans
argument de la base directe. Cette résolution porte `Constructeur` et
`InitialisationBase`, mais pas `ConstructionExplicite`. Une base sans
constructeur déclaré conserve sa construction implicite triviale ; une base
qui ne possède aucune surcharge sans argument ou dont cette surcharge est
inaccessible est refusée au même emplacement que par le bootstrap.

Deux diagnostics complètent le contrat sans modifier les structures ABI :

| Code | Diagnostic | Signification |
| ---: | --- | --- |
| 36 | `AriteInitialiseurChampInvalide` | un champ scalaire ne reçoit pas exactement une expression |
| 37 | `TypeInitialiseurChampIncompatible` | l’expression connue ne peut pas initialiser le type scalaire du champ |

Un corpus positif bilingue vérifie quatre champs directs, dont un objet classe,
une valeur constante et des littéraux entiers positifs et négatifs adaptés,
ainsi que la construction implicite de la base. Huit corpus négatifs couvrent
l’arité, le type, le dépassement d’un entier étroit, les constructeurs de
champs absents,
incompatibles ou privés et les constructeurs de base implicites incompatibles
ou privés. Le total courant atteint quarante-neuf corpus sémantiques négatifs
comparés au bootstrap pour le code, la ligne et la colonne.

## Tableaux et valeurs de champs par défaut — alpha.8

La passe sémantique retrouve maintenant le type final d’un tableau sans
modifier le nœud AST public de 64 octets. Elle relit les dimensions qui suivent
le nom dans la source, reproduit leur hachage canonique puis compare le hachage
complet du type aux types natifs et nommés indexés. Les séparateurs, les
commentaires et les séparateurs `_` des tailles entières sont acceptés comme
par l’analyseur de déclarations.

Cette reconstruction permet de couvrir les contrats suivants :

- un tableau local d’objets classes sélectionne le constructeur sans argument
  ou la surcharge correspondant à ses arguments uniformes ;
- un tableau de champ objet applique la même sélection lorsqu’il apparaît dans
  la liste du constructeur ;
- un champ objet direct ou tableau omis de la liste reçoit une construction
  implicite, sauf dans un constructeur délégué ;
- une valeur de champ par défaut est appliquée à chaque constructeur qui ne la
  remplace pas explicitement ;
- le type connu d’une valeur scalaire par défaut est validé avec les mêmes
  adaptations de littéraux que les initialiseurs explicites ;
- un tableau scalaire explicite ou par défaut exige une expression agrégée ;
- un champ constant non-adresse doit être initialisé explicitement ou posséder
  une valeur par défaut ;
- un champ objet classe ne peut pas utiliser la forme `= expression` et une
  classe possédant une valeur par défaut doit déclarer un constructeur.

Les résolutions distinguent désormais l’initialisation implicite, les tableaux
et les valeurs par défaut sans agrandir `ResolutionSemantique`. Un corpus
positif bilingue exerce les champs objets directs, les tableaux d’objets avec
et sans argument, les tableaux scalaires agrégés, le remplacement explicite
d’une valeur et les champs constants. Quatorze corpus négatifs supplémentaires
portent le total à soixante-trois et comparent toujours le code, la ligne et la
colonne au bootstrap.

Quatre diagnostics complètent le contrat :

| Code | Diagnostic | Signification |
| ---: | --- | --- |
| 38 | `ValeurChampParDefautObjetClasseInterdite` | un objet classe utilise son constructeur et non `= expression` |
| 39 | `ClasseValeurChampParDefautSansConstructeur` | une classe avec valeur de champ par défaut ne déclare aucun constructeur |
| 40 | `ChampConstantNonInitialise` | un champ constant non-adresse reste sans valeur |
| 41 | `InitialiseurChampTableauNonAgrege` | un tableau scalaire reçoit une expression non agrégée |

## Typage récursif des agrégats — alpha.8

Le frontend parcourt désormais récursivement chaque nœud `Agregat` avec une
description interne de son type de destination. Pour un tableau, la profondeur
courante sélectionne la dimension correspondante relue depuis la déclaration ;
chaque niveau contrôle donc sa propre capacité avant de descendre vers le type
final de l’élément. Cette description reste interne et ne modifie ni le nœud
AST public de 64 octets, ni les structures ABI sémantiques.

Le contrat rejoint maintenant le bootstrap pour les types déjà déterminables
dans l’AST compact :

- les agrégats vides ou partiels sont acceptés et les éléments absents restent
  destinés à la mise à zéro ;
- chaque dimension d’un tableau multidimensionnel exige son niveau d’agrégat
  et refuse un nombre d’éléments supérieur à sa capacité ;
- une structure consomme ses champs directs dans leur ordre lexical ;
- une union accepte au plus une valeur, associée à son premier champ ;
- un scalaire agrégé accepte au plus une valeur, y compris au travers
  d’accolades imbriquées ;
- chaque feuille dont le type est connu réutilise les adaptations de littéraux,
  qualifications de valeur et conversions d’héritage déjà prises en charge ;
- le même parcours s’applique aux globales, variables locales, valeurs de
  champs par défaut et initialiseurs explicites de champs.

Le corpus positif bilingue couvre vingt nœuds agrégats : tableaux globaux et
locaux multidimensionnels, structure contenant un tableau, union, scalaire
imbriqué, champ multidimensionnel par défaut et remplacement du même champ dans
un constructeur. Quatorze corpus négatifs supplémentaires portent le total à
soixante-dix-sept ; ils vérifient le code auto-hébergé attendu ainsi que la
ligne et la colonne fournies par le bootstrap.

Cinq diagnostics complètent le contrat :

| Code | Diagnostic | Signification |
| ---: | --- | --- |
| 42 | `TropElementsInitialiseurTableau` | un niveau de tableau dépasse la capacité de sa dimension |
| 43 | `TropElementsInitialiseurStructure` | une structure ou une union reçoit trop de valeurs |
| 44 | `TropElementsInitialiseurScalaire` | un scalaire agrégé reçoit plus d’une valeur |
| 45 | `TypeElementInitialiseurAgregeIncompatible` | une feuille connue ne peut pas initialiser sa destination ou une dimension imbriquée manque |
| 46 | `InitialiseurTableauNonAgrege` | un tableau global ou local reçoit une expression non agrégée |

## Propagation récursive des appels et opérateurs — alpha.8

La passe conserve maintenant, dans son arène privée, la cible sélectionnée pour
chaque référence, membre ou opérateur résolu. Ce cache n’est ni exposé ni relu
depuis le tampon public de résolutions : l’interrogation de capacité et
l’analyse avec stockage fourni par l’appelant suivent donc exactement le même
chemin, sans modification des tailles ABI.

Les références, membres et opérateurs sont résolus au cours d’un parcours
descendant unique de l’AST compact. Comme l’AST est aplati en préordre, ce sens
de parcours traite les enfants avant leur parent. Le type d’un appel interne ou
d’un opérateur surchargé est ainsi disponible lorsque l’expression englobante
classe ses propres surcharges. La propagation couvre maintenant :

- le type de retour sans marqueur de référence d’une fonction libre appelée ;
- le type de retour d’une méthode directe ou héritée sélectionnée ;
- le type de retour d’un opérateur membre unaire ou binaire sélectionné ;
- le type `booléen` des comparaisons et opérateurs logiques intrinsèques ;
- la réutilisation de ces types dans les appels englobants et dans les feuilles
  d’initialiseurs agrégés.

Le corpus positif bilingue combine une méthode imbriquée dans un appel
surchargé, un opérateur membre imbriqué dans une autre sélection de surcharge,
deux opérateurs en feuilles d’un tableau et une comparaison booléenne. Trois
refus différentiels supplémentaires vérifient les retours incompatibles d’un
appel, d’un opérateur membre et d’une comparaison dans un agrégat. Le total
courant atteint quatre-vingts corpus sémantiques négatifs comparés au bootstrap
pour le code, la ligne et la colonne.

## Indexations, adresses et appels indirects — alpha.8

L’empreinte compacte des types couvre maintenant
`pointeur_fonction<retour(paramètres)>` et sa forme anglaise
`function_pointer<return(parameters)>`. La signature interne hache le type de
retour, les paramètres dans leur ordre lexical et leur nombre. Cette
représentation est récursive : un callback peut retourner ou recevoir un autre
callback, y compris avec la fermeture lexicale `>>`.

Le passage sémantique relit les jetons de type dans son arène privée. Il peut
ainsi retrouver le retour d’un callback sans ajouter de champ à
`NoeudDeclaration`, `SymboleSemantique` ou `ResolutionSemantique`. Les tailles
ABI publiques restent donc inchangées. Cette description interne permet les
transformations suivantes :

- une indexation de tableau retire exactement sa première dimension et
  conserve les dimensions restantes ;
- une indexation de pointeur retire un niveau de pointeur ;
- `&` ajoute un niveau de pointeur à une valeur adressable, tandis que
  l’adresse d’une fonction produit sa signature de callback complète ;
- `*` retire un niveau de pointeur et conserve un pointeur de fonction direct,
  conformément au bootstrap ;
- un appel direct conserve le type de retour de la fonction sélectionnée ;
- un appel indirect extrait le retour de la signature du callback, y compris
  après une indexation ou lorsqu’un premier callback retourne le callback
  appelé.

Le corpus positif bilingue classe cinq appels surchargés à partir d’une double
indexation, de `*(&valeur)`, d’un callback local, d’un tableau de callbacks et
d’un callback imbriqué. Quatre refus supplémentaires comparent au bootstrap une
indexation, une adresse, un déréférencement et un retour d’appel indirect
incompatibles dans un agrégat. Le total atteint quatre-vingt-quatre corpus
sémantiques négatifs positionnés.

La matrice de publication du 29 août 2026 passe 4/4 sous Visual Studio 2026 et 5/5 sous
GNU/Linux, la conformité reste à 20/20 et les quatre scénarios de benchmark
smoke réussissent sur chaque chaîne. Le frontend auto-hébergé est désormais
livré dans une seule image, identique bit à bit entre les chaînes :

| Image | Taille | SHA-256 |
| --- | ---: | --- |
| `Frontend.GsE` | 231 809 | `e798a3fae8903a1788d66c0c2ef4187a64bd136d9a99131d170044a8a65c301a` |

Les fichiers `ClassificateurMotsCles.GsObj`, `Lexeur.GsObj`,
`AnalyseurDeclarations.GsObj` et `AnalyseurSemantique.GsObj` restent des
modules intermédiaires de construction. Ils ne sont ni installés ni présentés
comme des applications distinctes. `Frontend.GsE` expose leurs quatre points
d’entrée publics afin que les tests différentiels puissent encore valider
chaque étape séparément.

## Contraintes des expressions typées — développement après alpha.8

La passe contrôle maintenant les préconditions complètes des quatre familles
d’expressions déjà typées. `&` exige une valeur gauche, sauf pour l’adresse
directe d’une fonction, et refuse encore les pointeurs vers tableaux complets
que le bootstrap ne prend pas en charge. `*` exige un pointeur mais conserve la
sémantique particulière du pointeur de fonction direct.

Une indexation exige un tableau ou un pointeur véritable, puis un indice entier.
Elle refuse donc aussi bien un scalaire ou un pointeur de fonction pur que
`vide*`. L’appel indirect exige une signature de callback appelable au niveau de
pointeur courant, le nombre exact d’arguments et un initialiseur compatible pour
chaque paramètre. Les liaisons par référence exigent une valeur gauche et
réutilisent les conversions d’héritage déjà validées.

La reconstruction reste privée : la position lexicale, le retour, l’arité et
les paramètres sont relus depuis les jetons du type. Aucune taille de
`NoeudDeclaration`, `SymboleSemantique`, `ResolutionSemantique`,
`ResultatAnalyseSemantique` ou `RequeteAnalyseSemantique` ne change. Les neuf
nouveaux codes sont ajoutés après les codes existants :

| Code | Diagnostic | Condition refusée |
| ---: | --- | --- |
| 47 | `AdresseValeurNonAdressable` | l’opérande de `&` n’est pas une valeur gauche |
| 48 | `AdresseTableauCompletInterdite` | l’opérande de `&` est encore un tableau complet |
| 49 | `DereferencementSansPointeur` | l’opérande de `*` n’est pas un pointeur |
| 50 | `CibleIndexationInvalide` | la cible n’est ni un tableau ni un pointeur indexable |
| 51 | `IndiceNonEntier` | l’indice n’est pas un entier |
| 52 | `IndexationPointeurVide` | l’élément calculé serait de type `vide` |
| 53 | `CibleAppelIndirectInvalide` | la cible n’est pas un callback directement appelable |
| 54 | `AriteAppelIndirectInvalide` | le nombre d’arguments diffère de la signature |
| 55 | `TypeArgumentAppelIndirectIncompatible` | un argument ne peut pas initialiser son paramètre |

Le corpus positif bilingue inclut désormais un paramètre de callback par
référence, `(&Fonction)(...)` et l’appel d’un callback obtenu par
déréférencement. Vingt-quatre refus français et anglais couvrent les neuf codes,
les références temporaires, les pointeurs vers callbacks non déréférencés et
les callbacks purs indexés. Le total atteint cent huit corpus sémantiques
négatifs dont le code, la ligne et la colonne sont comparés au bootstrap.

La matrice locale de développement passe 4/4 sous Visual Studio 2026 et 5/5
sous GNU/Linux. Les deux chaînes reconstruisent une image `Frontend.GsE` GsE
1.0 de 241 921 octets, identique bit à bit, dont le SHA-256 est
`c4d3e331f5d86e7266151a8755084741bcfb79bc1f6a711ec46a0220d4f0a567`.

Cette tranche reste volontairement bornée aux formes décrites. Les autres
combinaisons d’opérateurs intrinsèques et les opérateurs libres restent à
migrer.

## Plans de construction, destruction et durée de vie — développement après alpha.8

La passe auto-hébergée reconstruit maintenant, dans son arène privée, la
disposition des structures, unions et classes nécessaire aux plans de durée de
vie. Le calcul conserve les règles du bootstrap : base à l’adresse zéro,
alignement naturel des champs, superposition des champs d’union, pointeur de
table virtuelle de huit octets et réutilisation de son emplacement dans une
hiérarchie déjà polymorphe. Les cycles de types par valeur sont détectés pendant
ce calcul.

Les structures ABI publiques restent inchangées. Une
`ResolutionSemantique` marquée `EtapeDureeVie` réutilise son champ
`HachageType` pour transporter le décalage relatif en octets. Sa cible désigne
un constructeur, un destructeur ou le type de la table virtuelle. L’ordre des
résolutions est l’ordre exécutable du plan. Les nouveaux drapeaux sont additifs :

| Valeur | Drapeau français | Rôle |
| ---: | --- | --- |
| 32768 | `EtapeDureeVie` | distingue un plan d’une résolution de type ordinaire |
| 65536 | `ConstructionPlanifiee` | étape de construction |
| 131072 | `DestructionPlanifiee` | étape de destruction |
| 262144 | `InitialisationTableVirtuelle` | installation ou remplacement de table virtuelle |
| 524288 | `SousObjetBase` | étape appartenant à une base |
| 1048576 | `SousObjetChamp` | étape appartenant à un champ |
| 2097152 | `ElementTableau` | étape répétée pour un élément de tableau |

La construction implicite traite la base avant la table virtuelle et les
champs dans leur ordre de déclaration. Un tableau avance de l’indice zéro au
dernier indice. La destruction appelle d’abord le destructeur propre, puis les
champs dans l’ordre inverse et enfin la base ; un tableau parcourt ses éléments
en sens inverse. Les constructeurs délégués, les bases explicites, les champs
explicitement ou implicitement construits et les tableaux multidimensionnels
produisent tous leurs étapes avec leur adresse relative exacte.

Les diagnostics `56` (`DestructeurInaccessible`), `57`
(`CycleTypeParValeur`) et `58` (`TailleObjetInvalide`) prolongent la table sans
renuméroter les codes existants. Le corpus différentiel bilingue vérifie un
objet polymorphe dérivé, un constructeur avec base sans constructeur propre,
des champs objets, un tableau de champs, un tableau local et la destruction
inverse. Les refus du destructeur privé, du cycle par valeur et du champ `vide`
portent le total à cent onze corpus sémantiques négatifs comparés au bootstrap.

La matrice de développement passe 4/4 sous Visual Studio 2026 et 5/5 sous
GNU/Linux 11.4. Les deux chaînes produisent une image `Frontend.GsE` GsE 1.0
de 269 121 octets, acceptée par `gseverifier`, avec le même SHA-256 :
`069f7de8430dbe075d2be1a9fcb5885775d692f7bd23b8b04636847f617b4e26`.
Cette preuve valide la tranche de durée de vie annoncée ; elle ne constitue pas
encore un frontend auto-hébergé complet ni un compilateur reconstruit par
Gs++ lui-même.

## Opérateurs libres et qualifications de valeur — développement après alpha.8

Le parseur auto-hébergé représente maintenant une déclaration libre
`opérateur` / `operator` par le genre compact `SurchargeOperateur`, à la racine
de son espace de noms. Le bootstrap marque lui aussi explicitement ces
fonctions comme opérateurs. Cette convergence permet de comparer la même forme
d’AST au lieu de déduire artificiellement l’opérateur pendant les tests.

La résolution conserve l’ordre du langage : elle recherche d’abord un
opérateur membre sur le type de l’opérande gauche, y compris dans ses bases. Si
aucun groupe membre n’existe, elle classe les opérateurs libres visibles dans
l’espace global ou dans l’espace courant. Un opérateur binaire est recherché
dès que l’un des deux opérandes est une classe ; les opérateurs libres unaires
`!` et `~` sont également pris en charge. Un groupe membre présent mais
incompatible reste prioritaire et produit le même diagnostic que le bootstrap,
sans repli silencieux sur un groupe libre.

Le classement réutilise l’arité exacte, les références exigeant une valeur
gauche, les conversions d’héritage et les littéraux entiers représentables.
Pour les valeurs non pointeurs, `constante` / `const` et `volatile` ne changent
plus l’identité de valeur employée par le classement, conformément à
`TypesValeurEgaux` du bootstrap. Les pointeurs conservent en revanche leurs
qualifications exactes. Le diagnostic `59` (`AriteOperateurInvalide`) refuse à
présent les déclarations membres et libres dont l’arité ne correspond pas à
l’opérateur, sans renuméroter les codes précédents ni modifier l’ABI publique.

Le corpus positif bilingue sélectionne quatre opérateurs libres : deux
surcharges `classe + entier`, une forme `entier + classe` et un `!` unaire. Les
paramètres objets sont liés par `constante T&` / `const T&`. Huit refus
français et anglais couvrent l’absence de surcharge compatible, l’ambiguïté et
les arités invalides, ce qui porte le total à cent dix-neuf corpus sémantiques
négatifs dont le code, la ligne et la colonne sont comparés au bootstrap. Un
test du compilateur natif vérifie en plus la génération des quatre appels et
l’identité du code produit par les syntaxes française et anglaise.

La matrice de développement passe 4/4 sous Visual Studio 2026 et 5/5 sous
GNU/Linux 11.4. Les deux chaînes reconstruisent une image `Frontend.GsE` GsE
1.0 de 275 345 octets avec 73 exports, acceptée par `gseverifier` et identique
bit à bit. Son SHA-256 est
`f3762720abf876c24d0a4e2da0a81f75026425b6f2048c84bfe1449a0b7b1963`.
Les tailles ABI restent respectivement de 64, 48, 32, 56 et 120 octets pour
`NoeudDeclaration`, `SymboleSemantique`, `ResolutionSemantique`,
`ResultatAnalyseSemantique` et `RequeteAnalyseSemantique`.

Cette preuve valide la représentation, la sélection et l’arité des opérateurs
libres décrits ci-dessus. Elle ne valide pas encore toutes les combinaisons
d’opérateurs intrinsèques, toutes les conversions implicites ou l’auto-
reconstruction fonctionnelle du compilateur.

## Opérateurs intrinsèques — développement après alpha.8

Après la sélection prioritaire d’une surcharge membre ou libre, la passe
auto-hébergée valide maintenant les six formes unaires `+`, `-`, `!`, `~`, `&`
et `*`, puis les dix-huit formes binaires arithmétiques, de décalage, binaires,
logiques et de comparaison. Les opérateurs surchargés déjà résolus conservent
leur type de retour et ne sont jamais reclassés comme intrinsèques.

La matrice applique les mêmes règles que le bootstrap : `!`, `&&` et `||`
exigent des scalaires ; les calculs, décalages et opérations bit à bit exigent
des entiers ; les types de valeur doivent coïncider après suppression des
qualificatifs ; seuls `==` et `!=` sont autorisés sur les pointeurs et les
booléens. Les pointeurs de fonction sont reconnus explicitement comme des
adresses scalaires malgré leur hachage compact distinct. Les littéraux entiers
représentables sont adaptés à l’autre opérande à gauche comme à droite, et le
type de résultat non qualifié reflète cette adaptation. La valeur unaire
`-9_223_372_036_854_775_808` conserve le cas limite `entier64` du bootstrap.

Les diagnostics 60 à 68 identifient respectivement la négation non scalaire,
l’opérateur unaire non entier, l’opérateur logique non scalaire, les types
d’opérandes différents, le décalage non entier, le calcul non entier, la
comparaison d’un type non scalaire, la comparaison ordonnée de pointeurs et la
comparaison ordonnée de booléens. Ils prolongent la table existante sans
renuméroter ses entrées ni modifier l’ABI publique.

Deux corpus positifs français et anglais parcourent les vingt-quatre formes
intrinsèques au total, en réunissant cette matrice et les cas `&` / `*` déjà
validés. Ils couvrent aussi les entiers signés et non signés, l’adaptation d’un
littéral vers `entier64`, les pointeurs ordinaires et les pointeurs de fonction.
Dix-huit nouveaux refus bilingues couvrent les neuf diagnostics, ce qui porte
le total à cent trente-sept corpus négatifs dont le code, la ligne et la colonne
sont comparés au bootstrap.

La matrice de développement passe 4/4 sous Visual Studio 2026 et 5/5 sous
GNU/Linux 11.4. Les deux chaînes reconstruisent une image `Frontend.GsE` GsE
1.0 de 281 169 octets avec 73 exports, acceptée par `gseverifier` et identique
bit à bit. Son SHA-256 est
`fcc65ad812685ba84c468faf47515427efb2a49b13580a7388d8ee2218027dcf`.
Les tailles ABI restent respectivement de 64, 48, 32, 56 et 120 octets pour
`NoeudDeclaration`, `SymboleSemantique`, `ResolutionSemantique`,
`ResultatAnalyseSemantique` et `RequeteAnalyseSemantique`.

Cette preuve clôt la matrice de typage des opérateurs intrinsèques. Elle ne
revendique pas encore toutes les conversions implicites, la résolution globale,
un frontend auto-hébergé complet ni un compilateur reconstruit fonctionnellement
par Gs++ lui-même.

## Conversions, références et mutations — développement après alpha.8

La passe auto-hébergée applique maintenant le même contrat que le bootstrap aux
initialiseurs scalaires et aux trois chemins de passage par référence :
déclaration locale, appel direct surchargé et appel indirect par pointeur de
fonction. Une référence exige une valeur gauche de type identique ou compatible
par héritage. Une destination non constante ne peut pas recevoir une expression
constante, y compris un champ ou un élément observé au travers d’un objet
constant. La conversion `Dérivée*` vers `Base*` refuse également la perte de
qualification constante.

Les affectations sont désormais contrôlées après résolution récursive de leurs
deux opérandes. La cible doit être une valeur gauche modifiable ; les valeurs
constantes et les tableaux complets ne sont pas assignables ; la valeur source
doit pouvoir initialiser le type cible. Les instructions `retourner` / `return`
exigent une valeur pour une fonction non `vide` et appliquent la même
compatibilité d’initialisation au résultat fourni. L’adaptation des arguments
littéraux couvre aussi les formes unaires `+N` et `-N` représentables.

Les diagnostics 69 à 75 couvrent la liaison de référence incompatible, la cible
d’affectation non modifiable, l’affectation constante ou de tableau, le type
d’affectation incompatible, la valeur de retour absente et le type de retour
incompatible. Les codes 0 à 68 et les tailles ABI restent inchangés. Dix-huit
refus différentiels bilingues portent le total à cent cinquante-cinq corpus dont
le code, la ligne et la colonne correspondent au bootstrap.

| Code | Diagnostic | Condition refusée |
| ---: | --- | --- |
| 69 | `LiaisonReferenceIncompatible` | la source n’est pas une valeur gauche compatible ou retire `constante` |
| 70 | `CibleAffectationNonModifiable` | la cible n’est pas une valeur gauche |
| 71 | `AffectationValeurConstanteInterdite` | la cible ou son objet propriétaire est constant |
| 72 | `AffectationTableauInterdite` | la cible est encore un tableau complet |
| 73 | `TypeAffectationIncompatible` | la source ne peut pas initialiser le type cible |
| 74 | `ValeurRetourAttendue` | une fonction non `vide` retourne sans valeur |
| 75 | `TypeRetourIncompatible` | la valeur ne peut pas initialiser le type de retour |

La matrice locale complète passe 4/4 sous Visual Studio 2026 et 5/5 sous
GNU/Linux. Les deux chaînes produisent la même image `Frontend.GsE` GsE 1.0 de
285 537 octets, avec 73 exports, acceptée par `gseverifier`. Son SHA-256 est
`675c0579adc26431fc25ce846a390c2d68351fadc4c218d0148906333a8a5d59`.

## Contraintes des déclarations et initialiseurs globaux — développement après alpha.8

La passe auto-hébergée applique maintenant les cinq contraintes structurelles
que le bootstrap vérifie avant l’analyse d’un initialiseur global. Un objet de
classe par valeur est refusé, y compris dans un tableau, tandis qu’un pointeur
vers cette classe reste autorisé. Une globale ne peut être ni une référence ni
une valeur de type `vide`. Une déclaration importée avec `externe` / `extern`
ne peut pas être simultanément exportée avec `publique` / `public`. Enfin, une
globale `constante` / `const` qui n’est pas une adresse doit posséder un
initialiseur.

La reconstruction privée du type relit désormais la forme complète la plus à
gauche avant le nom de la déclaration. Elle conserve ainsi les qualificatifs
et les noms qualifiés complets au lieu de retenir seulement leur suffixe. Les
tableaux, pointeurs ordinaires, références et pointeurs de fonction sont
distingués sans changer `NoeudDeclaration` ni aucune autre structure de l’ABI
publique.

| Code | Diagnostic | Condition refusée |
| ---: | --- | --- |
| 76 | `ObjetClasseGlobalInterdit` | la globale contient un objet de classe par valeur |
| 77 | `ReferenceGlobaleInterdite` | le type global est une référence |
| 78 | `GlobaleVideInterdite` | le type global est `vide` sans indirection |
| 79 | `GlobaleExternePubliqueInterdite` | la même globale est importée et exportée |
| 80 | `GlobaleConstanteNonInitialisee` | une globale constante non-adresse n’a pas d’initialiseur |

Une seconde passe parcourt maintenant chaque initialiseur global avec la même
destination récursive que le validateur des agrégats. Elle impose une liste
pour une structure ou une union, descend dans les tableaux et les champs,
accepte un pointeur de fonction seulement lorsqu’il vise directement une
fonction, et refuse toute initialisation d’un pointeur de données. Les feuilles
scalaires doivent être des littéraux entiers, des constantes d’énumération
qualifiées, des opérateurs constants ou une conversion constante. Cette
validation porte sur la forme ; elle ne calcule pas encore la valeur finale.

| Code | Diagnostic | Condition refusée |
| ---: | --- | --- |
| 81 | `InitialiseurGlobalAgregeRequis` | une structure ou union globale reçoit autre chose qu’une liste |
| 82 | `CiblePointeurFonctionGlobalInvalide` | un pointeur de fonction global ne vise pas directement une fonction |
| 83 | `InitialiseurPointeurDonneesGlobalInterdit` | un pointeur de données global possède un initialiseur |
| 84 | `InitialiseurGlobalNonConstant` | une feuille scalaire globale n’est pas structurellement constante |

Vingt refus différentiels français et anglais couvrent ensemble les deux
sous-tranches et portent le total à cent soixante-quinze corpus dont le code,
la ligne et la colonne correspondent au bootstrap. Le corpus positif bilingue
protège les cas autorisés : import constant, constantes littérales ou
calculées, conversion constante, agrégat imbriqué, constante d’énumération
qualifiée, pointeurs non initialisés et cible directe de fonction.

La matrice locale complète passe 4/4 sous Visual Studio 2026 et 5/5 sous
GNU/Linux. Les deux chaînes reconstruisent une image `Frontend.GsE` GsE 1.0 de
295 646 octets avec 73 exports, acceptée par `gseverifier` et identique bit à
bit. Son SHA-256 est
`03422775cda6395fa57d00eeaf0b75ed96394b0ceb971731cf51d936bd2b3a9c`.

## Constantes numériques et valeurs d’énumération — développement après alpha.8

La passe sémantique auto-hébergée calcule maintenant les valeurs des formes
constantes déjà reconnues : littéraux entiers et booléens, constantes
d’énumération résolues, opérateurs unaires `+`, `-`, `!` et `~`, opérations
arithmétiques et binaires, décalages, comparaisons, égalités, courts-circuits
`&&` / `||` et conversions scalaires explicites. Les calculs suivent la largeur
et le caractère signé ou non signé du type résolu, y compris le cas limite de
la division de la valeur minimale `entier64` par `-1`.

Les énumérateurs sont évalués dans l’ordre source. La valeur implicite commence
à zéro et progresse à partir de la valeur précédente ; une valeur explicite
doit être une constante entière autre qu’un booléen ou une énumération et rester
dans la plage `entier32`. Une référence qualifiée ne voit que les énumérateurs
antérieurs, comme dans le bootstrap. Les valeurs et leurs états sont stockés
dans l’arène privée de l’analyse, sans modifier les structures de l’ABI publique.

Le validateur des initialiseurs globaux réutilise cette évaluation pour refuser
une division ou un modulo par zéro et une valeur qui ne tient pas dans sa
destination signée ou non signée. Le contrôle descend également dans les
tableaux, structures et unions. Les diagnostics historiques des initialiseurs
locaux et de champs restent inchangés ; les nouveaux diagnostics de plage sont
réservés au chemin global ajouté dans cette tranche.

| Code | Diagnostic | Condition refusée |
| ---: | --- | --- |
| 85 | `ValeurEnumerationNonEntiere` | l’initialiseur d’un énumérateur n’est pas un entier admissible |
| 86 | `ValeurEnumerationNonConstante` | l’initialiseur entier n’est pas une expression constante |
| 87 | `ValeurEnumerationHorsPlage` | la valeur explicite sort de la plage `entier32` |
| 88 | `DebordementEnumerationSuivante` | une valeur implicite devrait suivre `2_147_483_647` |
| 89 | `DivisionConstanteParZero` | une division ou un modulo constant utilise zéro comme diviseur |
| 90 | `ConstanteHorsPlageType` | la valeur globale ne tient pas dans son type de destination |

Vingt nouveaux refus différentiels français et anglais portent la matrice à
cent quatre-vingt-quinze corpus. Chaque corpus compare le code auto-hébergé, la
ligne et la colonne au diagnostic du bootstrap. Le corpus positif couvre aussi
les opérations arithmétiques, bit à bit, décalages signés, comparaisons,
conversions étroites valides, courts-circuits et valeurs implicites
d’énumération.

La matrice locale complète passe 4/4 sous Visual Studio 2026 et 5/5 sous
GNU/Linux. Les deux chaînes reconstruisent la même image `Frontend.GsE` GsE 1.0
de 317 022 octets avec 73 exports. Les deux copies sont acceptées par
`gseverifier` et possèdent le SHA-256
`9446947bb60908d8a7df57b53e8397de3b387f15b1cff7c881bd7a7d0f22b281`.

Cette tranche ne sérialise pas encore les valeurs calculées dans les octets des
globales et ne produit pas leurs relocalisations. Elle ne constitue donc ni un
frontend auto-hébergé complet ni un compilateur reconstruit fonctionnellement
par Gs++.

## Émission des données globales — développement du 20 septembre 2026

La nouvelle API exportée par `Frontend.GsE` réutilise l’analyse sémantique et
son arène privée, puis produit les données initiales et les informations de
stockage nécessaires au futur backend auto-hébergé :

```text
GalacticShrine::GsPP::Autohebergement::EmettreGlobales(
    RequeteEmissionGlobales*) -> ErreurAnalyseSemantique
```

L’alias anglais `EmitGlobals` pointe sur la même fonction. Les nouvelles
structures n’étendent ni ne déplacent les champs des contrats AST/sémantique
existants. Leur disposition sous `GsAbi:x64-ms-v1` est contrôlée dans les tests :

| Structure | Taille | Rôle |
| --- | ---: | --- |
| `GlobaleEmise` / `EmittedGlobal` | 48 octets | nœud, symbole, décalage, taille, alignement et drapeaux |
| `RelocalisationGlobale` / `GlobalRelocation` | 32 octets | nœud global, décalage relatif, symbole cible et genre |
| `ResultatEmissionGlobales` / `GlobalEmissionResult` | 56 octets | diagnostic, quatre besoins et consommation de l’arène |
| `RequeteEmissionGlobales` / `GlobalEmissionRequest` | 136 octets | source, AST, trois tampons/capacités et résultat à l’offset 80 |

La requête fournit la même source et le même AST que `AnalyserSemantique`.
Un premier appel avec trois capacités nulles mesure les besoins sans remplir
les tampons. Le résultat indique le nombre de globales définies, le nombre
d’octets initialisés, le stockage zéro et le nombre de relocalisations.
Après allocation des trois tampons, un second appel effectue l’émission.
Une unité sans aucune globale définie réussit dès la mesure.

| Code | Diagnostic | Capacité insuffisante |
| ---: | --- | --- |
| 91 | `CapaciteGlobalesInsuffisante` | descripteurs de globales |
| 92 | `CapaciteDonneesGlobalesInsuffisante` | octets initialisés |
| 93 | `CapaciteRelocalisationsGlobalesInsuffisante` | relocalisations |

Les besoins sont complets pour ces trois diagnostics, testés dans cet ordre.
Une erreur sémantique conserve son code et sa position ; les compteurs d’une
analyse en erreur ne constituent pas une émission valide. Les trois tampons
restent intacts en cas d’erreur ou de capacité insuffisante. Ils appartiennent
à l’appelant et ne doivent se recouvrir ni entre eux, ni avec la source, l’AST
ou la requête. Une capacité non nulle avec un pointeur nul est refusée.
Les allocations privées sont libérées avant le retour, y compris sur erreur.

Les globales apparaissent dans l’ordre source, sans stockage pour les imports.
Le bit 0 de `Drapeaux` désigne la zone initialisée, le bit 1 une globale publique ;
`Reserve` vaut zéro. Les zones initialisée et zéro sont alignées séparément.
`Decalage` est relatif à la zone choisie et `NombreOctetsZero` n’exige aucun
tampon. Chaque zone est limitée à `UINT32_MAX`, comme les offsets du backend
actuel ; un dépassement retourne `TailleObjetInvalide` sans écrire les sorties.

Les entiers, booléens et énumérations sont encodés en little-endian, selon leur
largeur et leur signe. Les tableaux multidimensionnels, structures et unions
sont parcourus récursivement ; éléments omis, listes vides et octets de
remplissage valent zéro. Une cible de fonction directe ou précédée de `&`
produit huit octets nuls et une relocalisation de genre `1` (adresse absolue
64 bits). Son décalage est relatif à la globale ; son indice cible désigne la
table obtenue par `AnalyserSemantique` avec les mêmes entrées, que la fonction
soit définie ou importée.

Les tests comparent les octets avec `GenerateurX64::Generer`, les positions et
tailles avec ses symboles, et les relocalisations avec leurs cibles exactes.
Ils couvrent cinq corpus, dont une paire français/anglais avec valeurs limites,
signes, conversions, énumérations relatives à l’espace courant, agrégats,
unions, tableaux de callbacks, imports et zones zéro. Les tailles exactes ou
insuffisantes, sentinelles, appels répétés, arguments invalides, erreurs sans
écriture partielle et dépassements de section sont aussi vérifiés. Huit refus
différentiels supplémentaires portent la matrice sémantique à 203 corpus.

Validation locale : **4/4 CTest sous Visual Studio 2026, 5/5 sous GNU/Linux**,
avec la cible de préparation `tests_gspp_preparation`. Les deux chaînes
produisent un `Frontend.GsE` GsE 1.0 identique de **328 270 octets**, ABI 1,
75 exports et deux imports, accepté par `gseverifier`. SHA-256 :
`1bf0c652b7cd6d51c8434cbc7d8fc2a21da00385dbf26fd3965426a3a5a70d5d`.

Cette tranche émet des **données en mémoire**, pas un fichier GsObj complet.
Le bootstrap reste responsable de la génération du code machine, des tables
virtuelles, des littéraux chaînes, des écrivains de formats et de la liaison.
Les nouvelles cibles natives et les SDK de la décision multi-cible restent
prévus, non implémentés ici. Il ne s’agit pas d’un compilateur auto-hébergé complet.

## Conversions explicites — alpha.9

Le frontend valide maintenant les expressions `convertir<Type>(valeur)` et
`cast<Type>(value)` avant de les laisser participer à une initialisation, une
affectation, un retour, une opération ou un appel indirect. Le type cible est
relu à la position du mot-clé dans la source, sans agrandir l’AST public.
La signature d’un callback converti reste disponible pour les opérations
suivantes, y compris son appel indirect.

| Code | Diagnostic | Condition refusée |
| ---: | --- | --- |
| 94 | `CibleConversionNonScalaire` | conversion vers `vide`, une structure, une union ou une classe par valeur |
| 95 | `SourceConversionNonScalaire` | source agrégée, tableau ou valeur `vide` |
| 96 | `ConversionPointeurEntierInterdite` | conversion entre catégories adresse et numérique |
| 97 | `SignatureConversionIncompatible` | signature ou qualifications de pointeur de fonction incompatibles |
| 98 | `ConstanteConversionHorsPlage` | constante non représentable dans le type converti |
| 99 | `TypeConversionIntrouvable` | cible nommée inconnue |

Le contrôle de plage s’applique aux globales et aux agrégats, aux expressions
locales, aux conversions imbriquées et aux initialiseurs d’énumérateurs. Il
ne réduit pas silencieusement une constante hors plage. Une conversion vers
un booléen conserve la règle zéro/non-zéro du bootstrap ; les conversions
étroites d’une valeur calculée à l’exécution restent autorisées. Chaque
conversion est contrôlée même dans une branche à court-circuit, comme dans le
bootstrap. Le résultat perd le marqueur de référence, sans devenir une valeur
gauche, mais conserve les qualificatifs de pointeur explicitement demandés.

Les 27 paires françaises/anglaises de refus ajoutent 54 corpus, pour un total
de **257**. Les cas positifs incluent les pointeurs `constante`/`volatile`, les
conversions numériques imbriquées, les valeurs d’énumération, les références
converties en valeurs et les appels à travers des callbacks convertis.
Les tailles de l’AST et des requêtes publiques ne changent pas.

L’image alpha.9 est identique sous MSVC et GNU : **334 318 octets**, 75 exports,
format GsE 1.0, ABI 1, SHA-256
`aef86685f1444466f78951725e0b08cbfe81f70dcb9c9ba5f38eb69cc1597c95`.
La [matrice de publication](Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.9.md)
regroupe les tests, benchmarks et contrôles de paquets de cette publication.

## Adaptations implicites — développement après alpha.9

La tranche du 20 septembre 2026 remplace le contrôle limité aux littéraux par
une évaluation partagée des constantes entières composées. Les appels
surchargés, méthodes, constructeurs, opérateurs et initialiseurs appliquent les
règles du bootstrap, sans introduire d'élargissement automatique des variables
numériques ni de conversion implicite entre booléens, énumérations et entiers.

- une surcharge de type exact reste prioritaire ; une constante représentable
  vaut un point de conversion, un dépassement exclut le candidat et une égalité
  des meilleurs scores reste ambiguë ;
- les qualificatifs scalaires sont traités selon le bootstrap ; les identités
  qualifiées des pointeurs et les restrictions des liaisons de références sont
  conservées, y compris pour les références locales de classe vers une base ;
- une référence locale de classe n'est plus traitée comme un objet à construire ;
- les énumérateurs sont résolus et calculés dans l'ordre avant leur utilisation
  dans les expressions des fonctions, sans dupliquer leurs résolutions ;
- les erreurs de calcul, dont la division par zéro, ne sont plus remplacées par
  un diagnostic générique de surcharge lors de la recherche de candidats ;
- les types adaptés des opérandes sont conservés dans l'arène privée de
  l'analyse. Les comparaisons, divisions et restes constants puis les octets
  globaux sont comparés au bootstrap. Le type écrit d'un `convertir` / `cast`
  reste distinct de l'adaptation implicite appliquée à son résultat.

La suite ajoute **42 refus français/anglais**, portant le corpus négatif de
257 à **299**. Elle ajoute aussi **36 corpus sémantiques valides**, dont six
vérifiant la surcharge effectivement choisie, ainsi que **deux corpus bilingues
d'émission** comparant les octets des globales. Les cas refusés comparent le
code attendu et la position au bootstrap ; ils couvrent aussi les pertes de
qualificatifs, les références vers des temporaires et les limites de `caractère`.

Validation locale : CTest **4/4 Windows / Visual Studio 2026**, **5/5 GNU/Linux**,
conformité **20/20** sur chaque chaîne et contrôle de style réussi. Commandes :

```text
cmake --build --preset windows-release --target espace_travail --parallel
ctest --preset windows-release
cmake --build --preset linux-release --target espace_travail --parallel 4
ctest --preset linux-release
```

L'image de développement est identique entre MSVC et GNU : **332 926 octets**,
**75 exports**, SHA-256
`da0ce2c48274649216a64bda033bb31c4d1469ebfe68497da952f1d8d02d2201`.
L'AST public, les requêtes d'analyse et d'émission, les formats 1.0 et l'ABI 1
ne changent pas. Cette image n'est pas celle de la release alpha.9 ; `VERSION`
reste à `0.27.0-alpha.9` tant qu'une nouvelle publication n'est pas préparée.

Les contrôles smoke réussissent également **4/4** sur chaque chaîne. Les deux
distributions de développement ont été produites par CPack puis extraites dans
des dossiers neufs : vérification de l'image, compilation de `Bonjour.Gs++`,
compilation/exécution d'un programme utilisant une surcharge et une constante
composée (retour **42**), puis suite d'auto-hébergement complète contre le
`Frontend.GsE` extrait. Ces paquets locaux se trouvent sous
`D:\『Projet』 Archives Transition\Retrait-GSLSE-2026-10-02\GSLSE\Construction\GsPlusPlus-Development\ConversionsImplicites\Packages\`
(archives locales, anciennement sous `D:\GSLSE`) ; ils
ne remplacent pas les archives alpha.9 publiées sur GitHub.

Cette tranche ne clôt pas toute la matrice des conversions et qualifications.
En particulier, les définitions de fonctions retournant une référence restent
interdites par le bootstrap ; aucune nouvelle capacité de ce type n'est revendiquée.

## Types composés — développement après alpha.9

La tranche du 21 septembre 2026 complète la reconnaissance des références de
pointeurs de fonction et des indirections profondes. Un index de descriptions
de types est construit dans l'arène privée à partir des jetons de la source ;
les transformations de référence, qualification et indirection conservent la
signature d'origine. Aucun champ n'est ajouté aux interfaces publiques.

Les nouveaux tests couvrent :

- les références locales et paramètres de callbacks, leur passage aux appels
  directs ou indirects, la surcharge réellement sélectionnée et leur invocation ;
- la prise d'adresse d'une référence de callback, le déréférencement de son
  emplacement et la liaison d'un élément de tableau de callbacks ;
- les signatures imbriquées, dont les appels renvoyant un callback par valeur
  ou par référence. Ce dernier cas concerne une signature de pointeur de
  fonction acceptée par le bootstrap, pas une nouvelle autorisation de définir
  des fonctions retournant des références ;
- les qualifications `constante` et `volatile`, sans autoriser les pertes de
  qualification ni les liaisons à des temporaires refusées par le bootstrap ;
- les pointeurs primitifs et nommés à trois et cinq indirections, leurs
  références et tableaux multidimensionnels, y compris la disposition globale ;
- la distinction entre une fonction et l'emplacement d'un pointeur de fonction :
  ce dernier peut être converti via `vide*` comme le bootstrap, sans autoriser la
  conversion d'un pointeur de fonction lui-même vers `vide*`.

Les types d'éléments et d'indexations sont relus depuis la déclaration au lieu
de dépendre seulement d'une recherche limitée à deux ou quatre indirections.
Les dimensions restantes sont réappliquées au type complet, signature comprise.

La suite ajoute **30 corpus valides français/anglais**, dont deux contrôlent
la surcharge choisie, **36 refus différentiels**, portant le total de 299 à
**335**, et **deux corpus d'émission** comparant les tailles, alignements et
octets globaux au bootstrap. Les refus contrôlent code, ligne et colonne.

Validation locale : CTest **4/4 Windows / Visual Studio 2026** et **5/5 GNU/Linux**,
conformité **20/20** et smoke **4/4** sur chaque chaîne. Les commandes de
construction et CTest restent celles indiquées dans la section précédente.
L'image reconstruite est identique entre MSVC et GNU : **337 982 octets**,
**75 exports**, SHA-256
`c6c7fd50bd804a3782a8ffa3e889098d2c4ceeaf154355b301394513522f1bcc`.
Les formats 1.0 et l'ABI 1 sont inchangés. Les mesures de la section précédente
décrivent la tranche du 20 septembre, non cette nouvelle image.

Cette couverture ne constitue pas une preuve d'exhaustivité des conversions,
des qualifications ou des types nommés dans toutes les signatures imbriquées.
Le frontend 0.27 reste partiel. `VERSION` demeure à `0.27.0-alpha.9` ; ces
changements locaux ne modifient pas la release publique alpha.9.

## Types nommés dans les signatures — développement après alpha.9

Cette étape du 21 septembre 2026 poursuit la tranche précédente. Les types
nommés relus dans les déclarations et les conversions sont résolus dans leur
contexte, récursivement dans les signatures de pointeurs de fonction. Dans
`espace N`, `S` et `N::S` désignent le même type si aucun type global `S` ne
prend priorité ; `A::S` et `B::S` restent distincts, même si leurs signatures
étaient écrites avec le même nom court.

La recherche suit le bootstrap : d'abord le nom complet écrit, puis ce nom
préfixé par l'espace effectif de la déclaration. Elle ne parcourt pas librement
tous les espaces ni tous leurs parents. Le contexte des paramètres et corps
de méthodes tient compte de l'espace de classe utilisé par le bootstrap.

La requête et ses nœuds sont copiés dans l'état privé de l'analyse. Les
empreintes de types de cette copie et des symboles de travail sont normalisées,
sans modifier l'AST fourni par l'appelant. Le pointeur d'entrée et les octets
des nœuds sont contrôlés après les analyses valides et les refus différentiels.
Les requêtes de capacité et l'émission globale empruntent le même chemin.

Le diagnostic bilingue **100** (`TypeNommeIntrouvable` / `UnknownNamedType`)
signale un type déclaré inconnu, y compris au sein d'une signature imbriquée.
Le diagnostic **99** reste utilisé pour une cible de conversion inconnue.
Les dispositions binaires des structures publiques, les formats 1.0 et l'ABI 1
ne changent pas ; les empreintes sémantiques de noms qualifiés peuvent désormais
différer des empreintes syntaxiques d'entrée, ce qui est intentionnel.

Un accès membre utilisé comme cible d'appel recherche d'abord une méthode,
puis un champ si aucune méthode de ce nom n'existe. Un champ callback passe
alors par la validation des appels indirects, sans contourner visibilité,
arité, qualifications ou identité des types.

La matrice ajoute **32 corpus valides français/anglais**, **42 refus** (total
**377**) et **deux corpus d'émission globale**. Elle couvre les structures,
énumérations, signatures imbriquées, noms relatifs, homonymes, priorité du type
global, références, conversions, champs callbacks et méthodes. Les empreintes
canoniques sont explicitement comparées pour des noms équivalents et distincts ;
l'émission compare également les relocalisations de callbacks globaux et de
tableaux de champs au bootstrap.

Validation locale : CTest **4/4 Windows / Visual Studio 2026**, **5/5 GNU/Linux**,
conformité **20/20** et smoke **4/4** sur chaque chaîne. Le `Frontend.GsE`
reconstruit est identique entre MSVC et GNU : **344 462 octets**, **75 exports**,
SHA-256 `563a14af3eae573ab4a4f789c596d5ca049f2fbda52706ef3ed268bb62012fda`.
Les tailles des sections précédentes sont les mesures de leurs étapes
respectives. Cette tranche est locale, non publiée, et ne clôt pas le frontend
0.27 ni l'ensemble des familles sémantiques du bootstrap.

## Contraintes des signatures — développement après alpha.9

Cette tranche du 21 septembre 2026 contrôle les signatures déclarées, même
lorsqu'elles ne sont jamais appelées. La validation récursive des callbacks
refuse les paramètres `vide` par valeur et applique les limites du bootstrap :
quatre paramètres au maximum, réduits à trois pour un retour de structure,
union ou classe par valeur. Un retour par pointeur ou référence et un retour
d'énumération ne sont pas assimilés à un retour d'agrégat par valeur.

Pour les fonctions déclarées, les paramètres tableaux, `vide` et `vide&`, ainsi
que les retours par référence, sont refusés. Les références de tableaux sont
contrôlées avant les contraintes propres au contexte de déclaration. Les limites
de quatre/trois paramètres comptent aussi le récepteur implicite des méthodes
et constructeurs ; aucune extension de l'ABI machine n'est introduite.

Les restrictions des définitions ne sont pas imposées arbitrairement aux
signatures de callbacks : les cas de référence acceptés par le bootstrap dans
ces signatures restent acceptés. Les erreurs imbriquées sont conservées dans
l'ordre de validation des types, sans être remplacées par une erreur de nom ou
d'arité de la signature englobante.

| Code | Identifiant français | Identifiant anglais |
|---:|---|---|
| 101 | `ReferenceTableauInterdite` | `ArrayReferenceForbidden` |
| 102 | `ParametreCallbackInvalide` | `InvalidCallbackParameter` |
| 103 | `AriteSignatureCallbackInvalide` | `InvalidCallbackSignatureArity` |
| 104 | `RetourReferenceInterdit` | `ReferenceReturnForbidden` |
| 105 | `ParametreFonctionInvalide` | `InvalidFunctionParameter` |
| 106 | `AriteSignatureFonctionInvalide` | `InvalidFunctionSignatureArity` |

La relecture d'une déclaration d'opérateur reconnaît désormais le type qui
précède le mot-clé `opérateur` / `operator`. La sélection des opérateurs libres
s'applique également aux structures et unions, pas seulement aux classes ; les
énumérations ne sont pas traitées comme des agrégats. Les surcharges compatibles,
absentes et incompatibles sont couvertes différentiellement.

La suite ajoute **42 corpus valides français/anglais** et **66 refus
différentiels**, portant le total à **443**. Deux corpus d'émission comparent
les callbacks globaux aux limites autorisées. Quatre refus supplémentaires de
l'API d'émission vérifient que des signatures invalides ne modifient aucun
tampon de sortie ; ces contrôles ne sont pas ajoutés au total différentiel.
Les contrôles d'immuabilité de l'AST et de capacité restent actifs.

Validation locale : CTest **4/4 Windows / Visual Studio 2026**, **5/5 GNU/Linux**,
conformité **20/20** et smoke **4/4** sur chaque chaîne. Les images `Frontend.GsE`
sont identiques : **348 686 octets**, **75 exports**, SHA-256
`fbb9406629bee93ea9046a577eeeddf41580fbcfd11ff20458085e07bfb9df30`.
L'énumération des diagnostics est étendue, mais les dispositions des structures
publiques, les formats 1.0 et l'ABI 1 restent inchangés.

Cette tranche ne clôt pas les autres contraintes de déclarations, la résolution
des alias de types ou la totalité du frontend. Elle n'est pas une publication :
`VERSION` reste à `0.27.0-alpha.9`, et la migration `.GsA` vers `.Glib` reste
prévue pour 0.28.0.

## Chaînes d'alias de champs — développement après alpha.9

Cette tranche locale du 2 octobre 2026 complète la normalisation des alias de
champs dans le frontend auto-hébergé. Tous les alias d'une structure, union ou
classe sont résolus après ses champs, même si aucun accès ni constructeur ne
les utilise. Les cibles déclarées plus loin et les chaînes sont acceptées.
Le champ final doit appartenir directement au type déclarant : une globale,
une fonction ou un champ seulement hérité n'est pas une cible valide.

Le parcours est itératif. Son cache de cibles et d'états appartient à l'arène
privée, libérée à la fin de chaque requête ; l'AST de l'appelant et les symboles
publics d'alias restent intacts. Les accès `.` / `->`, adresses, tableaux,
callbacks et listes d'initialisation de constructeurs utilisent le symbole du
champ canonique. Sa visibilité, sa qualification constante et son ordre
d'initialisation continuent à s'appliquer. Un alias ne crée aucun champ ni
stockage supplémentaire.

| Code | Français | Anglais |
|---|---|---|
| 107 | `CycleAliasChamp` | `FieldAliasCycle` |
| 108 | `CibleAliasChampIntrouvable` | `UnknownFieldAliasTarget` |

La suite ajoute **30 corpus valides bilingues**, dont une chaîne de 128 alias,
et **40 refus différentiels**. Deux corpus d'émission comparent les octets et
dispositions de structures/unions contenant des alias avec le bootstrap.
Quatre refus d'émission supplémentaires vérifient l'absence d'écriture dans
les tampons lors d'un cycle ou d'une cible inconnue. Ces quatre corpus sont
aussi différentiels : le total est désormais **487**, compté directement par
la suite à l'exécution. Le code, la ligne, la colonne et l'immuabilité de l'AST
sont vérifiés pour chaque refus différentiel.

Validation locale : CTest **5/5 Windows / Visual Studio 2026**, **6/6 GNU/Linux**,
et validation **MSBuild native sans CMake** réussis ; conformité **20/20** sur
chaque construction. Les trois images `Frontend.GsE` sont identiques :
**351 150 octets**, **75 exports**, SHA-256
`3013cf046b5ab2904ee9ccf09732d9e64bffb6ba4a992c140409eb75d2e4c09c`.
Les dispositions ABI publiques, les formats 1.0 et l'ABI 1 restent inchangés.

Cette tranche ne complète pas la résolution des alias de types, fonctions et
globales du frontend auto-hébergé. Elle ne constitue pas une publication :
`VERSION` reste à `0.27.0-alpha.9`, `.GsA` reste l'extension actuelle et la
migration `.Glib` / `.GdLib` reste prévue pour 0.28.0.

## Alias racines — développement après alpha.9

Cette tranche locale du 2 octobre 2026 résout tous les alias racines avant la
normalisation des types déclarés, même s'ils ne sont jamais utilisés. Les
cibles sont des structures, unions, classes, fonctions ou globales, directement
ou par une chaîne d'alias. Comme dans le bootstrap, une énumération ou un
énumérateur n'est pas une cible d'alias ; une fonction surchargée est ambiguë.
Le nom complet écrit est prioritaire sur le nom relatif à l'espace déclarant,
y compris quand des types ou fonctions homonymes existent dans plusieurs espaces.
Les noms d'alias et cibles qualifiés peuvent contenir espaces et commentaires.

Le parcours itératif comprime les chaînes dans un cache privé à trois états,
libéré avec l'arène. Les types des déclarations et signatures imbriquées sont
normalisés dans la copie privée de l'AST ; l'AST public et l'empreinte de cible
des symboles d'alias restent intacts. Les accès, adresses, tableaux, callbacks
et appels libres visent la déclaration canonique. Les types d'héritage valides
peuvent aussi être désignés par un alias. Les globales conservent leur constance
et leur stockage unique ; les callbacks globaux produisent des relocalisations
vers les fonctions canoniques.

| Code | Français | Anglais |
|---|---|---|
| 109 | `CycleAlias` | `AliasCycle` |
| 110 | `CibleAliasIntrouvable` | `UnknownAliasTarget` |
| 111 | `CibleAliasAmbigue` | `AmbiguousAliasTarget` |
| 112 | `AliasFonctionSurchargeeAmbigu` | `AmbiguousOverloadedFunctionAlias` |

Les conflits de noms racines précèdent la résolution des alias ; les erreurs
d'énumération restent prioritaires. La matrice couvre notamment les cibles
inutilisées, cycles internes, cibles anticipées, ambiguïtés de surcharges,
constance des globales, types non utilisables comme alias et appels libres
aux arguments incompatibles. Le code 111 conserve le contrôle des collisions
entre familles de cibles ; la matrice de cette tranche ne revendique pas un
corpus qui atteint ce diagnostic après l'indexation préalable des conflits.

La suite ajoute **64 corpus valides bilingues**, dont une chaîne de 128 alias
et la comparaison des empreintes de types canoniques avec le bootstrap,
**68 refus différentiels** et **six refus d'émission** sans écriture dans les
tampons. Le total différentiel, compté à l'exécution, est **561**. Quatre corpus
d'émission supplémentaires comparent les octets, zones données/zéro et
relocalisations. Les contrôles de capacité et d'immuabilité de l'AST restent
actifs. L'ancien frontend refusait encore un paramètre utilisant un alias de
type valide ; le même test passe avec la nouvelle image.

Validation locale : CTest **5/5 Windows**, **6/6 GNU/Linux** et validation
**MSBuild native sans CMake**, avec conformité **20/20**. Les trois images
`Frontend.GsE` sont identiques : **363 038 octets**, **75 exports**, SHA-256
`3ab9482badd3507757ad8aaee501ff8bd25f5f6e300b31e2e0503e536ade0768`.
Les dispositions publiques, les formats 1.0 et l'ABI 1 restent inchangés.

Les cibles de méthodes qualifiées sont indexées, y compris pour refuser les
surcharges inutilisées, mais les appels via alias de méthodes avec récepteur
implicite ne sont pas validés par cette tranche. Les contraintes restantes
de l'héritage et les exports machine d'alias appartiennent aux travaux suivants.
Ce lot ne constitue ni un frontend complet ni une publication : `VERSION`
reste à `0.27.0-alpha.9`, et `.Glib` / `.GdLib` restent prévus pour 0.28.0.

## Alias de méthodes non liées — développement après alpha.9

Cette tranche locale du 2 octobre 2026 complète les appels via alias racines
de méthodes. L'alias représente une fonction **non liée à une instance** :
le récepteur implicite de la déclaration devient le premier argument explicite
de l'appel, une référence mutable vers la classe déclarante. Par exemple :

```cpp
classe C {
    publique entier32 Lire(entier32 valeur) { retourner valeur; }
};
alias Appeler = C::Lire;
pointeur_fonction<entier32(C&, entier32)> Rappel = Appeler;
publique entier32 F(C& objet) { retourner Rappel(objet, 42); }
```

`Appeler(objet, 42)` et `(&Appeler)(objet, 42)` sont également validés.
Un pointeur `C*` doit être déréférencé ; une référence constante, une instance
temporaire ou un objet de classe étrangère ne peuvent pas servir de récepteur
mutable. Un objet dérivé peut être lié au récepteur de la classe de base ; la
signature d'un callback reste celle de la classe déclarante, sans substitution
de `Base&` par `Derivee&` dans le type du pointeur de fonction.

La signature canonique inclut le récepteur avant les paramètres explicites,
sans ajouter de nœud à l'AST public ni modifier le comptage des paramètres des
appels membres déjà liés. Les chaînes d'alias, espaces de noms, homonymes,
callbacks locaux/globaux, adresses, conversions explicites et retours de
structures utilisent la déclaration réelle. Les relocalisations globales
visent la méthode canonique, pas son alias. Les appels directs portent aussi
le drapeau sémantique de méthode.

Les appels directs vérifient d'abord les arguments, puis la visibilité, comme
le bootstrap. Les signatures et appels indirects conservent ses diagnostics
existants, notamment 21, 25, 45, 54, 55, 90 et 97, avec contrôle du code,
de la ligne et de la colonne. **Limite héritée du bootstrap actuel :** une
prise d'adresse de méthode privée/protégée ne vérifie pas sa visibilité, et
un appel par le callback ainsi obtenu ne la revérifie pas. Ce lot conserve
ce comportement de référence ; il ne revendique pas un contrôle de visibilité
supplémentaire pour ces chemins indirects.

La suite ajoute **48 corpus valides bilingues**, **52 refus différentiels**
et **six refus d'émission** sans écriture partielle. Le total différentiel
compté à l'exécution est **619**. Quatre corpus d'émission supplémentaires
comparent les octets, dispositions et relocalisations, avec sentinelles,
capacités insuffisantes et répétition déterministe. L'AST d'entrée reste
intact. Le test de régression sans récepteur échouait avec l'ancienne image,
qui acceptait l'appel refusé par le bootstrap ; il passe avec la nouvelle.

Validation locale : CTest **5/5 Windows**, **6/6 GNU/Linux** et validation
**MSBuild native sans CMake**, avec conformité **20/20**. Les trois images
`Frontend.GsE` sont identiques : **366 718 octets**, **75 exports**, **deux
imports**, SHA-256
`949903207b8b73b46927efa6730c3e5c7e35211f17aa0877ae12cabed948f9cf`.
Les dispositions publiques, les formats 1.0 et l'ABI 1 restent inchangés.

Cette tranche ne termine pas l'ensemble des contraintes d'héritage ni des
autres familles sémantiques. Elle ne constitue pas une validation intégrale
des appels virtuels via alias, des exports machine d'alias ou du backend
auto-hébergé. Aucune publication : `VERSION` reste à `0.27.0-alpha.9`, et la
migration `.Glib` / `.GdLib` reste prévue pour 0.28.0.

## Consolidation — alpha.10

Le 2 octobre 2026, `VERSION` passe à `0.27.0-alpha.10` pour consolider les lots
développés après alpha.9. Les outils et les métadonnées des images construites
annoncent alpha.10, sans changer les formats ni l'ABI. CTest **5/5 Windows**,
**6/6 GNU/Linux**, validation **MSBuild native sans CMake**, conformité
**20/20** sur les trois constructions et benchmark smoke **4/4** sous
Windows/CMake et GNU/Linux réussissent à nouveau.

Les trois images frontend restent identiques : **366 719 octets**, **75
exports**, deux imports, SHA-256
`4cccad8f5ad8b57168f9942caf00b5d37d4495d7a070071512cc0c1c3a8bf401`.
L'octet supplémentaire par rapport au lot précédent correspond à la version
alpha.10 dans les métadonnées. La suite contrôle toujours **619 refus**.

Les deux paquets extraits réussissent **11/11 contrôles de distribution**,
y compris la suite différentielle contre le frontend livré et l'exécution des
bibliothèques livrées. Les archives de diffusion sont reconstruites depuis un
export du commit signé, puis contrôlées à nouveau avant publication.
Le périmètre complet
et les limites sont dans la
[matrice alpha.10](Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.10.md).

## Déclarations d'héritage — développement après alpha.10

Cette tranche locale du 2 octobre 2026 valide chaque base déclarée, même quand
la classe n'est jamais instanciée. Elle corrige l'acceptation de bases absentes,
non-classes ou non publiques par le frontend précédent. Un test ajouté avant
la correction reproduit le défaut : l'ancienne image accepte l'héritage privé
alors que le bootstrap le refuse à la position de la classe dérivée.

La recherche suit la résolution des types du bootstrap : nom écrit complet
en premier, puis ce nom préfixé par l'espace déclarant. Une classe homonyme
dans un espace sans rapport n'est plus retenue arbitrairement. Une base
qualifiée relative (`A::B` dans `N`, donc `N::A::B`) et les chaînes d'alias de
classes sont résolues vers la même déclaration canonique. Les commentaires
et espaces autour du nom qualifié n'interviennent pas dans son empreinte.
Les bases validées sont conservées dans un cache privé par symbole, utilisé
par les dispositions, conversions dérivée/base et accès aux membres hérités.
Les empreintes des nœuds et symboles publics ne sont pas réécrites pour la base.

| Code | Français | English |
|---|---|---|
| 113 | `HeritageNonPublic` | `NonPublicInheritance` |
| 114 | `BaseClasseInvalide` | `InvalidClassBase` |
| 115 | `AutoHeritageClasse` | `ClassSelfInheritance` |

Le code **100** est réutilisé pour une base introuvable ; structure, union ou
énumération ne sont pas des bases de classe (**114**). L'auto-héritage direct
ou via alias produit **115**. Les cycles indirects suivent le contrôle des
dispositions existant (**57**), comme le bootstrap, et ne sont pas reclassés
en un nouveau diagnostic de table virtuelle. Le parseur refuse déjà la syntaxe
d'héritage sur une structure ou union ; cette tranche ne la rend pas légale.
Les alias d'énumérations restent des cibles non prises en charge dans le
bootstrap et sont refusés comme alias (**110**) avant la clause d'héritage.

Les conflits racines, valeurs d'énumération et alias sont contrôlés avant les
bases. Une base invalide précède ensuite une erreur de type de champ ou de
signature, ainsi que l'absence de fonction. Les diagnostics conservent leur
code, ligne et colonne, y compris dans les corpus multilignes français/anglais.

La matrice ajoute **28 corpus valides**, **60 refus** et **16 refus d'émission**
vérifiant l'absence de toute écriture dans les tampons de données, descriptions
de globales et relocalisations. Le total différentiel contrôlé à l'exécution
est **695**. Quatre corpus d'émission valides supplémentaires sont comparés
octet par octet au bootstrap, avec les contrôles habituels de capacité,
déterminisme et préservation de l'AST.

Validation locale : CTest **5/5 Windows**, **6/6 GNU/Linux**, solution et
validation **MSBuild natives sans CMake**, conformité **20/20** sur chaque
construction. Les trois images `Frontend.GsE` sont identiques : **368 879
octets**, **75 exports**, **deux imports**, SHA-256
`7554e77d693e3f0e14dfe061162c45a1157d6cc7c8091c3af03b2a7ca13052e4`.

Commandes de reconstruction et de validation depuis la racine du dépôt :

```powershell
cmake --build --preset windows-release --target espace_travail --parallel 6
ctest --preset windows-release --output-on-failure
wsl -d Ubuntu -- bash -lc 'cd /mnt/d/Langage-GsPlusPlus && cmake --build --preset linux-release --target espace_travail --parallel 4 && ctest --preset linux-release --output-on-failure'
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' GsPlusPlus.slnx /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' VisualStudio/Validation.vcxproj /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
```

Cette tranche ne valide pas exhaustivement les remplacements de méthodes
virtuelles ni les appels virtuels via alias. Elle ne produit pas un backend
auto-hébergé ni de sorties natives PE/ELF. Les dispositions publiques, formats
1.0 et ABI 1 restent inchangés. `VERSION` demeure à `0.27.0-alpha.10` ; les
archives et preuves de l'alpha.10 publiée ne sont pas modifiées. La migration
`.Glib` / `.GdLib` reste prévue pour 0.28.0.

## Remplacements virtuels — développement après alpha.10

Cette tranche locale du 2 octobre 2026 ajoute le contrôle des méthodes virtuelles
héritées, même si aucun appel ni aucune instanciation ne les utilise. Un test
ajouté avant la correction reproduit l'acceptation d'un `remplacer` dans une
classe sans base compatible par l'ancienne image, alors que le bootstrap refuse
la déclaration à la position de la méthode.

La clé comparée comprend le nom source, les types canoniques des paramètres
explicites et le type de retour. Le récepteur implicite ne fait pas partie de
la comparaison : ses classes déclarante et dérivée diffèrent nécessairement.
Qualifications, niveaux de pointeurs, références, types nommés, alias et
signatures de callbacks doivent correspondre exactement. Une différence de
retour ne constitue pas un remplacement covariant : elle forme une autre clé,
comme dans le bootstrap actuel.

- `remplacer` / `override` sans clé virtuelle compatible est refusé avec
  **116**, `RemplacementVirtuelIncompatible` / `NoMatchingVirtualOverride` ;
- une redéfinition compatible sans `remplacer` / `override`, même marquée
  `virtuel` / `virtual`, est refusée avec **117**, `RemplacementVirtuelRequis` /
  `VirtualOverrideRequired` ;
- les destructeurs partagent le nom source `$destructeur`, indépendamment des
  noms des classes ; les opérateurs sont distingués par leur opérateur ;
- la visibilité n'intervient pas dans l'identité d'une clé virtuelle ; une
  méthode privée virtuelle peut donc être remplacée, comme dans le bootstrap ;
- les ancêtres sont validés avant la dérivée, y compris en cas de déclaration
  anticipée ; une méthode non virtuelle de clé différente dans un intermédiaire
  n'efface pas les clés virtuelles plus anciennes.

Le parcours des classes utilise une pile et des états privés dans l'arène.
Les positions des erreurs et les données publiques ne sont pas réécrites.
Les contrôles de types, signatures et dispositions précèdent le remplacement ;
les vérifications de globales et des corps de fonctions le suivent dans les cas
de priorité testés. Les codes existants ne sont pas renumérotés.

La détection du polymorphisme inclut désormais les destructeurs et opérateurs
virtuels, auparavant exclus. Six corpus français/anglais comparent au bootstrap
les offsets des tables et les pas des tableaux de deux objets : classe avec
destructeur virtuel seul, destructeur remplacé dans une dérivée et opérateur
virtuel introduit après un sous-objet de base non polymorphe. Ils font partie
des **46 corpus valides** de cette tranche. La matrice ajoute **64 refus
différentiels** et **huit refus d'émission** avec tampons intacts, pour un total
de **767 refus** vérifiés par code, ligne et colonne, avec AST préservé.

Quatre corpus d'émission valides supplémentaires contrôlent les globales et
callbacks vers les méthodes canoniques. Le backend C++ ajoute ses tables
virtuelles dans la zone de données avant les globales utilisateur. Le comparateur
isole les tranches des globales de référence et recale leurs offsets et
relocalisations dans une zone commençant à zéro ; il compare leurs octets,
alignements, cibles, capacités et déterminisme. Cette normalisation ne valide
pas l'émission des tables virtuelles par l'API `EmettreGlobales`, dont le contrat
reste limité aux globales utilisateur.

Validation locale avec les mêmes commandes que la section précédente : CTest
**5/5 Windows**, **6/6 GNU/Linux**, solution et validation **MSBuild natives
sans CMake**, conformité **20/20** sur chaque construction. Les trois images
`Frontend.GsE` sont identiques : **374 367 octets**, **75 exports**, **deux
imports**, SHA-256
`c1f616678af8bc86da9d255524fd36a5e6d8fb080619bd6cc18fd478d30b1d87`.

Ce périmètre ne comprend pas les collisions de déclarations ou surcharges,
l'attribution des indices de tables virtuelles dans les résolutions publiques,
leur émission par un backend auto-hébergé, ni une validation exhaustive des
appels virtuels via alias. Les dispositions ABI publiques, formats 1.0 et ABI 1
restent inchangés. `VERSION` demeure à `0.27.0-alpha.10` ; aucun commit, paquet,
tag ou publication n'est ajouté ou remplacé pour cette tranche locale.

## Doublons de surcharges — développement après alpha.10

Cette tranche locale du 2 octobre 2026 ajoute le diagnostic bilingue **118**,
`SurchargeDeclareePlusieursFois` / `OverloadDeclaredMoreThanOnce`. Il désigne
la seconde déclaration de la première paire identique d'un groupe de même nom.
Un test ajouté avant la correction reproduit l'écart : avec deux fonctions
`F()` inutilisées, l'ancienne image terminait la validation puis signalait
la capacité de sortie absente, alors que le bootstrap refusait le doublon à
la position de la seconde fonction.

L'identité d'une surcharge comprend le nom source complet et les types
canoniques ordonnés des paramètres, récepteur implicite des méthodes compris.
Le retour, les noms de paramètres, la visibilité, `virtuel` et `remplacer` ne
permettent pas de déclarer deux fois cette signature. Les qualifications,
indirections, références et signatures de callbacks restent distinctes ; les
alias de types désignant le même type ne créent pas une nouvelle surcharge.
Les espaces de noms et classes déclarantes distincts ne sont pas confondus.

Le contrôle inclut les fonctions libres, méthodes, constructeurs, destructeurs
et opérateurs libres ou membres. Comme dans le bootstrap actuel, une déclaration
`externe` suivie d'une définition de même signature dans le même programme
analysé est également refusée : cette tranche ne fusionne pas les prototypes
et définitions. Le nom canonique
des constructeurs et destructeurs vient de leur classe et du nom synthétique
déjà utilisé pour les alias.

La validation intervient après la résolution des types, les contraintes des
signatures et les dispositions, avant les remplacements virtuels, globales et
corps de fonctions. Les tests contrôlent ces priorités dans le périmètre retenu.
Le parcours des paires à l'intérieur d'un groupe suit celui du bootstrap ;
un corpus à quatre déclarations vérifie que la première paire examinée peut
désigner la quatrième déclaration, même si la troisième répète une autre
signature. La priorité entre plusieurs groupes invalides indépendants n'est
pas généralisée : le bootstrap les parcourt dans une table non ordonnée,
tandis que le frontend conserve un parcours lexical déterministe.

La matrice ajoute **68 refus différentiels français/anglais**, **40 corpus
valides** de surcharges distinctes et **dix refus d'émission** vérifiant les
tampons sentinelles et les positions des erreurs, pour un total de **845 refus**
comparés par code, ligne et colonne, avec AST public intact. Quatre corpus
d'émission valides supplémentaires comparent données et relocalisations avec
des surcharges libres ou membres : les callbacks ciblent des fonctions
distinctes qui appellent ces surcharges. Le bootstrap refuse l'adresse d'un
groupe surchargé même dans l'initialisation d'un callback typé ; les tests ne
contournent pas cette limite en modifiant le compilateur de référence.

Validation locale avec les commandes de la section héritage : CTest
**5/5 Windows**, **6/6 GNU/Linux**, solution et validation **MSBuild natives
sans CMake**, conformité **20/20** sur chaque construction. Les trois images
`Frontend.GsE` sont identiques : **376 031 octets**, **75 exports**, **deux
imports**, SHA-256
`3537f51f3d22851168552ee4f7bc464c6c4cd9f536859c65fd7581e14ad46635`.

Cette tranche n'ajoute pas le contrôle des collisions entre symboles de liaison
calculés, les indices publics de slots virtuels, un backend auto-hébergé ou
des sorties PE/ELF. Les dispositions publiques, formats 1.0 et ABI 1 sont
inchangés ; les codes 0 à 117 sont conservés. `VERSION` reste à
`0.27.0-alpha.10`. Aucun commit, paquet, tag ou publication n'est ajouté ou
remplacé ; les preuves et archives de l'alpha.10 publiée restent intactes.

## Signatures non liées et collisions classe/espace — développement après alpha.10

Cette tranche locale du 3 octobre 2026 corrige la comparaison des surcharges
entre une méthode et une fonction libre de même nom source complet. Une classe
et un espace peuvent porter le même nom dans le bootstrap actuel. La méthode
`C::F()` possède alors la signature non liée `C::F(C&)` et peut entrer en
collision avec une fonction libre `F(C&)` déclarée dans `espace C`.

Le test de régression ajouté avant la correction confirme le refus du bootstrap
avec le diagnostic de surcharge répétée, à la position de la seconde fonction.
L'ancienne image terminait sa validation et signalait seulement une capacité
de sortie absente. Le frontend comparait le récepteur séparément des paramètres
explicites, ce qui excluait à tort les paires méthode/fonction libre.

Le nouveau lecteur privé de paramètres traite le récepteur implicite comme le
premier paramètre canonique de la signature non liée, puis lit les paramètres
explicites dans leur ordre. Le contrôle de doublons compare ces séquences
complètes, sans modifier l'AST ni ajouter un nœud public de récepteur. Le
diagnostic **118** est réutilisé ; les codes existants ne sont pas renumérotés.

La couverture comprend les deux ordres de déclaration, espaces imbriqués,
noms qualifiés, chaînes d'alias de classe, références, pointeurs qualifiés,
callbacks, signatures à la limite d'arité et opérateurs libres ou membres.
Retour et visibilité ne distinguent pas les surcharges. Les références
constantes ou volatiles, pointeurs, types de callbacks, ordres et nombres de
paramètres différents continuent de distinguer les signatures. Les priorités
testées conservent les contrôles d'alias, types et héritage avant les doublons,
puis les remplacements virtuels, globales et corps après eux.

Cette comparaison ne change pas les clés virtuelles : elles continuent
d'exclure le récepteur implicite et d'inclure le retour. Le test à plusieurs
paires d'un même groupe conserve la position choisie par le bootstrap.
L'ordre entre plusieurs groupes invalides indépendants reste hors du
périmètre généralisé, comme dans la tranche précédente.

La matrice ajoute **44 refus différentiels français/anglais**, **28 corpus
valides** et **huit refus d'émission** avec tampons sentinelles intacts. Le
total atteint **897 refus**, comparés par code, ligne et colonne avec AST
préservé. Quatre corpus supplémentaires d'émission valides contrôlent les
données, métadonnées et relocalisations en présence de groupes mixtes dont
les signatures restent distinctes ; leurs callbacks ciblent une fonction
unique, pas le groupe surchargé.

Validation locale avec les commandes de la section héritage : CTest
**5/5 Windows**, **6/6 GNU/Linux**, solution et validation **MSBuild natives
sans CMake**, conformité **20/20** sur chaque construction. Les trois images
`Frontend.GsE` sont identiques : **376 287 octets**, **75 exports**, **deux
imports**, SHA-256
`5e721407a2babded502041e1dcb1ad971c07a897f684da75741a1ebc304011cc`.

Cette tranche valide les collisions de signatures sources, pas les collisions
entre noms de liaison calculés par `SuffixeSurcharge` dans le bootstrap. Elle
ne généralise pas non plus la sélection des appels dans tous les groupes
mixtes, les indices publics de slots virtuels ou un backend auto-hébergé.
Formats 1.0, ABI 1 et dispositions publiques restent inchangés. `VERSION`
demeure à `0.27.0-alpha.10` ; aucun commit, paquet, tag ou publication n'est
ajouté ou remplacé. Les preuves et archives de l'alpha.10 publiée restent
intactes ; `.Glib` / `.GdLib` reste une migration prévue pour 0.28.0.

## Collisions de symboles de liaison — développement après alpha.10

Cette tranche locale du 3 octobre 2026 distingue les surcharges répétées
(diagnostic 118) des surcharges de types distincts dont les noms de liaison
calculés sont identiques. Le nouveau diagnostic bilingue **119**,
`CollisionSymboleFonction` / `FunctionSymbolCollision`, refuse ces collisions
avant les remplacements virtuels, les globales et les corps, après les types,
dispositions, signatures et doublons. Les codes 0 à 118 sont conservés.

La régression a été reproduite avant la correction avec deux types ASCII
distincts, `TypeCollision_Bf_8190k3Dbe13aCfcn` et
`TypeCollisioncAadlNBpc0Aabp0aAaba`. Les surcharges `F(TypeA*)` et `F(TypeB*)`,
où `TypeA` et `TypeB` représentent ces noms complets, donnent toutes deux le
symbole **`F$949BBCA84D1140F8`** dans le bootstrap. L'ancienne image acceptait
l'analyse avant de signaler l'absence de capacité de sortie ; la nouvelle
refuse la seconde fonction avec le code 119 à la position attendue.

Le calcul privé reproduit l'affichage canonique des paramètres de
`TypeGs::Afficher()` et l'empreinte de `SuffixeSurcharge` : types primitifs
normalisés en français indépendamment de la langue source, noms complets des
types après résolution des alias, qualifications, pointeurs, références et
callbacks récursifs. Chaque paramètre est suivi de `;`, le récepteur implicite
`Classe&` précède les paramètres explicites et le retour n'entre pas dans
l'identité de la fonction. Le seed du bootstrap est conservé exactement :
`1469598103934665603`, avec multiplicateur `1099511628211` ; ce seed diffère
de celui du hachage des noms sémantiques et n'est pas remplacé par celui-ci.

Les contextes privés d'espaces de noms sont reconstruits à partir des jetons
du lexeur, l'AST compact n'ayant pas de nœuds publics d'espaces. Les graphies
qualifiées et imbriquées, commentaires et espaces, alias et noms UTF-8 restent
canoniques. Cette préparation n'est effectuée qu'en présence d'un groupe de
surcharges ; l'empreinte de chaque fonction est ensuite calculée une seule
fois par analyse et mise en cache dans l'arène privée. Aucun champ de symbole,
nœud d'AST ou nom de liaison public n'est ajouté.

Le contrôle compare les fonctions de même nom source complet. Un suffixe
identique dans deux fonctions de noms différents ou d'espaces distincts ne
constitue pas une collision ; le séparateur synthétique `$` n'est pas autorisé
dans les identifiants sources de cette grammaire. Les fonctions sont examinées
dans l'ordre source des déclarations ultérieures, comme lors de leur insertion
dans l'index de liaison du bootstrap. Une régression à deux paires de collisions
vérifie cette priorité, distincte de la recherche des doublons de signatures.
Elle ne généralise pas l'ordre de tous les diagnostics entre groupes invalides
indépendants.

Huit paires de types réellement en collision couvrent les empreintes avec
récepteur implicite, callbacks, qualifications, espaces qualifiés ou imbriqués
et UTF-8. Les tests C++ recalculent indépendamment les empreintes et vérifient
que les noms et identités sémantiques restent distincts. Les fixtures sont
stockées dans les tests : la recherche locale des collisions, conservée dans
un dossier de construction ignoré, n'ajoute aucune dépendance Python ou SymPy
à la compilation ou à l'exécution des tests distribués. Le bootstrap de
référence et son AST ne sont pas modifiés pour provoquer les refus.

La matrice ajoute **60 refus différentiels français/anglais**, **24 corpus
valides**, **huit refus d'émission** avec tampons sentinelles intacts et **quatre
corpus d'émission valides**. Le total atteint **965 refus**, comparés par code,
ligne et colonne avec AST préservé. Les alias, déclarations anticipées,
constructeurs, opérateurs et priorités des validations antérieures sont
couverts ; les callbacks émis ciblent toujours une fonction unique, sans
revendiquer la sélection d'une adresse dans un groupe surchargé.

Validation locale avec les commandes de la section héritage : CTest
**5/5 Windows**, **6/6 GNU/Linux**, solution et validation **MSBuild natives
sans CMake**, conformité **20/20** sur chaque construction. Les trois images
`Frontend.GsE` sont identiques : **388 319 octets**, **75 exports**, **deux
imports**, SHA-256
`c6f78ab8cdb79ff700673b22d14b5a0c38edf1d9f0578668edba2230c5af9493`.

Ce contrôle n'ajoute ni émission publique de noms de liaison auto-hébergés,
ni écrivain d'objets, ni indices publics de slots virtuels, ni backend
auto-hébergé, ni sorties PE/ELF. Formats 1.0, ABI 1 et dispositions publiques
restent inchangés. `VERSION` demeure à `0.27.0-alpha.10` ; aucun commit, paquet,
tag ou publication n'est ajouté ou remplacé. Les preuves et archives de
l'alpha.10 publiée restent intactes ; `.Glib` / `.GdLib` reste une migration
prévue pour 0.28.0.

## Appels de groupes mixtes — développement après alpha.10

Cette tranche locale du 3 octobre 2026 aligne la sélection des appels sur le
groupe complet de fonctions de même nom source qualifié. Le bootstrap permet
qu'une classe et un espace portent le même nom : une méthode `C::Lire` et une
fonction libre déclarée dans `espace C` appartiennent alors au même groupe de
surcharges. Leur simple déclaration ne suffit pas à prouver que les appels
choisissent la bonne fonction.

Le test ajouté avant la correction utilise une méthode `Lire()` et une fonction
libre `Lire(constante C&)`, puis appelle `C::Lire(objet)` avec un `C&` mutable.
Le bootstrap refuse l'appel ambigu à **1:173** ; l'ancienne image acceptait
l'analyse avant de signaler la capacité de sortie absente. Le frontend
excluait les méthodes de la sélection des fonctions libres, ou retenait une
méthode d'alias avant d'examiner les autres candidates.

La résolution des noms qualifiés et le comptage des surcharges utilisent
désormais le même groupe canonique, classe déclarante comprise. Une méthode
est comparée avec son récepteur `Classe&` en première position, une fonction
libre avec ses seuls paramètres déclarés. Les règles de score restent celles
du bootstrap : égalité de type, liaisons de références, conversions d'héritage,
adaptation des constantes entières et ambiguïté en cas de meilleur score
partagé. La visibilité n'est vérifiée qu'après sélection : une meilleure
méthode privée provoque le refus, et n'est pas remplacée par une candidate
publique moins adaptée. Le diagnostic d'ambiguïté précède ce contrôle d'accès.

Pour `objet.Lire(...)` et `objet->Lire(...)`, le groupe est recherché dans le
type statique, puis ses bases sans fusionner un groupe masqué. Les fonctions
libres de même nom complet sont incluses, même si le groupe ne contient pas de
méthode. Le récepteur synthétique est évalué avant les arguments explicites ;
la flèche utilise le type pointé et ses qualifications. Les tranches privées
de paramètres permettent de retirer le premier paramètre déclaré d'une
fonction libre lorsque celui-ci reçoit l'objet. Aucun nœud de déréférencement
ou de récepteur n'est ajouté à l'AST de l'appelant.

Les résolutions conservent les indicateurs de membre, de groupe surchargé et
de groupe hérité ; `Methode` n'est présent que si la cible effectivement choisie
est une méthode, pour les appels qualifiés comme pour les appels par membre.
Le retour et la cible canonique proviennent de cette déclaration sélectionnée,
pas de la première fonction rencontrée. Les adresses de groupes surchargés et
les alias de fonctions surchargées restent refusés ; aucun mécanisme nouveau
de sélection contextuelle de callback n'est revendiqué.

La matrice ajoute **66 corpus valides français/anglais**, **46 refus
différentiels**, **huit refus d'émission** avec tampons sentinelles intacts et
**quatre corpus d'émission valides**. Le total atteint **1 019 refus**, comparés
par code, ligne et colonne avec AST préservé. Les corpus valides vérifient
également la position de la déclaration retenue dans le bootstrap, le hachage
de son retour et les drapeaux de résolution. Ordres de déclaration, arités,
qualifications, références, constantes, callbacks, espaces qualifiés, alias
de types, héritage, masquage, ambiguïtés et visibilité sont couverts. Les
relocalisations émises visent des fonctions d'entrée uniques qui contiennent
des appels mixtes, pas les groupes surchargés eux-mêmes.

Validation locale avec les commandes de la section héritage : CTest
**5/5 Windows**, **6/6 GNU/Linux**, solution et validation **MSBuild natives
sans CMake**, conformité **20/20** sur chaque construction. Les trois images
`Frontend.GsE` sont identiques : **392 399 octets**, **75 exports**, **deux
imports**, SHA-256
`9ac550f425373dc954a27c9b1607320f34dbd0ba5b941e3478356c307121c3ff`.

Le bootstrap de référence n'est pas modifié. Les combinaisons non présentes
dans cette matrice, notamment les groupes d'opérateurs mixtes et les priorités
entre groupes invalides indépendants, restent à compléter. Cette tranche ne
génère pas de code machine auto-hébergé, de slots virtuels publics ou de sorties
PE/ELF. AST public, diagnostics 0–119, formats 1.0 et ABI 1 sont inchangés.
`VERSION` reste à `0.27.0-alpha.10` ; aucun commit, paquet, tag ou publication
n'est ajouté ou remplacé. Les preuves et archives publiées restent intactes ;
`.Glib` / `.GdLib` reste une migration prévue pour 0.28.0.

## Opérateurs mixtes et priorité des groupes invalides — développement après alpha.10

Cette tranche locale du 3 octobre 2026 poursuit celle des appels mixtes.
Elle compare les opérateurs unaires et binaires dans tout le groupe canonique
de même nom complet, méthodes et fonctions libres comprises. Un groupe peut
être défini dans la classe, dans l'espace homonyme, ou dans les deux ; la
recherche dans les bases s'arrête au premier groupe trouvé, sans fusionner un
groupe masqué ni tenter un autre espace si ses candidates sont incompatibles.
À défaut de groupe du type gauche, la recherche libre conserve la priorité du
nom global puis de l'espace courant du bootstrap. Un objet à droite peut
utiliser une fonction libre ; les opérateurs intrinsèques `&` et `*` ne sont
pas remplacés par ce mécanisme.

La sélection commune compare tous les opérandes. Pour une méthode, le premier
est lié au récepteur implicite `Classe&`, avec valeur gauche, constance et
conversion d'héritage ; pour une fonction libre, il est comparé au premier
paramètre déclaré. Les autres paramètres conservent les adaptations, références
et scores déjà utilisés pour les appels. Une égalité de meilleur score est
ambiguë, même si une méthode candidate est privée. L'accès à la méthode n'est
contrôlé qu'après sélection ; une fonction libre choisie n'est pas soumise à
l'accès d'une méthode non retenue. `Methode` ne décrit que la cible réelle,
`Operateur` décrit l'expression résolue, et le retour vient de la déclaration
effectivement sélectionnée. L'AST de l'appelant reste intact.

Le test introduit avant la correction oppose `C::operator+(entier32)` à une
fonction libre `C::operator+(constante C&, entier32)` avec objet mutable.
Le bootstrap signale l'ambiguïté à **1:227** ; l'ancienne image retenait la
méthode et atteignait le diagnostic de capacité de sortie absente au lieu de
refuser l'opérateur. Les corpus couvrent aussi l'ordre inversé des déclarations,
les groupes uniquement libres du type, les espaces qualifiés, alias, opérateurs
unaires, objets constants ou temporaires, références et adaptations de
littéraux, héritage, masquage et visibilité.

### Priorité définie pour les groupes indépendants

La table de recherche du bootstrap reste une `std::unordered_map`, mais elle
ne détermine plus l'ordre de validation des groupes de surcharges. Une liste
privée conserve leur ordre de **première déclaration dans le programme**.
Ce changement du bootstrap est intentionnel : il fixe la règle du produit sur
MSVC et GNU, sans changer les types, conversions, scores, noms de liaison ou
candidates de référence utilisés dans les comparaisons.

Les règles vérifiées sont :

1. Les doublons sont examinés groupe par groupe suivant leur première
   déclaration ; dans chaque groupe, les paires gardent l'ordre existant.
   Pour `F(a), G(a), G(b), F(b)` où les types sont identiques, le doublon de
   `F` est signalé sur la **quatrième ligne**, bien que celui de `G` soit
   déjà visible sur la troisième. Inverser les noms initiaux inverse le
   groupe prioritaire, pas cette règle.
2. Le contrôle des doublons précède celui des collisions de symboles de
   liaison, puis les diagnostics des corps. Une vraie collision calculée
   de `F` ne masque donc pas un doublon ultérieur de `G`.
3. Les collisions de liaison suivent les fonctions dans l'ordre source et
   désignent la première déclaration qui entre en collision avec une précédente.
4. La résolution auto-hébergée traite les déclarations dans l'ordre source
   de chaque catégorie : champs/énumérateurs, globales, fonctions. Le parcours
   interne de chaque déclaration reste inchangé, pour résoudre les arguments
   avant la cible surchargée d'un appel. Entre deux corps invalides indépendants,
   le premier corps est désormais prioritaire. Avant la correction, un test
   signalait l'ambiguïté de la seconde fonction à **6:43**, au lieu de celle de
   la première fonction à **5:49** dans le bootstrap.

Cette matrice ne généralise pas toutes les priorités de diagnostics. En
particulier, plusieurs erreurs dans un même corps, les instructions voisines,
les sous-expressions et les interactions entre passes non testées restent à
compléter. Les sections de jalons précédentes conservent leurs limites
historiques ; leur mention de la table non ordonnée décrit l'état avant cette
tranche.

### Preuves locales et portée

La matrice ajoute **38 corpus d'opérateurs valides français/anglais**, **26
refus d'opérateurs**, **28 refus de priorité** et **12 refus d'émission**
avec tampons sentinelles intacts, soit **66 nouveaux refus** et un total de
**1 085 refus** comparés par code, ligne et colonne avec AST préservé.
Les positions attendues de priorité sont aussi assertées indépendamment dans
les tests du bootstrap. Quatre corpus d'émission valides comparent données,
alignements et relocalisations vers des fonctions d'entrée uniques contenant
des opérateurs mixtes, sans prendre l'adresse d'un groupe surchargé.

Validation avec les commandes de la section héritage : **CTest 5/5 Windows**,
**6/6 GNU/Linux**, solution `GsPlusPlus.slnx` et validation **MSBuild natives
sans CMake**, conformité **20/20** sur chaque construction. Les trois images
`Frontend.GsE` sont identiques : **392 687 octets**, **trois segments**, **huit
sections**, **75 exports**, **deux imports**, SHA-256
`22641f4e2bda2438ae734ba91abc132b28cece87f3028b5dc1f7d6c8d8e1a9d6`.
Le vérificateur confirme une image GsE 1.0 valide. Le contrat public, les
diagnostics 0–119, les formats 1.0 et l'ABI 1 sont inchangés. Aucun backend
auto-hébergé, slot virtuel public ou sortie PE/ELF n'est ajouté.

Les trois en-têtes `VersionProduit.hpp` auparavant restés à alpha.9 ont aussi
été régénérés depuis `VERSION` :

- `Construction/MSBuild/x64/Debug/Generated/GsPP/VersionProduit.hpp`, avec
  `VisualStudio/Prepare-Version.ps1 -OutputRoot Construction/MSBuild/x64/Debug` ;
- `D:/Systeme-Sanctuaire-SE/Construction/CMake/VisualStudio/Release/GsPlusPlus/Generated/GsPP/VersionProduit.hpp`,
  avec `cmake --preset windows-release` depuis ce consommateur local ;
- `D:/Systeme-Sanctuaire-SE/Construction/CMake/Ninja/Release/GsPlusPlus/Generated/GsPP/VersionProduit.hpp`,
  avec `cmake --preset linux-release` sous WSL depuis ce consommateur local.

Ils contiennent tous `0.27.0-alpha.10`. Cette régénération ne reconstruit pas
leurs exécutables Debug ou système et ne réécrit aucun paquet publié.
`VERSION` reste à alpha.10 ; aucun commit, paquet, tag ou publication n'est
créé ou remplacé. `.Glib` / `.GdLib` reste prévu pour 0.28.0.

## Priorité des instructions d'un même corps — développement après alpha.10

La tranche précédente a été enregistrée dans le commit signé `6686761`, sans
push, tag ou release. Cette nouvelle tranche locale du 3 octobre 2026 aligne
la résolution des instructions d'un même corps sur l'ordre du bootstrap.
L'ordre des déclarations indépendantes était déjà corrigé, mais le parcours
inversé de toute une déclaration faisait encore passer les diagnostics de la
dernière instruction avant ceux de la première.

Le test ajouté avant la correction appelle un opérateur privé à la troisième
ligne, puis une méthode unaire avec récepteur constant à la quatrième.
Le bootstrap refuse le premier opérateur à **3:7**, diagnostic 25 ; l'ancienne
image signalait le second à **4:1**, diagnostic 21. Le nouveau parcours traite
les instructions successives, les blocs imbriqués et les expressions de
conditions dans l'ordre source, puis les branches et corps de boucles dans
l'ordre syntaxique du bootstrap.

Les sous-arbres d'instructions retour, d'expressions autonomes et de variables
locales restent chacun une unité. La résolution interne conserve le parcours
déjà validé : enfants avant parent et arguments résolus avant la cible
surchargée d'un appel. Les conditions sont traitées avant leurs blocs ; les
branches `si` puis `sinon` sont toutes deux analysées, même si la condition est
un littéral. Le corps d'un `tantque(faux)` et les instructions après un retour
ne sont pas ignorés pendant l'analyse sémantique, conformément au bootstrap.
Les expressions d'initialiseurs explicites de constructeurs gardent aussi leur
parcours interne. Cette tranche ne change pas le bootstrap et n'ajoute pas
de nouvelles règles de validation des types de conditions ou de construction.

Le parcours utilise les indices parents de l'AST préordonné pour borner chaque
unité. Il ne modifie pas les nœuds, n'ajoute pas de pile ni d'allocation, et
ne résout pas deux fois une expression appartenant à une unité déjà traitée.
Champs, énumérateurs et globales conservent le parcours de la tranche
précédente ; ce changement s'applique aux déclarations de fonctions et membres
exécutables.

La matrice ajoute **40 refus différentiels français/anglais**, **16 corpus
valides**, **12 refus d'émission** sans écriture partielle et **quatre corpus
d'émission valides**. Elle couvre instructions successives, initialiseurs
locaux contenant un appel ambigu, blocs, conditions, branches, boucles,
retours, affectations incompatibles, code inatteignable et variantes d'ordre.
Le total atteint **1 137 refus**, comparés par code, ligne et colonne avec
AST intact. Les lignes prioritaires attendues sont aussi assertées directement
dans le bootstrap ; les corpus valides contrôlent les interrogations de
capacité puis les sorties complètes. Les tests d'émission conservent les
tampons sentinelles en cas de refus et comparent données et relocalisations
vers des fonctions d'entrée uniques contenant plusieurs instructions.

Validation avec les commandes de la section héritage : **CTest Windows 5/5**,
**GNU/Linux 6/6**, solution `GsPlusPlus.slnx` et validation **MSBuild natives
sans CMake**, conformité **20/20** sur les trois constructions. Les images
`Frontend.GsE` sont identiques : **393 775 octets**, **trois segments**, **huit
sections**, **75 exports**, **deux imports**, SHA-256
`642f09521faf823e5b7d0264504c947a1b148c4f9caa74e840419ec7a5e79f01`.
Le vérificateur confirme une image GsE 1.0 valide. Contrats publics, diagnostics
0–119, formats 1.0 et ABI 1 sont inchangés ; aucun backend auto-hébergé,
slot virtuel public ou sortie PE/ELF n'est ajouté.

Les erreurs concurrentes dans une **même expression** et les priorités entre
**passes différentes**, notamment les contrôles différés d'initialiseurs ou de
conversions constantes, restent à étendre. Les preuves et limites de la
section précédente décrivent le jalon avant ce nouveau parcours ; elles ne
sont pas réécrites rétroactivement.

Cette nouvelle tranche reste locale non commitée et non publiée. `VERSION`
reste à `0.27.0-alpha.10` ; aucun nouveau paquet, tag ou release n'est créé.
Les archives publiées restent intactes et `.Glib` / `.GdLib` reste une migration
prévue pour 0.28.0.

## Priorité des erreurs dans une expression — développement après alpha.10

Cette tranche locale du 4 octobre 2026 complète le parcours des instructions.
Le bootstrap C++ reste l'oracle et n'est pas modifié. Avant correction, le
premier test ajouté associait un opérateur privé dans l'opérande gauche et
un opérateur unaire appelé avec récepteur constant dans l'opérande droit.
Le bootstrap retenait le diagnostic **25 à 1:326** ; le parcours inversé de
l'image auto-hébergée signalait **21 à 1:353**. Ce refus a été reproduit avant
la modification du parcours.

### Ordre couvert et limite des appels

Le parcours résout les opérandes de gauche à droite, puis contrôle leur
expression parente. La matrice couvre notamment :

- opérateurs binaires imbriqués, opérateurs unaires et appels ambigus utilisés
  comme opérandes, avec inversion des deux erreurs pour contrôler la priorité ;
- opérandes logiques `&&` et `||` : les deux côtés sont analysés sémantiquement,
  sans les confondre avec le court-circuit d'exécution ;
- objet avant indice dans une indexation, dont un membre absent ou une référence
  introuvable face à un indice lui-même invalide ;
- cible d'affectation avant sa valeur : valeur non modifiable, constante ou
  tableau refusé avant l'analyse de la source ; les erreurs de l'expression
  cible sont elles-mêmes traitées avant la compatibilité de la valeur ;
- type cible de conversion avant sa source : cible non scalaire ou type nommé
  introuvable refusé avant l'erreur présente dans l'opérande converti ;
- éléments d'agrégat et références d'initialiseur d'énumération dans l'ordre
  source ; mêmes opérandes intégrés à un initialiseur local, retour ou condition.

Les appels restent des unités atomiques du nouveau parcours : **leur sous-arbre
conserve le parcours interne précédent**. Cela préserve les cas déjà validés
de groupes mixtes, d'alias non liés et d'arguments disponibles avant sélection
de la cible. Cette tranche ne revendique pas encore la priorité générale
entre cible, candidats, arité et erreurs concurrentes des arguments d'un même
appel, ni celle des expressions imbriquées à l'intérieur de ses arguments.

### Parcours et contrats préservés

`ResoudreExpressionsSemantiques` utilise les indices parents de l'AST
préordonné pour visiter les enfants dans l'ordre puis revenir aux contrôles
parents. Le parcours est itératif, borné à la tranche fournie, sans récursion,
pile auxiliaire, allocation supplémentaire ou modification des nœuds publics.
Un sous-arbre d'appel est traité une seule fois par son parcours conservé.
Les contrôles de cible d'affectation et de type cible de conversion sont
séparés des contrôles nécessitant la valeur source déjà résolue ; les codes
de diagnostics existants ne changent pas.

### Preuves locales du 4 octobre 2026

La matrice ajoute **50 refus différentiels français/anglais**, **18 corpus
sémantiques valides**, **12 refus d'émission** avec tampons sentinelles intacts
et **quatre corpus d'émission valides**. Les corpus valides incluent une chaîne
de **128 opérations binaires**, des appels directs, membres et callbacks dans
des opérandes, des indexations, agrégats et conversions imbriquées. Ils
contrôlent l'AST d'entrée, les interrogations de capacité et les sorties
complètes. Les tests d'émission comparent octets de données globales,
alignements et relocalisations ; ils ne prouvent pas un backend auto-hébergé
de code machine pour les corps de fonctions.

Le total atteint **1 199 refus**, comparés au bootstrap par code, ligne et
colonne avec AST intact. Validation avec les commandes de la section héritage :
**CTest Windows 5/5**, **GNU/Linux 6/6**, solution `GsPlusPlus.slnx` et validation
**MSBuild natives sans CMake**, conformité **20/20** sur les trois constructions.
Les images `Frontend.GsE` sont identiques : **396 543 octets**, **trois segments**,
**huit sections**, **75 exports**, **deux imports**, SHA-256
`e99dbeea736e3619b3434d2c49cf038e4110647b8952aba8faebb7f2b29074dd`.
Le vérificateur confirme une image GsE 1.0 valide. Diagnostics 0–119, contrats
publics, formats 1.0 et ABI 1 sont inchangés ; aucun slot virtuel public,
backend auto-hébergé ou sortie PE/ELF n'est ajouté.

Les sections précédentes conservent leurs preuves historiques, notamment
les **1 137 refus** avant cette tranche. Le commit signé `6686761` a depuis été
poussé ; les nouvelles tranches de priorité des instructions et expressions
restent locales non commitées et non publiées. `VERSION` reste à
`0.27.0-alpha.10`, sans nouveau push, paquet, tag ou release. Les archives
publiées restent intactes ; `.Glib` / `.GdLib` demeure prévu pour 0.28.0.

## Priorité des cibles et arguments d'appel — développement après alpha.10

Cette tranche locale du 4 octobre 2026 remplace l'unité atomique d'appel de
la tranche précédente par le parcours itératif de ses enfants. Le bootstrap
C++ reste inchangé. Avec l'ancienne image de **396 543 octets**, le nouveau
corpus signale **21 à 1:557** dans le second argument, au lieu du diagnostic
**25 à 1:532** dans le premier argument retenu par le bootstrap. Cet écart a
été reproduit en exécutant les nouveaux tests sur la copie de l'image avant
correction ; aucun ancien paquet publié n'a été remplacé pour cette preuve.

### Ordre couvert

- Les expressions des arguments sont analysées dans l'ordre source, avec les
  contrôles de conversion, affectation et opérandes de la tranche précédente,
  ainsi que les appels imbriqués.
- Une cible indirecte est résolue et contrôlée avant les arguments : référence
  absente, valeur non appelable, membre absent ou privé, champ callback et
  variable locale masquant une fonction sont couverts.
- L'arité d'un appel indirect est contrôlée avant les expressions d'arguments.
  Pour les groupes directs ou membres, l'absence de signature d'arité et de
  récepteur recevables est aussi signalée avant ces expressions.
- La sélection d'une surcharge reste différée jusqu'aux arguments résolus.
  Cela préserve les scores, groupes mixtes, alias non liés, ambiguïtés et
  contrôles de visibilité après sélection déjà couverts.
- Pour les signatures de callbacks décrites par les types privés, le préfixe
  d'arguments déjà parcouru est contrôlé avant le suivant : un premier argument
  incompatible est ainsi prioritaire sur l'expression invalide du second.

Les indices parents de l'AST servent toujours au retour vers les expressions
englobantes, sans récursion, pile auxiliaire, allocation supplémentaire ou
modification des nœuds publics. La référence de groupe différée est résolue
une seule fois à la fin de l'appel. Une structure privée regroupe la signature
indirecte et la limite de son préfixe : aucun helper ni corpus ne dépasse la
limite actuelle de **quatre paramètres** du langage.

### Preuves locales

La matrice ajoute **60 refus différentiels français/anglais**, **16 corpus
sémantiques valides**, **12 refus d'émission** sans écriture partielle et
**quatre corpus d'émission valides**. Les cas valides couvrent appels directs,
surchargés, par point ou flèche, champs callbacks, indexations de callbacks
et une chaîne de quatre appels imbriqués. Les interrogations de capacité et
les sorties complètes sont comparées avec AST d'entrée intact. Les tests
d'émission comparent octets, alignements et relocalisations des globales ;
ils ne constituent pas un backend auto-hébergé de code machine des fonctions.

Le total atteint **1 271 refus**, comparés au bootstrap par code, ligne et
colonne. Validation avec les commandes de la section héritage : **CTest
Windows 5/5**, **GNU/Linux 6/6**, solution `GsPlusPlus.slnx` et validation
**MSBuild natives sans CMake**, conformité **20/20** sur les trois constructions.
Les images `Frontend.GsE` sont identiques : **401 855 octets**, **trois segments**,
**huit sections**, **75 exports**, **deux imports**, SHA-256
`f7050af610b10063c404b83abf40d370e0fbd63331e4ef6467ee0d12e2a3083c`.
Le vérificateur confirme une image GsE 1.0 valide. Diagnostics 0–119, contrats
publics, formats 1.0 et ABI 1 sont inchangés.

### Limites et publication

Cette tranche ne généralise pas toutes les priorités de sélection. Le
bootstrap peut **abandonner un candidat après un argument incompatible sans
analyser les suivants**, puis essayer un autre candidat. Le parcours actuel
diffère encore l'évaluation complète de ces groupes jusqu'aux arguments
résolus ; cette priorité par candidat reste à migrer. Les initialiseurs
contextuels des arguments, leurs agrégats, les combinaisons d'adresses de
fonctions et les interactions entre passes restent également à étendre.

Les sections précédentes conservent leurs preuves historiques, notamment
les **1 199 refus** et le traitement atomique des appels avant cette tranche.
Les nouvelles tranches restent locales non commitées et non publiées, après
le commit signé déjà poussé `6686761`. `VERSION` reste à `0.27.0-alpha.10`,
sans nouveau push, paquet, tag ou release. Les archives publiées sont intactes ;
`.Glib` / `.GdLib` reste prévu pour 0.28.0. Aucun frontend 0.27 complet,
backend auto-hébergé ou sortie PE/ELF n'est annoncé.

## Abandon ordonné des candidats d'appel — développement après alpha.10

Cette tranche locale du 4 octobre 2026 étend la priorité des appels aux
arguments non contextuels déjà typés. Avant correction, le premier nouveau
corpus signalait **25 à 1:559** dans le second argument, alors que le bootstrap
C++ inchangé retenait **21 à 1:540** sur la cible : le premier argument était
déjà incompatible avec l'unique candidat. La régression est conservée dans
`TesterAbandonsCandidatsAppelsSemantiques`, sans assouplir la comparaison.

### Préfixes et ordre des candidats

- `TrancheArgumentsFonctionSemantique` porte une limite privée d'arguments
  résolus. L'arité de l'appel complet est toujours vérifiée ; seul le préfixe
  déjà parcouru est comparé aux paramètres avant l'argument suivant.
- `EvaluerPrefixeGroupeAppelSemantique` réutilise les scores de conversion et
  de références, le récepteur synthétique des appels par point ou flèche,
  ainsi que le récepteur explicite des alias de méthodes non liées.
- `ValiderPrefixeGroupeAppelSemantique` essaie les candidats dans l'ordre de
  déclaration et s'arrête au premier préfixe recevable. Si tous les candidats
  sont incompatibles, le diagnostic 21 précède l'analyse de l'argument suivant.
- Ne pas évaluer prématurément les candidats suivants préserve la priorité des
  calculs constants : une première surcharge `entier32` ne force pas le calcul
  de `1 / 0` avant l'argument suivant, alors qu'une première surcharge `naturel8`
  peut devoir l'évaluer pour adapter sa valeur. Les deux ordres de déclaration
  et la sélection finale avec un argument suivant valide sont testés.
- `ValiderPrefixeArgumentsAppelSemantique` réunit ce contrôle des groupes et
  celui des signatures de callbacks. Le choix final de la meilleure surcharge,
  ses scores, les ambiguïtés, la visibilité et les métadonnées restent confiés
  à la sélection complète existante, une fois les arguments résolus.

La passe de normalisation des types privés ne signale plus prématurément les
erreurs de types des **expressions de conversion**. Ces erreurs sont contrôlées
par `ValiderCibleConversionSemantique` lorsque la conversion est visitée,
avant son opérande. Un type cible inconnu dans un argument non encore parcouru
ne remplace donc pas le refus d'un préfixe antérieur : **21** reste prioritaire.
Si le préfixe est compatible, cette même conversion produit toujours **99**.
Les types de déclarations restent validés dans leur passe existante ; les
signatures de callbacks invalides et les conversions isolées sont aussi testées.

Les nœuds publics restent intacts. Aucun diagnostic public n'est ajouté ; les
helpers respectent la limite de quatre paramètres. Le contrôle des préfixes
n'ajoute ni récursion, ni pile de parcours, ni allocation auxiliaire.

### Preuves locales

La tranche ajoute **56 refus sémantiques français/anglais**, **16 corpus
sémantiques valides**, **12 refus d'émission** sans écriture partielle et
**quatre corpus d'émission valides** : **68 nouveaux refus**, pour un total de
**1 339**, comparés par code, ligne et colonne au bootstrap. La matrice couvre
les appels libres et surchargés, les références temporaires ou constantes,
les méthodes par point ou flèche, les groupes mixtes, les alias non liés,
les abandons après un deuxième argument et les appels imbriqués. Les cas
valides vérifient aussi l'AST intact, les requêtes de capacité, les octets de
données globales et leurs relocalisations ; ils ne constituent pas une
génération auto-hébergée de code machine des fonctions.

Validation avec les commandes de la section héritage : **CTest Windows 5/5**,
**GNU/Linux 6/6**, solution `GsPlusPlus.slnx` et validation **MSBuild natives
sans CMake**, conformité **20/20** sur les trois constructions. Le test Linux
de reproductibilité a d'abord comparé une bibliothèque périmée aux sources
modifiées pendant la validation. La modification locale de mise en forme de
`ConteneursDynamiques.GsPP` a été conservée ; après reconstruction de la
bibliothèque, la suite complète réussit avec les mêmes entrées actuelles.

Les trois images `Frontend.GsE` sont identiques : **405 135 octets**, **trois
segments**, **huit sections**, **75 exports**, **deux imports**, SHA-256
`0da4292bd3cad5843cb0ec599bf7d7caf1ebbcd43e2e6c3eb60497a07b706f91`.
Le vérificateur confirme une image GsE 1.0 valide. Contrats publics, diagnostics
0–119, formats 1.0 et ABI 1 sont inchangés.

### Limites et publication

Cette preuve porte sur les arguments non contextuels de la matrice. Les
initialiseurs d'arguments nécessitant un type attendu, leurs agrégats, les
combinaisons d'adresses de fonctions et d'autres interactions entre passes,
notamment les validations différées de calculs constants, restent à étendre.
Elle ne démontre pas l'équivalence exhaustive de toutes les sélections.

Les sections précédentes conservent leurs preuves historiques, notamment les
**1 271 refus** avant cette tranche. Les tranches restent locales non commitées
et non publiées, après le commit signé déjà poussé `6686761`. `VERSION` reste à
`0.27.0-alpha.10`, sans nouveau push, paquet, tag ou release ; les archives
publiées sont intactes. Le passage au jalon 0.28 n'est pas encore validé.
La migration `.Glib` / `.GdLib`, le backend auto-hébergé et les sorties PE/ELF
restent des travaux prévus, non des capacités livrées par cette tranche.

## Arguments agrégés contextuels — développement après alpha.10

Cette tranche locale du 4 octobre 2026 traite les initialiseurs `{…}` utilisés
comme arguments d'appels. Le bootstrap C++ est inchangé. Avant correction,
le premier corpus `Scalaire({1, 2}, 0)` passait l'analyse auto-hébergée et
retournait **4**, demande de capacité d'un résultat valide ; le bootstrap le
refusait avec le diagnostic correspondant à **44**, trop d'éléments scalaires.
La régression reste comparée par code, ligne et colonne.

### Type attendu et ordre de validation

- Le parcours principal saute le contenu d'un agrégat directement rattaché
  à un appel. Son préfixe reste évalué comme un argument agrégé lors de la
  sélection : score 2 pour un paramètre par valeur, refus d'une référence.
- Pour une fonction libre, méthode ou groupe mixte, la meilleure surcharge
  et sa visibilité sont déterminées avant le contenu des agrégats. Une
  ambiguïté, une méthode privée ou un argument non agrégé incompatible reste
  prioritaire sur un nom introuvable dans un agrégat différé.
- `ValiderArgumentsAgregesFonctionSemantique` applique ensuite les paramètres
  canoniques de la déclaration retenue, en tenant compte du récepteur
  synthétique par point/flèche, du premier paramètre des fonctions libres
  homonymes et du récepteur explicite des méthodes non liées.
- Pour un callback ou une adresse explicite de fonction, le type attendu est
  déjà disponible : l'argument agrégé est initialisé avant l'argument suivant.
  Les champs callbacks et tableaux de callbacks sont aussi couverts.
- `ValiderArgumentAgregeTypeSemantique` réutilise le validateur d'initialiseurs.
  Un indicateur **privé** dans `DestinationInitialiseurSemantique.Reserve`
  active la résolution contextuelle des feuilles. La forme et la capacité
  sont contrôlées avant les enfants ; chaque feuille est ensuite résolue et
  comparée à son type attendu, dans l'ordre source.
- L'indicateur se propage aux champs et dimensions imbriqués. Les conversions
  numériques nécessaires contrôlent aussi les bornes et les calculs constants ;
  références vers agrégats temporaires et initialisation agrégée de classes
  restent refusées conformément au bootstrap.

Le hachage privé d'un agrégat validé est mémorisé dans la table existante des
conversions implicites. Les vérifications successives du préfixe puis de
l'appel complet ne résolvent pas deux fois ses feuilles. Aucune nouvelle
allocation de parcours n'est ajoutée. Le validateur récursif existant sert aux
agrégats imbriqués ; ses feuilles peuvent appeler le parcours d'expressions,
notamment pour les appels imbriqués. Les contrats et nœuds publics restent
inchangés, avec la limite de quatre paramètres par fonction.

### Preuves locales

La matrice ajoute **66 refus sémantiques français/anglais**, **36 corpus
sémantiques valides**, **12 refus d'émission** sans écriture partielle et
**quatre corpus d'émission valides** : **78 nouveaux refus**, pour un total de
**1 417**. Elle couvre agrégats scalaires vides et imbriqués, structures,
unions, tableaux de champs, bornes numériques, fonctions libres, alias de types
qualifiés, méthodes par point/flèche ou non liées, groupes mixtes, callbacks
stockés en variables, champs ou tableaux, adresses de fonctions et appels
contextuels imbriqués. Les tests contrôlent aussi l'AST intact et les requêtes
de capacité. Les cas d'émission valides comparent les octets de globales et
leurs relocalisations au bootstrap ; ils ne valident pas un backend
auto-hébergé de génération de code machine des fonctions.

Commandes de validation de la section héritage : **CTest Windows 5/5**,
**GNU/Linux 6/6**, solution `GsPlusPlus.slnx` et validation **MSBuild natives
sans CMake**, conformité **20/20** sur les trois constructions. Les images
`Frontend.GsE` sont identiques : **409 519 octets**, **trois segments**, **huit
sections**, **75 exports**, **deux imports**, SHA-256
`4789f843541cefd64d1c57e5c1ddab37e2952e946259689c655ab212d41de2ab`.
Le vérificateur confirme une image GsE 1.0 valide. Bootstrap C++, diagnostics
0–119, formats 1.0 et ABI 1 restent inchangés. La modification locale de
`ConteneursDynamiques.GsPP` est conservée sans édition de cette tranche.

### Limites et publication

La preuve couvre les arguments des appels testés, pas tous les initialiseurs
contextuels du langage : constructions et opérateurs restent à étendre dans
leurs matrices respectives. Les qualifications, combinaisons d'adresses de
fonctions, validations différées de conversions constantes et priorités des
initialiseurs de déclarations restent à consolider entre passes. Cette tranche
ne démontre pas une équivalence sémantique exhaustive.

Les sections précédentes conservent leurs preuves historiques, dont les
**1 339 refus** avant cette tranche. `VERSION` reste à `0.27.0-alpha.10` ; les
tranches sont locales non commitées et non publiées, sans nouveau push, tag,
paquet ou release. Les archives publiées restent intactes. Le passage à
`0.28.0-alpha.1` n'est pas validé ; `.Glib` / `.GdLib`, le backend auto-hébergé
et les sorties PE/ELF restent des travaux prévus.

## Priorité des conversions constantes — développement après alpha.10

**VALIDÉ dans le périmètre différentiel testé — 4 octobre 2026.**

### Régression et contrôle au point de visite

`convertir<naturel8>(256); objet + 7;`, avec un opérateur privé, signalait
**25** à l’opérateur alors que le bootstrap signalait **98** à la conversion.
Le contrôle numérique avait lieu dans une passe parcourant tous les nœuds
après la résolution des expressions ; une erreur ultérieure pouvait le masquer.
Le nouveau test reproduit l’écart, code, ligne et colonne compris.

`ValiderConversionSemantique` contrôle désormais la valeur constante dès la
visite de la conversion, après le type cible, la source et les signatures.
Il réutilise `EvaluerConstanteNumeriqueSemantique` ; la passe tardive
`ValiderConversionsConstantesSemantiques` et son appel sont supprimés.
Une conversion non constante ou d’adresse ne déclenche pas de calcul numérique.
Les bornes signées/non signées, la conversion booléenne et le court-circuit
des calculs constants conservent leurs règles existantes.

Les vérifications préalables d’arité et les abandons de candidats continuent
à empêcher la visite d’arguments qui ne doivent pas encore être analysés.
Pour un agrégat d’appel direct, sélection et visibilité restent prioritaires
sur son contenu différé. Pour un callback, son initialisation contextuelle
précède l’argument suivant. Les cas inversés sont aussi comparés au bootstrap.

### Preuves et limites

La matrice ajoute **64 refus sémantiques français/anglais**, **24 corpus
sémantiques valides**, **12 refus d’émission** sans écriture partielle et
**quatre corpus d’émission valides** : **76 nouveaux refus**, soit **1 493**.
Elle couvre expressions successives, opérandes, conversions imbriquées,
arguments directs/indirects, adresses de fonctions, agrégats contextuels,
déclarations locales et globales, champs par défaut, énumérations et valeurs
de retour. Code, ligne, colonne, AST intact et capacités sont vérifiés.
Les octets de globales valides sont comparés au bootstrap ; aucun backend
auto-hébergé de fonctions n’est revendiqué.

L’exemple [TypesParFichier](../Exemples/TypesParFichier/Application.GsPj)
vérifie aussi la séparation d’une structure et d’une énumération en interfaces
`.HGsPP`, explicitement listées dans un projet XML. Il retourne **42** et est
exécuté dans le test d’intégration Linux. Cette orchestration relève du
bootstrap ; elle n’est pas une nouvelle capacité du frontend auto-hébergé.

Validation complète : **CTest Windows 5/5**, **GNU/Linux 6/6**, construction de
`GsPlusPlus.slnx` et de `VisualStudio/Validation.vcxproj` par **MSBuild natif**,
conformité **20/20** sur les trois constructions. Les commandes sont celles
de la section de validation héritage ; elles ont été rejouées pour cette tranche.
Les trois images `Frontend.GsE` sont identiques : **408 975 octets**, **trois
segments**, **huit sections**, **75 exports**, **deux imports**, SHA-256
`c98606d6dc7134ff4275381e9d2c92e4444eed256f51f48b7fea217262dc2fd8`.
Le vérificateur les accepte comme GsE 1.0. Les trois en-têtes générés
`VersionProduit.hpp` indiquent `0.27.0-alpha.10`. Les diagnostics publics 0–119,
formats 1.0 et ABI 1 sont inchangés ; la modification locale de
`ConteneursDynamiques.GsPP` est conservée sans édition.

Les autres priorités d’initialiseurs de déclarations, les contextes des
constructions et opérateurs, les qualifications et les combinaisons encore
absentes de la matrice restent à consolider. Il ne s’agit pas d’une preuve
exhaustive d’équivalence. Les sections précédentes conservent leurs résultats
historiques. `VERSION` reste à `0.27.0-alpha.10` ; ces tranches de développement
ne font pas partie des paquets alpha.10 publiés. Aucun nouveau tag, paquet ou
release n’est créé ; le jalon 0.28 n’est pas ouvert.

## Priorité des initialiseurs locaux — développement après alpha.10

**VALIDÉ dans le périmètre différentiel testé — 4 octobre 2026.**

### Régression et résolution contextuelle

`entier32 x = {Absente, 2};` signalait **18** au nom absent, alors que le
bootstrap refuse d’abord les deux éléments d’un agrégat scalaire avec **44**
à la racine de l’initialiseur. Le nouveau corpus reproduit cette divergence
avant la correction, code, ligne et colonne compris.

`ResoudreInstructionsSemantiques` traite désormais les variables locales
initialisées avec `=` par `ValiderInitialiseurLocalContextuelSemantique`,
avant l’instruction suivante. Le type de destination est déjà canonique.
Le contrôle refuse les initialisations de classes par `=` à la déclaration,
et les tableaux initialisés autrement que par un agrégat. Le validateur
existant vérifie ensuite les capacités et formes imbriquées avant les feuilles,
puis résout et type chaque feuille dans l’ordre source avec son type attendu.
Le même indicateur **privé** de résolution contextuelle que pour les arguments
d’appels est réutilisé ; aucune nouvelle allocation de parcours n’est ajoutée.
Sans description de destination, le parcours d’expressions existant reste utilisé.

Références vers agrégats temporaires, liaisons incompatibles, signatures de
callbacks, classes et tableaux de classes, structures, unions et tableaux
imbriqués sont couverts. Les adaptations numériques contrôlent aussi les
bornes et les erreurs de calcul avant la déclaration ou instruction suivante.
Les cas inverses restent dans la matrice : une erreur d’expression antérieure
garde sa priorité. Une expression numérique déjà du type attendu ne devient
pas automatiquement un calcul constant obligatoire ; ce comportement reste
celui du bootstrap, notamment pour `entier32 x = 1 / 0;` local.

### Preuves locales

La matrice ajoute **68 refus sémantiques français/anglais**, **24 corpus
sémantiques valides**, **12 refus d’émission** sans écriture partielle et
**quatre corpus d’émission valides** : **80 nouveaux refus**, soit **1 573**.
Code, ligne, colonne, AST intact et capacités sont contrôlés. Les corpus
d’émission valides comparent les octets et relocalisations des globales au
bootstrap ; ils ne valident pas un backend auto-hébergé de fonctions.

Validation complète : **CTest Windows 5/5**, **GNU/Linux 6/6**, solution
`GsPlusPlus.slnx` et `VisualStudio/Validation.vcxproj` par **MSBuild natif**,
conformité **20/20** sur les trois constructions. Les commandes de la section
de validation héritage ont été rejouées. Les trois `Frontend.GsE` sont identiques :
**410 191 octets**, **trois segments**, **huit sections**, **75 exports**, **deux
imports**, SHA-256
`b3ee802d086c9b3eb15c47d32fb62f6128f62a37428a381cbb3a3240f031308f`.
Le vérificateur les accepte comme GsE 1.0. Bootstrap C++, contrats publics,
liste des diagnostics 0–119, formats 1.0 et ABI 1 restent inchangés.

### Limites et version

Cette tranche concerne les initialiseurs **locaux avec `=`**. Les priorités
des globales, champs par défaut, listes de constructeurs et autres contextes
des constructions/opérateurs restent à consolider ; leurs passes ne sont pas
fusionnées ici. Les qualifications et combinaisons absentes de la matrice,
la conformité du frontend complet et les benchmarks restent en cours.

Les mentions de contrôles locaux encore différés dans les sections précédentes
sont historiques. `VERSION` reste à `0.27.0-alpha.10` ; les tranches de
développement ne font pas partie des archives alpha.10 publiées, et aucun
nouveau tag, paquet ou release n’est créé. Le jalon 0.28 n’est pas ouvert.

## Priorité des initialiseurs globaux et contrôles structurels des champs par défaut — développement après alpha.10

**VALIDÉ dans le périmètre différentiel testé — 4 octobre 2026.**

### Régressions et ordre des passes

`entier32 X = {Absente, 2};` signalait **18** au nom absent, alors que le
bootstrap refuse d'abord les deux valeurs de l'agrégat scalaire avec **44** à
sa racine. La divergence a été reproduite avant correction, code, ligne et
colonne compris. Une seconde régression a été reproduite avec une globale
invalide et un champ par défaut utilisant `Absente` dans une classe possédant
un constructeur défini : la visite prématurée du champ masquait le refus de
globale attendu par le bootstrap.

Chaque déclaration globale est désormais contrôlée entièrement dans l'ordre
source par `ValiderInitialiseurGlobalContextuelSemantique`, avant la globale
suivante et avant les corps des fonctions. Le contrôle de déclaration conserve
les refus de classes globales, références, `vide`, externes publiques et
constantes sans initialisation. La destination canonique sert ensuite au
contrôle de forme et de capacité, puis à la résolution et au typage de chaque
feuille. Le validateur existant et son indicateur privé de résolution
contextuelle sont réutilisés, sans nouvelle allocation de parcours.

La passe constante vient **après le typage de toutes les feuilles de cette
globale**, et non après chaque feuille. Ainsi, dans
`Point P = {Lire(), vrai};`, le type incompatible du second élément **45**
reste prioritaire sur le caractère non constant du premier **84**. Même
distinction pour `{1 / 0, vrai}` contre `{1 / 0, 0}`, et pour un pointeur
de données non pris en charge suivi d'un élément mal typé. Après typage,
la passe constante conserve les restrictions d'agrégats et de pointeurs,
les relocalisations de fonctions, divisions par zéro et contrôles de plage.

Les contrôles **structurels** des champs par défaut **38/39** sont effectués
après les conflits de signatures et de liaison, mais avant l'analyse des
globales et des fonctions. Les expressions de champs ne sont visitées
qu'après les globales. Les énumérateurs gardent leur phase antérieure et
chaque catégorie conserve l'ordre source. Ce changement ne réalise pas
encore la visite des champs dans chaque contexte de constructeur.

### Preuves locales

La matrice ajoute **104 refus sémantiques français/anglais**, **16 corpus
sémantiques valides**, **20 refus d'émission** sans écriture partielle et
**huit corpus d'émission valides** : **124 nouveaux refus**, soit **1 697**.
Code, ligne, colonne et AST intact sont comparés au bootstrap. Les corpus
valides vérifient aussi les capacités des sorties ; l'émission compare octets,
alignements et relocalisations de fonctions au bootstrap. Elle ne valide pas
un backend auto-hébergé de fonctions.

La couverture comprend scalaires, structures, unions et tableaux imbriqués,
alias de types nommés et espaces de noms, callbacks, courts-circuits,
erreurs concurrentes entre globales et fonctions, puis interactions avec les
contrôles structurels et expressions des champs par défaut. Les cas inverses
et les deux passes internes de chaque globale sont conservés dans le corpus.

Validation complète : **CTest Windows 5/5**, **GNU/Linux 6/6**, solution
`GsPlusPlus.slnx` et `VisualStudio/Validation.vcxproj` par **MSBuild natif**,
conformité **20/20** sur les trois constructions. Commandes rejouées dans
`D:\Langage-GsPlusPlus` :

```powershell
cmake --build --preset windows-release --target espace_travail --parallel 6
ctest --preset windows-release --output-on-failure
wsl -d Ubuntu -- bash -lc 'cd /mnt/d/Langage-GsPlusPlus && cmake --build --preset linux-release --target espace_travail --parallel 4 && ctest --preset linux-release --output-on-failure'
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' GsPlusPlus.slnx /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' VisualStudio/Validation.vcxproj /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
```

Les trois `Frontend.GsE` sont identiques : **410 431 octets**, **trois segments**,
**huit sections**, **75 exports**, **deux imports**, SHA-256
`75590937276d0eeb5d788b57184632745ff5d370f176259f88c01e8bb792f6bc`.
Le vérificateur les accepte comme GsE 1.0. Les trois `VersionProduit.hpp`
générés indiquent `0.27.0-alpha.10`. Le bootstrap C++, les contrats publics,
diagnostics 0–119, formats 1.0 et ABI 1 sont inchangés.
`ConteneursDynamiques.GsPP` n'a pas été modifié dans cette tranche.

### Limites et version

Les champs par défaut doivent encore être évalués dans le contexte de chaque
constructeur qui les utilise : paramètres visibles, initialisation explicite
remplaçant la valeur par défaut, délégation, prototypes d'interfaces et ordre
par rapport aux listes d'initialisation et corps. La visite globale actuelle
des expressions de champs n'est pas une preuve de conformité de ces contextes.
Les autres interactions sémantiques et qualifications absentes du corpus
restent également à consolider ; la matrice n'est pas une preuve exhaustive
d'équivalence du frontend.

Les mentions de priorités globales différées dans les sections précédentes
sont historiques ; les combinaisons non testées restent ouvertes. `VERSION`
reste à `0.27.0-alpha.10`, et ces tranches de développement ne font pas partie
des archives alpha.10 publiées. Aucun nouveau tag, paquet ou release n'est
créé ; le jalon 0.28 n'est pas ouvert.

## Inclusions et utilisations d'espaces — tranche locale du 4 octobre 2026

### Contrat et périmètre

Le bootstrap `gsppc` accepte désormais les inclusions textuelles
`#inclure "chemin"` / `#include "path"`. Les chemins UTF-8 sont relatifs au
fichier incluant ; les jetons sont insérés à l'endroit de la directive et
conservent leur fichier, ligne et colonne. `#pragma once` est propre à
chaque unité ; sans protection, une nouvelle inclusion produit effectivement
ses déclarations. Les cycles sont refusés et la profondeur est bornée.

`utilisant espace N;` / `using namespace N;`, sans `#`, est pris en charge
au niveau global ou d'un espace de noms : types et alias, valeurs
d'énumération, globales, fonctions et groupes de surcharges, noms relatifs,
imports transitifs et ambiguïtés. La directive agit après sa déclaration et
ne constitue ni une inclusion ni une dépendance de liaison.
L'exemple `Exemples/Directives/Application.GsPj` fournit les types uniquement
par inclusion, sans entrées `<Interface>` dans le XML ; sa source française
et `Principal.en.GsPP` retournent toutes deux 42.

**Le frontend auto-hébergé n'est pas encore équivalent pour cet ajout.** Son
classificateur et son lexeur reconnaissent les jetons 76–78 sans renuméroter
les anciens. L'AST de déclarations et la résolution des utilisations restent
à porter, et l'expansion des fichiers est réalisée par le bootstrap hôte.
Les API auto-hébergées en mémoire ne lisent pas les fichiers inclus.

Les utilisations dans les blocs, `using N::Nom`, `using Type = ...`, macros,
conditions `#if` / `#ifndef`, chemins `<...>` et options `-I` ne sont pas
implémentés ici. Le
[contrat actuel](SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md#inclusion-textuelle-et-utilisation-despaces-de-noms)
ne revendique pas un préprocesseur C++ complet.

### Régressions et preuve actuelle

- 24 corpus bootstrap valides français/anglais et 26 refus bilingues, dont
  portée, masquage, surcharges importées et noms ambigus ;
- six cas d'inclusion valides, plus un contrôle de casse des chemins sous
  Windows, et 15 refus vérifiant le fichier d'origine, la ligne et la colonne ;
- trois corpus lexicaux différentiels valides, incluant espaces après `#`,
  commentaires, BOM, CRLF et noms UTF-8, plus six refus lexicaux ;
- 85 classifications auto-hébergées ; les 1 697 refus sémantiques et d'émission
  de la tranche précédente continuent de passer, avec AST de l'appelant intact ;
- CTest Windows 5/5, GNU/Linux 6/6 ; construction `GsPlusPlus.slnx` et
  `VisualStudio/Validation.vcxproj` réussies ; conformité 20/20 dans les
  constructions CMake/MSVC, CMake/GNU et MSBuild natif ;
- exemples français et anglais vérifiés puis exécutés avec retour 42 sur
  les trois constructions, plus construction et exécution du projet XML.

Commandes principales exécutées :

```powershell
cmake --build --preset windows-release --target espace_travail --parallel 6
ctest --preset windows-release --output-on-failure
wsl -d Ubuntu -- bash -lc 'cd /mnt/d/Langage-GsPlusPlus && cmake --build --preset linux-release --target espace_travail --parallel 4 && ctest --preset linux-release --output-on-failure'
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' GsPlusPlus.slnx /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' VisualStudio/Validation.vcxproj /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
```

Les trois `Frontend.GsE` sont identiques : **412 255 octets**, **trois segments**,
**huit sections**, **75 exports**, **deux imports**, SHA-256
`d62edca321ad48f5c99e36c7df353e5a6d35aa54621428552f7c56ec88189fdc`.
Les dispositions publiques auto-hébergées, diagnostics sémantiques 0–119,
formats 1.0 et ABI 1 restent inchangés. Les ajouts d'AST et d'origine des
jetons dans le bootstrap C++ ne modifient pas le format public de l'AST
auto-hébergé.
`ConteneursDynamiques.GsPP` a été conservé sans modification.

`VERSION` reste à `0.27.0-alpha.10`. Cette preuve remplace la taille et le
hachage de la tranche précédente pour les constructions locales actuelles,
pas pour les archives alpha.10 publiées. Aucun commit, push, tag, paquet ou
release n'est créé pour cet ajout.

## Utilisations d'espaces auto-hébergées — tranche locale du 5 octobre 2026

**VALIDÉ dans le périmètre différentiel testé.** Le portage annoncé dans la
tranche précédente est réalisé : `utilisant espace N;` / `using namespace N;`
est reconnu par l'analyseur de déclarations et la résolution sémantique
auto-hébergés au niveau global ou d'un espace de noms. Les types, alias,
énumérations, globales, fonctions et opérateurs libres peuvent être importés.
Les groupes de surcharges sont fusionnés sans perdre les candidats de chaque
espace, y compris lorsqu'un candidat est un alias de fonction.

La recherche respecte les espaces imbriqués, la position de la directive,
le masquage et les noms qualifiés. Les imports transitifs utilisent une file
bornée avec détection des visites pour terminer même en présence de cycles.
Les types importés sont également résolus dans les bases de classes,
signatures, conversions et déclarations globales. Les initialisations globales
et leurs relocalisations sont comparées au bootstrap ; en cas de refus, les
tampons de sortie restent intacts.

### Contrats et limites

- ajout du genre `UtilisationEspace` / `UsingNamespace` **36** ;
- ajout syntaxique `EspaceAttendu` / `ExpectedNamespace` **30** ;
- ajouts sémantiques `EspaceUtiliseIntrouvable` / `UsedNamespaceNotFound`
  **120** et `NomImporteAmbigu` / `AmbiguousImportedName` **121** ;
- valeurs précédentes conservées, nœud AST toujours de **64 octets** et
  AST de l'appelant non modifié par l'analyse ; les consommateurs doivent
  néanmoins connaître ces nouvelles valeurs publiques ;
- lecture et expansion de `#inclure` / `#include` toujours effectuées par
  le bootstrap hôte ; l'API mémoire du frontend ne lit pas elle-même les fichiers ;
- utilisations en bloc, `using N::Nom`, `using Type = ...`, macros,
  chemins `<...>` et répertoires `-I` toujours hors de cette tranche.

### Régressions et constructions vérifiées

- **24 corpus valides**, chacun français/anglais, soit 48 analyses ;
  contrôle des cibles choisies pour les surcharges importées et leurs alias ;
- **14 refus sémantiques bilingues**, soit 28 cas vérifiant code, ligne,
  colonne et AST intact ; **trois refus syntaxiques bilingues**, soit six cas ;
- **quatre corpus d'émission valides bilingues**, soit huit cas, et
  **trois refus d'émission bilingues**, soit six cas protégeant les sorties ;
- total différentiel sémantique et émission : **1 731 refus**, au lieu de
  1 697 ; les six refus syntaxiques ne sont pas inclus dans ce compteur ;
- **85 classifications**, tests antérieurs conservés ;
- CMake/MSVC : `espace_travail` puis CTest **5/5** ;
- CMake/GNU sous Ubuntu/WSL : `espace_travail` puis CTest **6/6** ;
- Visual Studio 2026 natif : `GsPlusPlus.slnx` et
  `VisualStudio/Validation.vcxproj` réussis ;
- conformité **20/20** dans chacune des trois constructions.

Les commandes de construction sont celles de la tranche précédente et ont
été relancées pour ce portage. Les trois images `Frontend.GsE` sont identiques :
**437 535 octets**, trois segments, huit sections, 75 exports, deux imports ;
SHA-256 `d0ac0d2d1b0602c6f706090fe4371e382e19178f9707714674c86d425bde0b27`.
L'image est vérifiée comme **GsE 1.0**. Cette preuve remplace les tailles et
hachages antérieurs pour l'état local actuel, pas pour les paquets publiés.

`VERSION` et les trois en-têtes générés annoncent `0.27.0-alpha.10` ; formats
1.0 et ABI 1 conservés. `ConteneursDynamiques.GsPP` n'a pas été modifié.
Aucun commit, push, tag, paquet ou release n'est créé dans cette tranche.
Cela ne clôt ni l'ensemble de la sémantique 0.27 ni le backend auto-hébergé.

## Champs par défaut dans chaque constructeur — tranche locale du 5 octobre 2026

**VALIDÉ dans le périmètre testé.** Le frontend analyse maintenant les champs
par défaut dans le contexte du constructeur qui les utilise. Ses paramètres
sont visibles, même si le champ les précède dans le fichier ; les variables
déclarées dans le corps ne le sont pas. Les références à `soi` / `this` et
`parent` / `super`, callbacks, conversions, structures et tableaux utilisent
ce contexte. Les cibles d'expressions et conversions privées sont réinitialisées
entre deux visites du même champ, sans modifier l'AST public de l'appelant.

Les listes explicites sont traitées avant les champs implicites, puis vient
le corps du constructeur. Une initialisation explicite, y compris par un
alias de champ, empêche la visite de sa valeur par défaut. Un constructeur
délégué ne réévalue pas les champs ; son constructeur cible les traite.
Les erreurs de champ absent, de doublon, d'ordre et d'arité sont vérifiées
avant les expressions correspondantes. La forme et les dimensions des
agrégats sont contrôlées avant leurs feuilles.

### Correction du bootstrap et génération

La nouvelle matrice a découvert un défaut du bootstrap C++ : les constructeurs
partageaient un arbre d'expression de champ par défaut, que l'analyse du premier
modifiait en choisissant une surcharge ou en transformant un appel membre.
Le second pouvait donc réutiliser une cible inadaptée. Chaque initialiseur
implicite possède maintenant sa copie de syntaxe, conservant les positions
source, et les choix sémantiques de chaque constructeur restent indépendants.
Le test unitaire vérifie ces arbres distincts et la syntaxe originale intacte.

Le [programme d'intégration français](../Tests/Integration/ChampsParDefaut/Principal.GsPP)
et sa [version anglaise](../Tests/Integration/ChampsParDefaut/Principal.en.GsPP)
construisent deux objets par des signatures différentes. Le champ par défaut
appelle respectivement les surcharges `entier32` et `entier64`, qui retournent
11 et 31 ; le programme doit retourner **42**. Cette exécution vérifie les
choix conservés jusque dans le code machine, au-delà de l'acceptation sémantique.

### Prototypes et périmètre

Un prototype de constructeur fourni dans l'AST avec le drapeau externe ne
déclenche pas l'analyse des champs ni la génération d'un plan de corps.
Une classe avec valeur de champ par défaut exige toujours un constructeur
défini : un prototype seul conduit au diagnostic **39**. Trois corpus bilingues
comparent ces cas au bootstrap à partir de son AST d'interface, avec positions
source et intégrité de l'AST contrôlées. Ces tests portent sur l'API sémantique ;
ils ne prouvent pas une nouvelle expansion autonome des interfaces dans le
parseur mémoire auto-hébergé.

Les constructions locales restent encore sélectionnées dans une passe ultérieure.
Les interactions de priorité impliquant cette passe, les plans de durée de vie
et les autres familles absentes de la matrice restent à consolider.

### Régressions

- **26 corpus valides bilingues**, soit 52 analyses ; contrôles des paramètres
  et des surcharges distincts pour deux constructeurs partageant un champ ;
- **28 refus sémantiques bilingues**, soit 56 diagnostics comparés au bootstrap ;
- trois corpus d'interface bilingues : quatre analyses valides et deux refus ;
- **quatre corpus d'émission valides bilingues**, soit huit comparaisons, et
  **quatre refus d'émission bilingues**, soit huit contrôles des sorties intactes ;
- total différentiel : **1 797 refus**, soit 66 de plus que la tranche précédente ;
- test unitaire bootstrap bilingue et programme d'intégration bilingue exécuté.

### Constructions vérifiées

Les commandes de construction et validation de la tranche précédente ont
été relancées pour cet état : CMake/MSVC et CTest **5/5**, CMake/GNU sous
Ubuntu/WSL et CTest **6/6**, solution `GsPlusPlus.slnx` et
`VisualStudio/Validation.vcxproj` natives réussies. La conformité est **20/20**
dans chaque construction. Les programmes français et anglais retournent 42
sous Windows CMake/MSVC, GNU/Linux et MSBuild natif ; les exécutions GNU font
partie du test d'intégration permanent.

Les trois `Frontend.GsE` sont identiques : **442 015 octets**, trois segments,
huit sections, 75 exports, deux imports ; SHA-256
`1dfe10bcd7af5017bc8f4bafb75ebe112608b943feb728429cdd163ad4df34a9`.
Chaque vérificateur accepte son image comme GsE 1.0. Ces données remplacent
les tailles et hachages précédents pour l'état local actuel.

`VERSION` et les trois `VersionProduit.hpp` restent à `0.27.0-alpha.10`.
Les dispositions publiques auto-hébergées et diagnostics 0–121 sont conservés,
ainsi que les formats 1.0 et ABI 1 ; le bootstrap C++ possède un nouveau
champ privé de propriété de l'expression par défaut dans son AST interne.
`ConteneursDynamiques.GsPP` est conservé sans modification. Aucun commit,
push, tag, paquet ou release n'a été créé pour cette tranche.

## Constructions locales et plans de durée de vie — tranche locale du 5 octobre 2026

**VALIDÉ dans le périmètre testé.** La sélection des constructeurs locaux et
la planification de construction/destruction ne sont plus reportées après
l'analyse de tous les corps. Chaque objet est contrôlé lors de sa déclaration,
avant l'instruction, branche ou fonction suivante. Les corps des constructeurs
conservent l'ordre source des fonctions ; leur propre plan est contrôlé après
leurs initialiseurs et avant leurs instructions. La vérification des cycles
de délégation reste une passe finale, comme dans le bootstrap.

L'arité est filtrée avant de visiter les arguments. Les préfixes incompatibles
éliminent les candidats avant l'argument suivant. La sélection et la visibilité
précèdent l'analyse typée des arguments agrégés ; une ambiguïté ou un accès
interdit ne déclenche donc pas leurs feuilles. Un constructeur local est
sélectionné une seule fois et le plan correspondant n'est pas dupliqué.

Les objets sans constructeur propre passent par la construction implicite
de leurs bases et champs avant le refus éventuel d'une construction explicite
scalaire. Les tableaux sans arguments sont construits par défaut, y compris
avec `Classe objets[2]();` et sans constructeur propre. Les tableaux avec
arguments exigent toujours un constructeur déclaré pour leur type élément.

La position du diagnostic récursif est désormais distincte de l'origine du
plan : une erreur de constructeur/destructeur dans un sous-objet pointe son
champ, alors que les étapes restent rattachées à la variable ou au constructeur
qui les consomme. Cette distinction est privée et ne change ni les structures
publiques ni la numérotation des diagnostics. Les plans de destruction restent
inverses aux constructions et les cas de tableaux multidimensionnels,
héritage et objets imbriqués existants restent vérifiés.

### Régressions

- **12 corpus valides bilingues**, soit 24 analyses : objets, tableaux simples
  et multidimensionnels, bases, champs objets, alias, références, agrégats et
  corps imbriqués ; contrôles de sélection unique et du nombre/ordre des étapes
  de construction/destruction sur les objets et tableaux à constructeur propre ;
- **36 refus sémantiques bilingues**, soit 72 comparaisons : priorité entre
  instructions, branches, boucles et fonctions, arité, abandon de candidats,
  ambiguïté, visibilité, agrégats et diagnostics récursifs de durée de vie ;
- **trois corpus d'émission valides bilingues**, soit six comparaisons, et
  **six refus d'émission bilingues**, soit 12 contrôles des sorties intactes ;
- total différentiel : **1 881 refus**, soit 84 de plus que la tranche précédente ;
  code, ligne, colonne, intégrité de l'AST et bornes de sortie sont contrôlés.

### Constructions vérifiées

Les commandes de construction et validation précédentes ont été relancées :
CMake/MSVC et CTest **5/5**, CMake/GNU sous Ubuntu/WSL et CTest **6/6**, solution
`GsPlusPlus.slnx` et `VisualStudio/Validation.vcxproj` natives réussies.
La conformité reste **20/20** dans chaque construction, y compris les
régressions existantes d'ordre de construction/destruction et de déterminisme.

Les trois `Frontend.GsE` sont identiques : **443 599 octets**, trois segments,
huit sections, 75 exports, deux imports ; SHA-256
`a644e8b644e594023be802b2a9f347abe1b5d43048d38675d051818a3810e888`.
Chaque vérificateur accepte son image comme GsE 1.0. Ces données remplacent
les tailles et hachages précédents pour l'état local actuel.

`VERSION` et les trois en-têtes Release générés restent à `0.27.0-alpha.10`.
Les diagnostics 0–121, dispositions publiques auto-hébergées, formats 1.0 et
ABI 1 sont conservés. `ConteneursDynamiques.GsPP` n'est pas modifié.
Aucun commit, push, tag, paquet ou release n'est créé pour cette tranche.

Ce bilan ne couvre pas toutes les interactions entre plans récursifs de
bases/champs et expressions de listes explicites ou valeurs par défaut.
Le plan de corps reste contrôlé après la résolution des initialiseurs ;
les priorités internes non représentées dans la matrice restent à consolider.
La construction explicite de types non-classes et l'alimentation autonome
par interfaces ne sont pas ajoutées par cette tranche. Le backend
auto-hébergé et l'ensemble du frontend 0.27 ne sont pas déclarés terminés.

## Priorités des bases, champs et initialiseurs — tranche locale du 5 octobre 2026

**VALIDÉ dans le périmètre testé.** Les accès et sélections récursifs d'une
base ou d'un champ objet sans constructeur propre sont contrôlés au moment
de leur initialisation, avant l'expression suivante. Une erreur dans la base
précède ainsi les listes de champs et valeurs par défaut. Les champs implicites
sont visités dans l'ordre de déclaration : une erreur de construction du
premier champ précède la valeur par défaut suivante, et l'ordre inverse
inverse également le diagnostic, comme dans le bootstrap.

Le contexte privé de plan possède un mode de validation seule. Il réutilise
les mêmes règles récursives de types et d'accès, mais n'écrit aucune étape
et n'augmente pas la capacité requise des résolutions. Après validation des
initialiseurs, le plan final reste produit dans l'ordre canonique : base,
table virtuelle, puis champs déclarés, même lorsqu'un champ explicite a été
analysé avant un champ implicite placé plus tôt. Le plan d'un champ explicite
réutilise sa cible sélectionnée, sans nouvelle sélection de surcharge.

Les arguments de base, de champ objet et de délégation utilisent désormais
le même parcours contextuel que les constructions locales : arité avant
les expressions, abandon de candidats avant l'argument suivant, puis
sélection et visibilité avant les feuilles des agrégats. Les contrôles
structurels de champ absent, doublon, ordre et arité restent prioritaires
avant leurs arguments. Les cycles de délégation restent vérifiés à la fin.

Une base sans constructeur propre accepte `parent()` / `super()` et produit
les étapes implicites de ses bases, champs et tables virtuelles. Le frontend
ne fabrique plus une étape de constructeur vers une cible inexistante.
Une telle base n'accepte toujours pas d'arguments : le diagnostic 27 précède
alors leurs expressions. Les positions récursives restent celles du champ
concerné et les étapes finales restent rattachées au constructeur consommateur.

### Régressions

- **12 corpus valides bilingues**, soit 24 analyses : bases implicites et
  explicites sans constructeur propre, tableaux de champs, agrégats typés,
  délégation, tables virtuelles, alias de champs, pointeurs et accès protégés ;
  contrôles du nombre d'étapes, de leurs décalages et des constructeurs retenus ;
- **32 refus sémantiques bilingues**, soit 64 comparaisons : bases avant
  initialiseurs, champs récursifs avant ou après valeurs par défaut, constantes
  non initialisées, arités, préfixes, ambiguïtés, visibilité et agrégats ;
- **quatre corpus d'émission valides bilingues**, soit huit comparaisons, et
  **six refus d'émission bilingues**, soit 12 contrôles des sorties intactes ;
- total différentiel : **1 957 refus**, soit 76 de plus que la tranche précédente ;
  codes, lignes, colonnes, intégrité de l'AST et capacités de sortie contrôlés.

### Constructions vérifiées

```powershell
cmake --build --preset windows-release --target espace_travail --parallel 6
ctest --preset windows-release --output-on-failure
wsl -d Ubuntu -- bash -lc 'cd /mnt/d/Langage-GsPlusPlus && cmake --build --preset linux-release --target espace_travail --parallel 4 && ctest --preset linux-release --output-on-failure'
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' GsPlusPlus.slnx /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' VisualStudio/Validation.vcxproj /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
```

Résultats : CMake/MSVC et CTest **5/5**, CMake/GNU sous Ubuntu/WSL et CTest
**6/6**, solution et validation MSBuild natives réussies. La conformité reste
**20/20** dans chaque construction. Les tests de style, projets natifs,
capacités, plans de durée de vie et intégration existants passent également.

Les trois `Frontend.GsE` sont identiques : **444 447 octets**, trois segments,
huit sections, 75 exports, deux imports ; SHA-256
`920deba02e72eef5694b8139a9ed3778fe624771f21c9a7d4a13f191366888d4`.
Les trois vérificateurs acceptent leur image comme GsE 1.0. Ces données
remplacent les tailles et hachages précédents pour l'état local actuel.

`VERSION` et les trois en-têtes Release générés restent à `0.27.0-alpha.10`.
Les diagnostics 0–121, dispositions publiques auto-hébergées, formats 1.0 et
ABI 1 sont conservés. `ConteneursDynamiques.GsPP` reste inchangé.
Aucun commit, push, tag, paquet ou release n'est créé pour cette tranche.

Ces tests ne ferment pas toutes les combinaisons sémantiques de 0.27. Les
contrôles de déclaration locale, constructions explicites de types non-classes,
qualifications et signatures non représentées dans la matrice restent à
consolider. L'alimentation autonome par interfaces et le backend auto-hébergé
ne sont pas ajoutés ; le passage à 0.28 n'est pas déclaré acquis.

## Travaux restant dans Gs++ 0.27

- compléter les combinaisons de conversions et qualifications encore
  absentes de la matrice différentielle ;
- compléter les autres familles sémantiques encore prises en charge par le
  bootstrap, notamment les contextes des constructions et opérateurs et les
  interactions de priorité entre passes non encore testées, dont les contrôles
  non couverts d'initialiseurs globaux et les interactions avec les constructions
  locales non couvertes, contrôles de déclaration et constructions explicites
  de types non-classes, ainsi que les contextes de bases/champs non représentés
  dans la matrice,
  et les combinaisons de conversions
  non encore couvertes ; le
  raccordement des données globales aux écrivains d’objets
  auto-hébergés appartient au jalon backend ;
- étendre la conformité seulement lorsque cette tranche forme un frontend
  cohérent ;
- reconstruire les benchmarks avant la version 0.27.0 finale ;

Les outils de la préversion actuelle annoncent `0.27.0-alpha.10` ;
la tranche historique alpha.9 conserve sa version et ses preuves.
Aucun statut
`VALIDÉ` ni `stable` n’est revendiqué pour Gs++ 0.27 dans son ensemble.
