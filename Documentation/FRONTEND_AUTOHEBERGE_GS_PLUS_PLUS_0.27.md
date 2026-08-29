# Frontend auto-hébergé Gs++ 0.27

**EN COURS — lexeur, AST syntaxique, indexation, sélection typée et contraintes
des expressions couvertes VALIDÉS — 29 août 2026.**

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
bit. La validation détaillée de la préversion et ses empreintes historiques
sont consignées dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.7.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.7.md).

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

La matrice complète, les empreintes reproductibles et les contrôles des
paquets extraits sont consignés dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.7.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.7.md).

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

## Travaux restant dans Gs++ 0.27

- migrer les opérateurs libres et valider exhaustivement les opérateurs
  intrinsèques ;
- compléter les conversions implicites, qualifications et liaisons de
  références ;
- compléter la résolution des déclarations globales et les autres familles
  sémantiques encore prises en charge par le bootstrap ;
- étendre la conformité seulement lorsque cette tranche forme un frontend
  cohérent ;
- reconstruire les benchmarks avant la version 0.27.0 finale ;

Les outils de la tranche publique annoncent `0.27.0-alpha.8`. Aucun statut
`VALIDÉ` ni `stable` n’est revendiqué pour Gs++ 0.27 dans son ensemble.
