# Frontend auto-hébergé Gs++ 0.27

**EN COURS — lexeur, AST syntaxique, indexation et première sélection typée
VALIDÉS — 26 août 2026.**

Gs++ 0.27 a pour objectif de migrer le frontend du compilateur depuis le
bootstrap C++ vers Gs++. Le lexeur constitue la première tranche achevée,
l’alpha.2 ajoute les fonctions libres et leurs paramètres, l’alpha.3 étend le
même AST compact aux déclarations de données, l’alpha.4 couvre les méthodes,
constructeurs, destructeurs et opérateurs de classes, puis l’alpha.5 construit
la hiérarchie des blocs et instructions. L’alpha.6 ajoute l’AST interne des
expressions avec les mêmes priorités et associativités que le bootstrap. Le
jalon alpha.7 ajoute l’indexation des symboles et la première résolution des
noms. La branche de développement suivante sélectionne les surcharges libres et
membres à partir des types déjà déterminables dans l’AST compact, contrôle la
visibilité et résout maintenant les constructeurs locaux ainsi que leurs
initialiseurs explicites.
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

Le test charge réellement `Lexeur.GsE`, résout ses imports avec l’ABI
`GsAbi:x64-ms-v1` et compare chaque résultat à `GsPP::Lexeur` :

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

Le test charge réellement `AnalyseurDeclarations.GsE` et compare chaque nœud
au programme produit par `GsPP::AnalyseurSyntaxique` :

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

Les constructions MSVC et GNU produisent un `AnalyseurDeclarations.GsE`
identique bit à bit. La validation détaillée et les empreintes sont consignées
dans
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

## Première sélection typée des surcharges — après alpha.7

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
`AnalyseurSemantique.GsE` et par les suites complètes MSVC et GNU. Les étapes
objet qui prolongent cette sélection sont décrites dans la section suivante.

## Visibilité, constructeurs locaux et opérateurs membres — après alpha.7

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

## Initialiseurs explicites de constructeurs — après alpha.7

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

La matrice locale du 26 août 2026 passe 4/4 sous Visual Studio 2026 et 5/5 sous
GNU/Linux, la conformité reste à 20/20 et les quatre scénarios de benchmark
smoke réussissent sur chaque chaîne. Les quatre images sont identiques bit à
bit entre les chaînes. Les images modifiées par cette tranche sont :

| Image | Taille | SHA-256 |
| --- | ---: | --- |
| `AnalyseurDeclarations.GsE` | 107 523 | `f25a2b118bb04c8c62a9dcca3d8c2a269705e203f2ab157ba1061af724ad91d5` |
| `AnalyseurSemantique.GsE` | 77 622 | `3aa3c0df41bc2e732c4b181e166523980ec20382d63c15cd622c2cdda6537041` |

Cette tranche ne couvre pas encore la validation complète de l’arité et du
type des valeurs de champs scalaires, la sélection des constructeurs de champs
objets ou de tableaux, les constructions de base implicites sans nœud source,
les destructeurs et plans de durée de vie, les opérateurs libres, ni le typage
récursif complet des appels et opérateurs imbriqués.

## Travaux restant dans Gs++ 0.27

- compléter la résolution et la comparaison des types de toutes les
  expressions ;
- compléter les initialiseurs de champs objets, les tableaux et les
  constructions implicites de bases ;
- migrer les opérateurs libres et le typage des opérateurs imbriqués ;
- compléter les conversions implicites, qualifications et liaisons de
  références ;
- migrer les destructeurs et les plans de durée de vie ;
- étendre la conformité seulement lorsque cette tranche forme un frontend
  cohérent ;
- reconstruire les benchmarks avant la version 0.27.0 finale ;

Les outils de la tranche publique annoncent `0.27.0-alpha.7`. Aucun statut
`VALIDÉ` ni `stable` n’est revendiqué pour Gs++ 0.27 dans son ensemble.
