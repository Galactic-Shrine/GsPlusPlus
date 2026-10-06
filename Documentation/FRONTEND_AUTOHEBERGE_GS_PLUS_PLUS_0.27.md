# Frontend auto-hébergé Gs++ 0.27

**EN COURS — lexeur, AST syntaxique, indexation, sélection typée, contraintes
des expressions couvertes, alias racines, déclarations d'héritage, remplacements
virtuels, doublons de surcharges, signatures non liées, collisions de symboles
de liaison, appels et opérateurs de groupes mixtes, priorités indépendantes,
entre instructions, dans les expressions, appels, abandons de candidats et
arguments agrégés contextuels, conversions constantes lors de leur visite et
initialiseurs locaux contextuels, champs par défaut par constructeur,
constructions locales, plans de durée de vie et priorités des bases/champs couverts,
déclarations locales contextuelles, recherche lexicale des espaces parents
et contexte des noms et opérateurs dans les méthodes, constructeurs et destructeurs
et types de conversions, callbacks et arguments agrégés des constructions,
diagnostics internes des champs par défaut, retours par référence des callbacks,
qualifications des champs/éléments adressés, références de pointeurs des callbacks,
distinction des cibles de tableaux, références de callbacks paramétrés,
protection de leur stockage constant, arguments référencés des callbacks imbriqués,
références de structures et emplacements de pointeurs dans ces callbacks,
qualifications et conversions des références de groupes mixtes dans les constructions,
opérateurs mixtes référencés, constructions et expressions imbriquées,
opérateurs des initialiseurs agrégés, affectations et retours contextuels,
analyse des interfaces préparées en mémoire, assemblage préparé et normalisation
des déclarations libres et membres, groupes mixtes normalisés, analyse d'une
unité développée avec modes mixtes et origines de jetons, raccordement de ces
origines à l'assemblage/normalisation et à la sémantique par unité, origines
des diagnostics et isolation des imports par unité
et émission des globales
VALIDÉS dans le périmètre testé — 6 octobre 2026.**

La génération machine C++ des noms locaux réutilisés dans des portées distinctes
est également validée dans la tranche décrite plus bas ; elle ne constitue pas
une migration du backend vers Gs++.

Les sources actuelles annoncent `0.27.0-alpha.10`.
La [matrice alpha.10](Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.10.md)
regroupe les résultats de la publication, avec 619 refus différentiels.
Le développement après alpha.10, décrit plus bas, en vérifie 2 751. Le commit
signé [`039bd3f`](https://github.com/Galactic-Shrine/GsPlusPlus/commit/039bd3f8887c9d8bc826e000d3b8099609d52850)
a été poussé sur `main` le 6 octobre 2026 ; GitHub confirme sa signature valide.
Sa [CI Windows, Linux et MSBuild natif](https://github.com/Galactic-Shrine/GsPlusPlus/actions/runs/37485258264)
réussit et valide la consolidation à 2 719 refus, y compris les tranches de
références et callbacks, groupes/opérateurs mixtes, initialiseurs agrégés,
interfaces préparées, assemblage préparé, origines des unités et normalisation
des déclarations libres. Les tranches suivantes de normalisation des membres
et groupes mixtes, d'origines des inclusions préparées et de leur raccordement
multi-unités restent locales et non commitées ; leurs 2 751 refus sémantiques,
64 refus de normalisation préparée et 12 refus de normalisation avec inclusions
ne sont pas revendiqués pour cette CI.
Aucune de ces tranches n'est incluse dans les paquets alpha.10 publiés.
Les sections de jalons ci-dessous conservent
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

## Déclarations locales contextuelles — tranche locale du 5 octobre 2026

**VALIDÉ dans le périmètre testé.** Les types locaux restent normalisés dans
la copie privée de l'AST, mais leurs erreurs ne sont plus renvoyées avant la
visite des instructions précédentes. Chaque déclaration contrôle son type,
les tableaux de références, le type `vide`, les noms répétés et l'initialisation
obligatoire avant d'analyser son initialiseur ou ses arguments de construction.
Une erreur antérieure du corps ou d'une fonction précédente garde ainsi la
priorité sur une déclaration invalide située plus loin, comme dans le bootstrap.

Les paramètres répétés sont contrôlés à l'entrée de chaque fonction, méthode,
constructeur ou prototype, avant ses initialiseurs et son corps. Les contrôles
globaux des signatures et conflits de membres restent dans leurs passes dédiées.
Les déclarations locales ne sont plus refusées dans la passe globale des conflits.

Quatre diagnostics bilingues sont ajoutés à la fin de l'énumération existante :

| Code | Français | English |
|---|---|---|
| 122 | `ConstructionExigeClasse` | `ConstructionRequiresClass` |
| 123 | `VariableLocaleVideInterdite` | `VoidLocalVariableForbidden` |
| 124 | `ReferenceLocaleNonInitialisee` | `UninitializedLocalReference` |
| 125 | `VariableLocaleConstanteNonInitialisee` | `UninitializedConstLocalVariable` |

La forme `entier32 valeur();` est refusée avant ses arguments : cette syntaxe
de construction est réservée aux classes. Les structures continuent d'utiliser
leur initialisation agrégée avec `=`. Une variable `vide`, une référence sans
initialiseur et une constante non-adresse sans initialiseur sont refusées ;
les pointeurs `vide*`, pointeurs constants et callbacks constants restent
acceptés dans les cas autorisés par le bootstrap. Les diagnostics de type et
de déclaration gardent la priorité sur les expressions de l'initialiseur.

La recherche des noms et le contrôle des doublons utilisent la même limite
de portée. Une variable déclarée directement dans une branche `si` / `if` ou
un corps `tantque` / `while` sans bloc reste visible dans son propre initialiseur,
mais pas dans l'autre branche ou après l'instruction. Les portées sœurs peuvent
réutiliser un nom ; un paramètre ou une variable d'une portée ancêtre active
reste un conflit, conformément au bootstrap actuel.

### Régressions

- **14 corpus valides bilingues**, soit 28 analyses : pointeurs, références,
  constantes, callbacks, agrégats, alias importés, classes et tableaux, méthodes,
  constructeurs, portées sœurs et branches/boucles sans accolades ;
- **42 refus sémantiques bilingues**, soit 84 comparaisons : diagnostics 122–125,
  doublons locaux et de paramètres, types absents ou importés ambigus,
  tableaux de références, signatures de callbacks et priorités entre instructions,
  fonctions, paramètres et corps ;
- **trois corpus d'émission valides bilingues**, soit six comparaisons, et
  **six refus d'émission bilingues**, soit 12 contrôles des sorties intactes ;
- total différentiel : **2 053 refus**, soit 96 de plus que la tranche précédente ;
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
**20/20** dans chaque construction. Les contrôles de style et de projets natifs,
les capacités d'analyse, les plans de durée de vie et l'intégration existante
réussissent également.

Les trois `Frontend.GsE` sont identiques : **448 959 octets**, trois segments,
huit sections, 75 exports, deux imports ; SHA-256
`056db58caa7a6aaf905edccc0ddeed01fd449c212926682d596f6c630b6e7bf0`.
Les trois vérificateurs acceptent leur image comme GsE 1.0. Ces données
remplacent les tailles et hachages précédents pour l'état local actuel.

`VERSION` et les trois en-têtes Release générés restent à `0.27.0-alpha.10`.
Les diagnostics 0–121 conservent leur valeur ; seuls les codes 122–125 sont
ajoutés. Les dispositions publiques auto-hébergées, formats 1.0 et ABI 1
sont inchangés. `ConteneursDynamiques.GsPP` reste inchangé.
Cette nouvelle tranche n'est ni commitée ni poussée ; aucun tag, paquet ou
release n'est créé.

### Limite du générateur machine lors de cette première validation

Les corpus réutilisant un nom local dans des portées distinctes passent les
deux analyseurs sémantiques. Le générateur C++ `GenerateurX64` recense toutefois
les variables dans une seule table de noms par fonction et refuse encore ces
programmes lors de l'allocation des emplacements. Les corpus d'émission de cette
première validation utilisaient donc des noms distincts ; leur validation ne
prétendait pas corriger cette limite du backend. La tranche suivante lève cette
limite et réintroduit les noms identiques dans le corpus d'émission concerné.

Ces tests ne valident pas toutes les qualifications, signatures et interactions
sémantiques de 0.27. L'alimentation autonome par interfaces et le backend
auto-hébergé ne sont pas ajoutés ; le passage à 0.28 n'est pas déclaré acquis.

## Génération des variables locales par portée — tranche locale du 5 octobre 2026

**VALIDÉ dans le périmètre testé du backend C++.** La réservation des emplacements
de pile utilise désormais l'identité de chaque `InstructionVariable`, et non
son seul nom. Les paramètres sont actifs dès l'entrée de fonction ; chaque
nom local est activé lorsque sa déclaration est générée. La fermeture d'un
bloc, d'une branche ou d'un corps de boucle retire uniquement ses noms locaux,
après l'émission des destructions, en conservant les paramètres et portées
ancêtres. Le même traitement s'applique aux branches et boucles sans accolades.

Le générateur accepte ainsi un même nom dans des portées distinctes, même avec
des types ou tailles différents. Les déclarations dont les durées de visibilité
se chevauchent restent refusées par les règles sémantiques existantes ; cette
correction n'introduit pas le masquage d'un paramètre ou d'une variable locale
ancêtre. Les emplacements ne sont pas réutilisés entre déclarations : l'ordre
de réservation et les alignements existants sont conservés.

La correction porte sur `Compiler/include/GsPP/GenerateurX64.hpp` et
`Compiler/src/GenerateurX64.cpp`. Elle ne modifie pas l'AST public auto-hébergé,
ses diagnostics, les formats binaires ou l'ABI, et ne constitue pas un backend
auto-hébergé en Gs++.

### Régressions et exécution

- test unitaire de parité du code français/anglais, avec noms réutilisés pour
  des types 8 et 64 bits et réutilisation après des blocs ; quatre refus
  maintiennent les conflits avec un paramètre ou une portée ancêtre active ;
- les **14 corpus valides bilingues** de déclarations locales atteignent
  désormais aussi la génération machine C++, soit 28 générations ; le corpus
  d'émission de branches sans accolades réutilise à nouveau le même nom ;
- **dix nouveaux corpus bilingues**, soit **20 exécutions** de code généré,
  après comparaison des deux analyseurs sémantiques : structures, références,
  callbacks, globales, branches avec/sans blocs, boucles et tableaux ;
- chaque programme retourne **42**, n'ajoute aucun import d'hôte et produit
  une image GsE reproductible ; les codes et données des variantes française
  et anglaise sont identiques ;
- traces de destruction contrôlées après le retour : **123** pour les objets
  successifs, **1122** pour les tableaux et **2131** pour les retours anticipés
  dans les branches ;
- nouveaux exemples `Tests/Integration/PorteesLocales/Principal.GsPP` et
  `Principal.en.GsPP`, compilés en GsE, vérifiés et exécutés : résultat **42**
  et trace **122345** contrôlée par le programme ; ajout au test d'intégration
  GNU/Linux et vérification directe avec les deux constructions Windows ;
- total des refus différentiels conservé à **2 053** : les nouvelles régressions
  sont positives et ne sont pas ajoutées artificiellement à ce compteur.

La traduction du corpus reconnaît maintenant aussi `si` / `if`, `sinon` /
`else` et `tantque` / `while`, sans modifier les noms contenant ces séquences.
L'exemple d'intégration utilise le nom complet de sa globale `Trace` depuis
le destructeur ; il ne revendique pas de nouvelle recherche implicite des
espaces de noms ancêtres.

### Matrice reconstruite

```powershell
cmake --build --preset windows-release --target espace_travail --parallel 6
ctest --preset windows-release --output-on-failure
wsl -d Ubuntu -- bash -lc 'cd /mnt/d/Langage-GsPlusPlus && cmake --build --preset linux-release --target espace_travail --parallel 4 && ctest --preset linux-release --output-on-failure'
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' GsPlusPlus.slnx /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' VisualStudio/Validation.vcxproj /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
```

Résultats finaux : CTest **5/5** sous MSVC, **6/6** sous GNU/Linux, solution
et validation MSBuild natives réussies ; conformité **20/20** dans les trois
constructions. Les dix corpus bilingues sont exécutés par chaque chaîne.
Les deux exemples d'intégration retournent également 42 sous chaque chaîne.

Les trois `Frontend.GsE` reconstruits restent identiques à ceux de la tranche
précédente : **448 959 octets**, SHA-256
`056db58caa7a6aaf905edccc0ddeed01fd449c212926682d596f6c630b6e7bf0`.
Les vérificateurs GsE acceptent les images reconstruites. `VERSION` et les
en-têtes Release restent à `0.27.0-alpha.10` ; formats 1.0 et ABI 1 conservés.
`ConteneursDynamiques.GsPP` reste inchangé. Aucun commit, push, tag, paquet
ou release n'est créé pour cette nouvelle tranche.

La consolidation des qualifications et contextes sémantiques non couverts,
l'alimentation autonome par interfaces et le backend auto-hébergé restent
des travaux distincts. GsBuild reste prévu pour le jalon 0.28 et n'est pas
implémenté par cette correction.

## Recherche lexicale dans les espaces parents — 5 octobre 2026

**VALIDÉ dans le périmètre testé :** la recherche des noms remonte les espaces
parents sans exiger une directive `utilisant espace`, dans le bootstrap C++
et dans le frontend Gs++. Cette tranche suit celle des portées locales.

### Correction et règle de recherche

Un test de méthode de classe dans un espace nommé reproduisait le refus
bootstrap `variable ou fonction introuvable : Valeur`. La remontée lexicale
existait dans la recherche des imports, mais cette passe était désactivée
en l'absence d'une utilisation visible. Les replis historiques ne remontaient
pas tous les parents et certains privilégiaient un homonyme à la racine.

- La recherche des types nommés, alias, valeurs d'énumération, globales,
  fonctions et opérateurs libres utilise désormais la remontée lexicale,
  même sans import. Elle couvre les espaces imbriqués et les contextes de
  méthodes, de signatures et de bases de classes.
- Le premier niveau contenant un nom masque les niveaux externes. Une
  surcharge incompatible à ce niveau produit son diagnostic sans essayer
  une surcharge d'un espace plus éloigné. Une globale non appelable masque
  de même une fonction externe.
- Les paramètres et variables locales gardent leur priorité. Les directives
  d'import continuent d'agir après leur déclaration dans leur unité et leur
  espace, avec les règles existantes d'ancêtre commun et d'ambiguïté.
- La priorité existante d'un nom explicitement qualifié correspondant à un
  nom complet est conservée ; à défaut, les qualifications relatives remontent
  les parents. Il ne s'agit pas d'une migration complète de la recherche C++.
- L'index privé comprend les portées de classes utilisées pour les types de
  signatures, mais `utilisant espace N::C` ne devient pas valide pour une
  classe. Sa capacité tient compte des jetons et nœuds, avec garde arithmétique
  avant allocation. L'AST public reste inchangé.
- Les valeurs d'énumération futures restent invisibles dans les initialiseurs
  des valeurs précédentes ; la nouvelle recherche n'élargit pas cette visibilité.

Les anciennes régressions qui imposaient une priorité racine ont été adaptées
à cette règle. Les contrôles de signatures distinctes, alias de surcharges,
types non visibles dans des espaces frères et bases non-classes restent
présents. Les résultats de ces anciennes sections sont des preuves historiques,
pas une prescription de priorité racine pour la source actuelle.

### Régressions et exécution

Dans `Tests/AutoHebergement/AutoHebergement.cpp`,
`TesterNomsDansEspacesParents` ajoute :

- **12 corpus valides bilingues**, soit 24 exécutions par construction, avec
  résultat 42, code/données français et anglais identiques, image reproduite
  après une seconde génération et aucun import d'hôte ;
- globales de méthodes, noms masqués à plusieurs niveaux, structures et enums,
  alias de types/fonctions, signatures de callbacks, qualification relative,
  variable locale prioritaire, destructeur et opérateur libre de l'espace parent ;
- **12 cas de refus bilingues**, soit 24 comparaisons supplémentaires de
  code/ligne/colonne, avec contrôle de l'AST intact ;
- types et valeurs invisibles depuis un espace frère, surcharge proche
  incompatible, globale non appelable et nom non-type masquant un type externe,
  priorité des diagnostics du corps, classe non importable et énumérateurs futurs.

Les régressions existantes de signatures nommées couvrent aussi le type de la
classe dans sa propre méthode. Le test unitaire C++ vérifie un corps de méthode
et un type de l'espace parent dans les deux syntaxes.

Le destructeur de `Tests/Integration/PorteesLocales/Principal.GsPP` et de sa
version anglaise utilise maintenant `Trace = Trace * 10 + soi.Id` / `this.Id`,
sans préfixe complet. Les exemples conservent leur trace 122345 et résultat 42.
Ils sont compilés, vérifiés et exécutés sous Windows/CMake, Windows/MSBuild et
GNU/Linux ; les variantes GNU font partie de `gspp_integration`.

### Validation finale de la source

Les commandes de construction et de tests sont celles de la tranche précédente :
`espace_travail` puis CTest avec les presets `windows-release` et
`linux-release`, et `GsPlusPlus.slnx` puis `VisualStudio/Validation.vcxproj`
avec MSBuild Visual Studio 2026, Release x64.

Résultats finaux : CTest **5/5** sous MSVC, **6/6** sous GNU/Linux ; solution
et validation MSBuild natives réussies, style conforme et conformité **20/20**
par chaîne. Les suites comparent désormais **2 077 corpus négatifs** au
bootstrap, contre 2 053 dans la tranche précédente.

Les trois `Frontend.GsE` reconstruits et acceptés par les vérificateurs GsE
sont identiques : **446 127 octets**, SHA-256
`9d8695f036b985cf4a452782d9b210fd9ba9f8e0aaaeb23f147774b57326df4d`.
La nouvelle image remplace la taille et l'empreinte précédentes pour la source
actuelle ; les contrats publics, 75 exports, 2 imports, formats 1.0 et ABI 1
restent inchangés. `VERSION` et les trois en-têtes Release générés indiquent
toujours `0.27.0-alpha.10`. `ConteneursDynamiques.GsPP` n'a pas été modifié.

Aucun commit, push, tag, paquet ni release n'est créé pour cette tranche.
Les 1 957 refus de la dernière CI publiée et les 619 de la publication alpha.10
ne sont pas présentés comme les résultats de ces nouvelles sources locales.
L'alimentation autonome par interfaces, les autres combinaisons sémantiques
non représentées et le backend auto-hébergé restent ouverts. GsBuild reste
prévu pour 0.28, sans implémentation dans cette tranche.

## Contexte lexical des noms dans les méthodes — 5 octobre 2026

**VALIDÉ dans le périmètre testé :** le frontend Gs++ commence sa recherche
des noms à la portée de la classe dans les méthodes, constructeurs et
destructeurs, puis remonte les espaces parents comme le bootstrap C++.

### Divergence reproduite et correction

Un nouveau corpus déclarait `N::Lire()` et une méthode `N::C::Lire()`, puis
appelait `Lire()` depuis une autre méthode de `C`. Le bootstrap refusait
l'appel, faute du récepteur explicite requis par la méthode non liée ; le
frontend Gs++ acceptait à tort la fonction `N::Lire()` de l'espace parent.
La recherche utilisait l'espace public de l'AST compact, qui n'inclut pas la
portée de la classe pour les expressions de son corps.

`RechercheDepuisNoeudUtiliseSemantique` reconstruit désormais la portée
effective à partir des ancêtres du nœud. Lors de l'évaluation des champs
par défaut, il utilise aussi le constructeur actif. Cette correction garde
l'AST public intact : la portée de recherche reste une donnée privée.

Les paramètres et variables locales continuent à précéder cette recherche.
Une méthode homonyme masque les fonctions parentes et importées, sans repli
sur celles-ci lorsque sa signature ne convient pas. Une fonction libre de
même nom complet dans un espace homonyme de la classe reste membre du groupe
mixte existant, avec la sélection typée déjà définie par le bootstrap.

Le bootstrap C++ n'est pas modifié par cette tranche. Les règles d'appel
restent inchangées : la forme non liée `Lire(soi)` conserve son paramètre
récepteur, `soi.Lire()` conserve sa syntaxe de membre et `Lire()` n'injecte
pas automatiquement `soi`. Une qualification `N::Lire()` peut toujours
désigner la fonction de l'espace parent. Les droits d'accès restent contrôlés.

### Matrice et exécution

`TesterNomsDansMethodesSemantiques` dans
`Tests/AutoHebergement/AutoHebergement.cpp` ajoute :

- **14 corpus valides bilingues**, soit 28 exécutions par construction ;
  résultat 42, code/données français et anglais identiques, image reproduite
  après une seconde génération et aucun import d'hôte ;
- contrôles des cibles publiées : famille de déclaration, classe/espaces,
  et, pour les fonctions, position de déclaration et type de retour choisis
  par comparaison aux références du bootstrap après analyse ;
- appels directs avec récepteur explicite, méthode privée appelée depuis sa
  classe, surcharges, groupe mixte, callback de méthode, récursion, qualification
  de la fonction parente, paramètres prioritaires et masquage d'une fonction
  importée ;
- champs par défaut, arguments d'initialiseur, corps de constructeurs et
  destructeurs, ainsi qu'un callback paramètre de constructeur masquant une
  méthode homonyme pendant l'initialisation d'un champ ;
- **20 cas de refus bilingues**, soit 40 comparaisons code/ligne/colonne
  supplémentaires, avec contrôle de l'AST intact ;
- récepteurs manquants, types incompatibles, adresses de surcharges, appels
  indirects, conversion de callback, cible d'affectation, accès privé depuis
  une autre classe, imports masqués et diagnostic précédent/suivant dans un corps.

La suite différentielle vérifie désormais **2 117 corpus négatifs**, contre
2 077 dans la tranche précédente. Les matrices et résultats antérieurs
restent des preuves historiques ; cette correction complète leur couverture.

### Validation finale

Commandes : `cmake --build --preset windows-release --target espace_travail
--parallel 6` puis `ctest --preset windows-release --output-on-failure` ;
équivalents `linux-release` sous WSL Ubuntu avec `--parallel 4` ; construction
de `GsPlusPlus.slnx` puis validation `VisualStudio/Validation.vcxproj` avec
MSBuild Visual Studio 2026, Release x64.

Résultats : CTest **5/5** sous Windows/MSVC, **6/6** sous GNU/Linux,
solution et validation MSBuild natives réussies ; style conforme et
conformité **20/20** dans les trois constructions. Chaque chaîne exécute
les 14 nouveaux corpus bilingues et les suites précédentes.

Les trois `Frontend.GsE` reconstruits sont identiques et acceptés par leurs
vérificateurs : **446 927 octets**, SHA-256
`dc5760dee21d2bcced03d5cdbf5948b21d6b5cc77c898874f2d126c2a51ebe7c`.
Cette taille et cette empreinte remplacent les valeurs de la tranche
précédente pour la source actuelle. Les 75 exports, 2 imports et diagnostics
publics, formats 1.0 et ABI 1 restent inchangés. `VERSION` et les trois
en-têtes Release générés indiquent `0.27.0-alpha.10`.

`ConteneursDynamiques.GsPP` reste inchangé. Aucun commit, push, tag, paquet
ni release n'est créé pour cette tranche locale. Elle ne clôt pas toutes
les qualifications ni les contextes sémantiques de 0.27 ; l'alimentation
autonome par interfaces et le backend auto-hébergé restent ouverts. GsBuild
reste prévu pour 0.28 et n'est pas implémenté ici.

## Portées des opérateurs dans les méthodes — tranche locale du 5 octobre 2026

**VALIDÉ dans le périmètre testé.** La correction du contexte de recherche
décrite dans la tranche précédente couvre également les opérateurs. Cette
tranche ajoute leurs tests de régression ; elle ne modifie aucun algorithme
du bootstrap C++ ni du frontend Gs++, ni les diagnostics publics.

### Contrat et couverture

- le groupe associé au type de gauche, ou à l'opérande unaire, est prioritaire ;
  son héritage et ses contrôles d'accès restent applicables ;
- à défaut, le groupe lexical de la classe masque les espaces parents ou
  importés, même s'il ne contient aucune surcharge compatible ; les groupes
  mixtes méthode/fonction portant le même nom complet restent sélectionnables ;
- les opérandes sont les arguments de la surcharge, sans ajout automatique
  d'un récepteur de la classe appelante ;
- **17 corpus valides bilingues**, soit 34 exécutions par construction :
  méthodes, groupes mixtes binaires/unaires, espaces imbriqués et importés,
  champ par défaut, initialiseur explicite, corps de constructeur, destructeur,
  opérateur de l'objet passé en paramètre et accès protégé depuis une classe dérivée ;
- deux corpus évaluent un même champ par défaut depuis plusieurs constructeurs :
  surcharges distinctes pour deux types d'objets, puis opérateur surchargé sur
  un objet et intrinsèque sur un booléen, sans réutilisation de la résolution précédente ;
- les appels d'opérateurs du bootstrap sont comparés aux résolutions publiques
  Gs++ : position exacte de la déclaration, type de retour et drapeau de méthode.
  Les résolutions d'un même emplacement partagé par plusieurs constructeurs
  sont comparées comme un multiensemble, sans omission ni duplication ;
- chaque paire français/anglais produit les mêmes octets de code et données,
  une image reproductible sans import d'hôte, et retourne 42 lors du chargement ;
- **17 cas de refus bilingues**, soit 34 comparaisons code/ligne/colonne et AST
  intact supplémentaires : masquage incompatible, accès privé, objet constant,
  opérateur absent, opérande droite sans récepteur implicite, ordre des
  instructions, champ par défaut sans constructeur et contexte incompatible
  d'un second constructeur.

La matrice locale vérifie désormais **2 151 corpus négatifs**, contre 2 117
dans la tranche précédente. Les preuves antérieures restent historiques.
La version reste `0.27.0-alpha.10`, avec les formats 1.0 et ABI 1.
Cette tranche locale n'est ni commitée, ni poussée, ni publiée.

### Validation finale

Commandes : `cmake --build --preset windows-release --target espace_travail
--parallel 6` puis `ctest --preset windows-release --output-on-failure` ;
équivalents `linux-release` sous WSL Ubuntu avec `--parallel 4` ; construction
de `GsPlusPlus.slnx` puis `VisualStudio/Validation.vcxproj` avec MSBuild
Visual Studio 2026, Release x64.

Résultats : CTest **5/5** sous Windows/MSVC et **6/6** sous GNU/Linux,
solution et validation MSBuild natives réussies ; conformité **20/20** dans
les trois constructions, style et cohérence des projets natifs conformes.
Chaque chaîne exécute les 17 corpus bilingues et vérifie les 2 151 refus
différentiels avec les suites précédentes.

Les trois `Frontend.GsE` ont été vérifiés : **446 927 octets**, SHA-256
`dc5760dee21d2bcced03d5cdbf5948b21d6b5cc77c898874f2d126c2a51ebe7c`.
Leur identité avec la tranche précédente est attendue : seuls les tests et
la documentation changent ici. Format GsE 1.0, 3 segments, 8 sections,
2 imports et 75 exports. Les trois `VersionProduit.hpp` Release indiquent
`0.27.0-alpha.10` ; `ConteneursDynamiques.GsPP` reste inchangé.

## Qualifications des constructions et conversions des champs par défaut — 6 octobre 2026

**VALIDÉ dans le périmètre testé.** Le frontend Gs++ conserve désormais le
contexte de classe pour les types de conversions écrites dans une valeur de
champ par défaut. Le bootstrap C++ n'est pas modifié par cette tranche.

### Divergence reproduite et correction

Dans le premier corpus de `TesterQualificationsConstructionsSemantiques`,
`N::Vue` désigne `N::P`, tandis que `N::C::Vue` désigne `N::Q`. Le champ de
`N::C` est un `constante Q*`, avec la valeur par défaut
`convertir<constante Vue*>(p)` et un constructeur prenant `Q* p`.
Le bootstrap choisit l'alias `N::C::Vue` ; avant correction, le frontend
choisissait `N::Vue` et refusait ce programme valide avec le diagnostic
**37**, ligne 1, colonne 113, au lieu de réussir.

La canonicalisation privée des types donne maintenant aux conversions
descendantes d'un champ de classe la portée du constructeur qui les évalue.
Cette règle s'applique aussi aux conversions imbriquées dans des agrégats
et aux types nommés de signatures de callbacks. Le type déclaré du champ
conserve sa propre portée. L'AST fourni par l'appelant reste intact et les
erreurs des conversions ne sont émises que si la valeur par défaut est
effectivement visitée : remplacement explicite et délégation restent respectés.
Aucun diagnostic public, export, format, ABI ou numéro de produit ne change.

### Matrice et exécution

- **15 corpus valides bilingues**, soit 30 exécutions par construction, tous
  avec résultat 42, octets français/anglais identiques, images reproductibles
  et absence d'import d'hôte ;
- alias masquant un type parent ou importé dans une conversion de champ,
  agrégat contenant cette conversion, callback avec type nommé imbriqué,
  paramètres de plusieurs constructeurs et valeurs par défaut remplacées ;
- surcharges de constructeurs portant des pointeurs ou références qualifiés,
  champs objets, initialisation de base et délégation avec conversion explicite ;
- conversions dérivée-vers-base par pointeur et référence depuis une classe
  dérivée polymorphe, sans revendication d'une nouvelle disposition ABI ;
- les cibles de constructions locales, de bases, de champs et de délégations
  sont comparées aux choix du bootstrap : emplacement source, catégorie
  d'initialisation, position exacte du constructeur et type de retour, sans
  omission ni résolution dupliquée ; les plans de durée de vie restent distincts ;
- **16 refus bilingues**, soit 32 comparaisons supplémentaires de code, ligne,
  colonne et AST intact : types de conversion absents ou masqués par un non-type,
  séparation entre portée du type du champ et portée de sa conversion,
  perte de qualification, temporaire non adressable, slicing par valeur,
  pointeurs d'héritage à deux niveaux, visibilité, arité, agrégats incompatibles
  et contexte incompatible d'un second constructeur ;
- priorité de la visibilité ou de l'arité de construction sur les expressions
  suivantes, et d'un type de conversion absent dans le champ par défaut sur
  une référence inconnue dans le corps du constructeur.

La matrice locale atteint **2 183 corpus négatifs**, contre 2 151 dans la
tranche précédente. La correction ne rend pas implicites de nouvelles
conversions de qualifications : les ajouts de qualification aux pointeurs
ordinaires des corpus passent par les conversions explicites déjà prises en
charge. Elle ne complète pas tout le frontend 0.27.

### Validation finale

Commandes : `cmake --build --preset windows-release --target espace_travail
--parallel 6` puis `ctest --preset windows-release --output-on-failure` ;
équivalents `linux-release` sous WSL Ubuntu avec `--parallel 4` ; construction
de `GsPlusPlus.slnx` puis `VisualStudio/Validation.vcxproj` avec MSBuild
Visual Studio 2026, Release x64.

Résultats : CTest **5/5** sous Windows/MSVC, **6/6** sous GNU/Linux,
solution et validation MSBuild natives réussies ; conformité **20/20** dans
les trois constructions, style et cohérence des projets natifs conformes.
Chaque chaîne exécute les 15 nouveaux corpus bilingues et vérifie les
2 183 refus différentiels avec toutes les suites précédentes.

Les trois `Frontend.GsE` sont identiques et acceptés par leurs vérificateurs :
**447 263 octets**, SHA-256
`df4a1d390e5f87c3a9712c708d6d0e4a362759b177680f31dbbb66d05509f9ed`.
Cette taille et cette empreinte remplacent celles de la tranche précédente
pour la source actuelle. GsE 1.0, 3 segments, 8 sections, 2 imports et
75 exports ; l'objet sémantique contient 333 941 octets de code, 294 symboles
et 1 724 relocalisations ; l'image liée conserve 534 symboles avec
2 595 relocalisations.

`VERSION` et les trois `VersionProduit.hpp` Release indiquent
`0.27.0-alpha.10`, les formats 1.0 et ABI 1 restent inchangés.
`ConteneursDynamiques.GsPP` est inchangé. Aucun commit, push, tag, paquet
ni release n'est créé pour cette tranche locale. GsBuild reste prévu pour
0.28 et n'est pas implémenté ici.

## Callbacks des champs par défaut par constructeur — 6 octobre 2026

**VALIDÉ dans le périmètre testé.** La fonction de régression
`TesterCallbacksChampsContextuelsSemantiques` complète les combinaisons de
callbacks des valeurs par défaut évaluées depuis plusieurs constructeurs.
Cette tranche étend les tests : elle ne modifie ni le bootstrap C++, ni les
algorithmes du frontend Gs++, ni les diagnostics publics.

### Matrice différentielle et exécution

- **10 corpus valides bilingues exécutés**, soit 20 exécutions par construction,
  avec résultat 42, octets français/anglais identiques, images reproductibles
  et aucun import d'hôte ; la sémantique auto-hébergée est comparée au bootstrap,
  tandis que le code machine exécuté est généré par le backend C++ ;
- appels de callbacks par valeur ou référence, via pointeur déréférencé ou
  indexation, signatures `entier32`/`entier64` différentes suivant le constructeur ;
- fabriques retournant un callback ou un pointeur vers un callback, appels
  imbriqués, callback recevant une référence avec mutation effectivement vérifiée,
  stockage dans un agrégat et tableau de callbacks ;
- pour chaque référence de callback dans un champ, comparaison de la position
  exacte et de l'empreinte complète du type du paramètre choisi avec le bootstrap ;
  chaque constructeur doit publier le nombre attendu de résolutions, sans
  omissions, doublons ni réutilisation du paramètre d'un autre constructeur ;
- valeur par défaut invalide non visitée quand elle est remplacée par un
  initialiseur explicite via alias de champ, y compris après délégation ;
- **1 corpus valide bilingue uniquement sémantique** contrôle les références de
  callbacks `constante` et `volatile` dans deux constructeurs et leurs appelants ;
  aucune exécution machine n'est revendiquée pour ce corpus ;
- **14 refus bilingues**, soit 28 comparaisons supplémentaires de code, ligne,
  colonne et AST intact : signature du second constructeur incompatible,
  ordre inversé des constructeurs, retour non appelable, mauvaise arité,
  pointeur non appelable, tableau de callbacks incompatible, argument de
  référence non adressable ou perdant sa constance, conversion entre signatures,
  paramètre absent et priorités des diagnostics avant le corps du constructeur.

La matrice locale atteint **2 211 corpus négatifs**, contre 2 183 avant cette
tranche. Le contrat des qualifications existantes est conservé : cette extension
ne rend pas implicites de nouvelles conversions. Les fonctions Gs++ ordinaires
ne peuvent toujours pas retourner une référence ; les fabriques exécutées
retournent donc un callback par valeur ou un pointeur. Ni la complétude du
frontend 0.27, ni un backend auto-hébergé, ni des sorties PE/ELF ne sont revendiqués.

### Validation finale

Commandes exécutées :

```powershell
cmake --build --preset windows-release --target espace_travail --parallel 6
ctest --preset windows-release --output-on-failure
wsl -d Ubuntu -- bash -lc 'cd /mnt/d/Langage-GsPlusPlus && cmake --build --preset linux-release --target espace_travail --parallel 4 && ctest --preset linux-release --output-on-failure'
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' GsPlusPlus.slnx /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' VisualStudio/Validation.vcxproj /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
```

Résultats : CTest **5/5** sous Windows/MSVC, **6/6** sous GNU/Linux,
solution et validation MSBuild natives réussies. Conformité **20/20** dans
chaque construction, style et cohérence des projets natifs conformes.
Les trois chaînes vérifient les nouveaux corpus et les **2 211 refus** de la
matrice différentielle ; les suites précédentes restent incluses.

Les trois `Frontend.GsE` sont identiques et acceptés par leurs vérificateurs :
**447 263 octets**, SHA-256
`df4a1d390e5f87c3a9712c708d6d0e4a362759b177680f31dbbb66d05509f9ed`.
L'image est inchangée depuis la correction précédente : GsE 1.0, 3 segments,
8 sections, 2 imports et 75 exports. Cette tranche n'ajoute que des tests et
leur documentation.

`VERSION` et les trois `VersionProduit.hpp` Release restent à
`0.27.0-alpha.10` ; formats 1.0 et ABI 1 inchangés.
L'empreinte de `ConteneursDynamiques.GsPP` reste
`4eb8f7384823beb1d9e2c9b073f67487afe5f4b3fc038c5387181c1e2c50ef67`.
Aucun commit, push, tag, paquet ou release n'est créé pour cette tranche.
GsBuild reste prévu pour 0.28, sans implémentation ici.

## Arguments agrégés des callbacks et diagnostics des champs — 6 octobre 2026

**VALIDÉ dans le périmètre testé.** La matrice
`TesterCallbacksChampsContextuelsSemantiques` a révélé un écart dans la
propagation des diagnostics des champs par défaut. Le nouveau refus d'indice
14 passe `{42}` à un callback recevant `Q`, dont le champ est `booléen`.
Le bootstrap refuse la feuille avec le diagnostic **45**, ligne 1, colonne
91 ; avant correction, le frontend remplaçait ce refus par **37**, ligne 1,
colonne 83, à l'appel englobant.

### Correction

Le diagnostic 37 d'incompatibilité avec un champ n'est plus obtenu en
réécrivant tout diagnostic 45 retourné par son initialiseur. Un drapeau
privé de la destination d'initialisation réserve désormais ce diagnostic
au contrôle final de compatibilité avec le champ, après résolution réussie
de l'expression. Les erreurs internes des arguments, appels imbriqués et
conversions conservent ainsi leur code et leur position. Les initialiseurs
entre accolades et les valeurs explicitement remplacées gardent leur
comportement antérieur.

Le bootstrap C++ est inchangé. Aucun diagnostic public n'est ajouté ou
renuméroté ; les exports, formats et ABI ne changent pas, et l'AST fourni par
l'appelant reste intact.

### Matrice et périmètre

- **9 nouveaux corpus bilingues exécutés**, soit 18 exécutions par construction,
  avec résultat 42, octets français/anglais identiques, images reproductibles
  et aucun import d'hôte ; la sémantique auto-hébergée est comparée au bootstrap,
  et le code machine exécuté est généré par le backend C++ ;
- arguments de structures différentes suivant le constructeur, tableaux de
  champs imbriqués, fabriques retournant un callback, pointeurs qualifiés dans
  les agrégats, tableaux de callbacks et champs manquants initialisés à zéro ;
- appels agrégés dans les initialisations de champs objets, de bases et les
  délégations, avec retours `entier32`/`entier64` et surcharges de construction ;
- retour booléen compatible avec le champ, en plus des refus où un appel valide
  produit une valeur incompatible : ces derniers gardent bien le diagnostic 37 ;
- **1 nouveau corpus bilingue uniquement sémantique** vérifie les arguments
  agrégés d'un callback obtenu par référence via une signature de callback ;
  il ne définit pas une fonction ordinaire retournant une référence et ne
  revendique pas son exécution machine ;
- positions exactes et empreintes complètes des paramètres de callbacks des
  champs toujours comparées au bootstrap, sans omissions ni résolutions dupliquées ;
- **22 nouveaux refus bilingues**, soit 44 comparaisons supplémentaires de code,
  ligne, colonne et AST intact : capacité des tableaux et structures, signatures
  de deux constructeurs dans les deux ordres, références non adressables, perte
  de qualification, conversions de callbacks, valeurs non appelables et priorités
  entre appels agrégés, accès de construction, champs successifs et corps ;
- régressions spécifiques des diagnostics internes dans un appel direct,
  un appel indirect, une fabrique et une conversion englobante, avec maintien
  de 37 pour les véritables incompatibilités finales de champs.

Le groupe de tests cumulé comporte maintenant **19 corpus bilingues exécutés**
et **2 corpus bilingues uniquement sémantiques**. La matrice locale complète
atteint **2 255 corpus négatifs**, contre 2 211 avant cette tranche.

### Validation finale

Commandes : construction de `espace_travail` avec le preset `windows-release`
et `--parallel 6`, puis `ctest --preset windows-release --output-on-failure` ;
équivalents `linux-release` sous WSL Ubuntu avec `--parallel 4` ; construction
de `GsPlusPlus.slnx` puis de `VisualStudio/Validation.vcxproj` avec MSBuild
Visual Studio 2026, Release x64.

Résultats : CTest **5/5** sous Windows/MSVC et **6/6** sous GNU/Linux,
solution et validation natives réussies ; conformité **20/20** dans les trois
constructions, style et cohérence des projets natifs conformes. Chaque chaîne
exécute les **19 corpus bilingues** du groupe cumulé, contrôle ses deux corpus
bilingues uniquement sémantiques et vérifie les **2 255 refus différentiels**.

Les trois `Frontend.GsE` reconstruits sont identiques et acceptés par leurs
vérificateurs : **447 375 octets**, SHA-256
`67dfed60b339f2ffd2826c770573c521326417c62dfe96d33a3d88be08ad9a50`.
Cette taille et cette empreinte remplacent celles de la tranche précédente
pour la source actuelle. GsE 1.0, 3 segments, 8 sections, 2 imports, 75 exports.
L'objet sémantique contient 334 059 octets de code, 294 symboles et 1 725
relocalisations ; l'image liée conserve 534 symboles avec 2 596 relocalisations.

`VERSION` et les trois `VersionProduit.hpp` Release restent à
`0.27.0-alpha.10` ; formats 1.0, ABI 1 et `ConteneursDynamiques.GsPP` inchangés.
Aucun commit, push, tag, paquet ni release n'est créé pour cette tranche locale.
Ni le frontend 0.27 complet, ni un backend auto-hébergé, ni des sorties PE/ELF
ne sont revendiqués. GsBuild reste prévu pour 0.28 et n'est pas implémenté ici.

## Retours par référence des callbacks — 6 octobre 2026

**VALIDÉ dans le périmètre testé.**
`TesterRetoursReferencesCallbacks` confronte les signatures déjà reconnues par
le frontend à leur utilisation effective dans le code x86-64. Les callbacks
sont des fonctions C++ de test fournies par argument, respectant `GsAbi:x64-ms-v1`
et renvoyant de véritables références vers le stockage de l'hôte. Aucune
fonction ordinaire Gs++ retournant une référence n'est définie par ces corpus.

### Défauts corrigés

- Le backend traitait un retour `entier32&` comme un entier immédiat : le
  premier corpus produisait la partie basse de l'adresse au lieu de lire la
  valeur référencée. La génération conserve désormais l'adresse de 64 bits
  à la sortie de l'appel, puis charge la valeur seulement si son consommateur
  demande une valeur. Le chemin d'adressage accepte également ces appels.
- Une structure retournée par référence ne doit pas être générée comme une
  structure retournée par valeur : elle n'utilise ni temporaire de retour ni
  argument caché d'adresse de destination. Les registres des arguments restent
  ceux de sa signature réelle.
- Le bootstrap C++ ne propageait pas la constance d'un appel retournant une
  référence scalaire ou structurée. Il refuse désormais les liaisons et
  affectations mutables interdites, comme le frontend Gs++ le faisait déjà.
- Le frontend Gs++ perdait la constance héritée d'un champ ou élément lors de
  sa prise d'adresse. Son calcul privé du type adressé conserve maintenant
  cette qualification, sans qualifier implicitement le pointeur stocké ni
  modifier l'AST public.

### Matrice et contrôles

Les **15 corpus bilingues exécutés** couvrent lecture scalaire, liaison de
référence, affectation directe, prise d'adresse, passage à un constructeur,
référence de structure, accès aux champs et tableaux, valeurs constantes et
volatiles, référence de callback et invocation imbriquée, et valeur de champ
par défaut évaluée depuis le constructeur. Les adresses de valeurs, champs et
éléments constants sont également utilisées par des pointeurs constants.

Pour chaque version française et anglaise, les tests vérifient :

- résultat 42, modifications exactes des données de l'hôte et nombre exact
  d'appels, pour détecter notamment une double évaluation lors de l'adressage ;
- disposition identique de la structure de test dans Gs++ et C++ : taille
  24 octets, alignement 8, champs aux offsets 0, 4 et 16 ;
- empreintes complètes et positions de tous les paramètres, sans omission ;
- octets de code et de données identiques entre syntaxes, image reproductible,
  absence d'import statique ajouté par les callbacks passés en argument ;
- requêtes de capacité et tampons partiels, avec AST public intact.

L'absence d'import statique ne rend pas ces corpus indépendants de l'hôte :
leur exécution utilise explicitement les callbacks C++ et le stockage de test.

Les **15 refus bilingues** vérifient les liaisons incompatibles, références sur
temporaires par valeur, suppression de constance, affectations interdites,
arité avant arguments invalides, valeur non appelable, membre et type de
conversion absents, et priorités avant une erreur ultérieure. Cela ajoute
**30 comparaisons code/ligne/colonne**, avec AST intact, et porte la matrice
locale à **2 285 refus différentiels**, contre 2 255 avant cette tranche.

### Validation finale

Commandes : `cmake --build --preset windows-release --target espace_travail
--parallel 6`, puis `ctest --preset windows-release --output-on-failure` ;
équivalents `linux-release` sous WSL Ubuntu avec `--parallel 4` ; construction
de `GsPlusPlus.slnx`, puis `VisualStudio/Validation.vcxproj` avec MSBuild
Visual Studio 2026, Release x64.

Résultats : CTest **5/5** sous Windows/MSVC, **6/6** sous GNU/Linux, solution
et validation natives réussies ; conformité **20/20** dans les trois chaînes,
style et cohérence des projets natifs conformes. Chaque chaîne exécute les
**15 corpus bilingues** de cette tranche et les **2 285 refus différentiels**.

Les trois `Frontend.GsE` sont identiques et acceptés par leurs vérificateurs :
**448 159 octets**, SHA-256
`7261e97651c701ec039159ca83d0fbc46867db67d838858875409bee16ef6982`.
Cette empreinte remplace celle de la tranche précédente pour les sources
actuelles. GsE 1.0, 3 segments, 8 sections, 2 imports, 75 exports inchangés.
L'objet sémantique contient 334 838 octets de code, 295 symboles et 1 736
relocalisations ; l'image liée comporte 535 symboles et 2 607 relocalisations.
`VERSION` et les trois `VersionProduit.hpp` Release restent à
`0.27.0-alpha.10` ; `ConteneursDynamiques.GsPP` conserve son empreinte.

Les fonctions ordinaires Gs++ retournant une référence restent refusées dans
le sous-ensemble courant. Le stockage référencé par l'hôte doit rester valide
durant son utilisation ; aucun prolongement de durée de vie n'est ajouté.
Version alpha.10, formats 1.0 et ABI 1 conservés ; aucun commit, push ou release.
Le backend demeure C++ et les sorties natives PE/ELF ne sont pas ajoutées ici.

## Qualifications des champs adressés via callbacks — 6 octobre 2026

**VALIDÉ dans le périmètre testé.**
Cette extension de `TesterRetoursReferencesCallbacks` couvre les qualifications
`volatile` et `constante volatile` héritées par un champ ou élément de tableau
avant sa prise d'adresse, y compris après un accès par pointeur.

### Écart reproduit et correction

Le bootstrap accepte `volatile entier32* adresse = &lire(donnees).X` lorsque
le callback retourne `volatile P&`. Le frontend Gs++ refusait pourtant cet
initialiseur avec le diagnostic 45 à la position 1:190 du premier corpus de
régression, car le type adressé perdait `volatile`. L'ancien correctif pour
les valeurs constantes ne traitait pas cette qualification, et pouvait aussi
perdre `volatile` lorsque `constante` était présent simultanément.

Le calcul privé du type adressé recueille maintenant les bits `constante` et
`volatile` du type réel, des déclarations et des objets des accès membres ou
indexations. Il les conserve lors de la formation de l'adresse. La reconnaissance
des types qualifiés est partagée sans modifier les signatures, l'AST public ou
les diagnostics. Les champs pointeurs/callbacks gardent les qualifications de
leur type déclaré : celles de leur objet contenant ne sont pas ajoutées au
type du pointeur stocké. Les règles de conversion restent celles du bootstrap.

### Matrice complémentaire

Les **10 nouveaux corpus bilingues exécutés** vérifient :

- adresse d'une référence scalaire `volatile` ou `constante volatile` ;
- adresse d'un champ ou élément de tableau d'une référence de structure
  `volatile` ou `constante volatile`, avec lecture et mutations permises ;
- accès par flèche après liaison à un pointeur de structure qualifié ;
- copie et invocation d'un champ callback depuis les deux formes de structure
  qualifiée, pour vérifier qu'aucun qualificateur ne modifie sa signature.

Les contrôles du groupe précédent restent appliqués : résultat 42, stockage
exact de l'hôte, nombre exact d'appels, disposition mémoire concordante,
empreintes complètes et positions des paramètres, octets bilingues identiques,
images reproductibles, requêtes de capacité et AST intact. Ces corpus utilisent
des callbacks C++ fournis par l'hôte et ne sont donc pas indépendants de celui-ci.

Les **8 nouveaux refus bilingues**, soit 16 comparaisons de code/ligne/colonne,
couvrent les qualifications de pointeurs incompatibles ou supprimées, dont une
priorité sur une expression inconnue ultérieure. Le groupe cumulé comporte
**25 corpus bilingues exécutés** et **23 refus bilingues** ; la matrice locale
complète atteint **2 301 refus différentiels**, contre 2 285 avant cette tranche.

### Validation finale

Commandes : construction de `espace_travail` avec le preset `windows-release`
et `--parallel 6`, puis `ctest --preset windows-release --output-on-failure` ;
équivalents `linux-release` sous WSL Ubuntu avec `--parallel 4` ; construction
de `GsPlusPlus.slnx`, puis `VisualStudio/Validation.vcxproj` avec MSBuild
Visual Studio 2026, Release x64.

Résultats : CTest **5/5** sous Windows/MSVC et **6/6** sous GNU/Linux, solution
et validation natives réussies ; conformité **20/20** dans les trois chaînes,
style et cohérence des projets natifs conformes. Chaque chaîne exécute les
**25 corpus bilingues** du groupe cumulé et les **2 301 refus différentiels**.

Les trois `Frontend.GsE` sont identiques et acceptés par leurs vérificateurs :
**449 439 octets**, SHA-256
`ee318f7b8f3b006d9f2744201d813a0d45faff3235769463f33c7e64710343b9`.
Cette taille et cette empreinte remplacent celles de la tranche précédente
pour les sources actuelles. GsE 1.0, 3 segments, 8 sections, 2 imports et
75 exports inchangés. L'objet sémantique contient 336 120 octets de code,
297 symboles et 1 744 relocalisations ; l'image liée comporte 537 symboles
et 2 615 relocalisations. Les trois `VersionProduit.hpp` Release annoncent
`0.27.0-alpha.10` ; `ConteneursDynamiques.GsPP` conserve son empreinte.

Cette correction ne modifie pas le bootstrap C++ ni le backend. Elle n'ajoute
pas les retours par référence aux fonctions ordinaires Gs++, ni de garantie
d'atomicité, de synchronisation ou de barrière mémoire à `volatile`.
Version alpha.10, formats 1.0 et ABI 1 conservés ; aucun commit, push ou release.

## Références de pointeurs des callbacks et cibles de tableaux — 6 octobre 2026

**VALIDÉ dans le périmètre testé sur les trois chaînes de construction.**
`TesterRetoursReferencesPointeursCallbacks` étend les retours par référence
aux emplacements de pointeurs. Ses callbacks C++ de test retournent réellement
`entier32*&` ou `constante entier32*&` selon la signature testée, vers deux
emplacements distincts de stockage fournis par l'hôte.

### Contrat et défaut corrigé

La référence désigne ici l'emplacement contenant le pointeur, et non directement
la valeur pointée. Une lecture copie le pointeur ; une liaison ou une prise
d'adresse permet de modifier son emplacement. Les valeurs du tableau pointé
et les cibles des deux pointeurs sont donc vérifiées séparément. Dans la forme
`constante entier32*&`, les données pointées sont constantes, pas l'emplacement
du pointeur : celui-ci peut être remplacé par un autre pointeur du même type
qualifié, mais les données pointées ne peuvent pas être modifiées. Les tests
n'ajoutent pas de conversion implicite de qualification.

Le corpus de tableau de pointeurs a révélé un écart du frontend : après
`entier32* tableau[2] = {lire(d), &d->Valeurs[1]}`, l'affectation
`*tableau[0] = 42` était refusée avec le diagnostic 72 à la position 1:222 du
corpus de régression. Le contrôle comparait le type de la cible au type de
l'élément final de la déclaration d'origine et assimilait toute différence
à un tableau encore présent, même après déréférencement d'un pointeur.

`EstExpressionTableauSemantique` distingue désormais les dimensions qui restent
après les indexations de la déclaration réellement accédée. Un tableau entier
ou un sous-tableau conserve le diagnostic 72, prioritaire sur l'analyse de la
valeur affectée. Le déréférencement d'un pointeur extrait n'est plus considéré
comme un tableau ; une valeur incompatible conserve alors son diagnostic 73
et sa position. Le bootstrap C++, le backend et les diagnostics publics ne
changent pas dans cette tranche.

### Matrice

Les **20 corpus bilingues exécutés** couvrent lecture, copie, liaison,
réaffectation de pointeur, double indirection, passage à une fonction ou
un constructeur, retour ordinaire de pointeur par valeur, champ par défaut,
référents constants, indexation, adresse de référent, tableaux de pointeurs
à une ou deux dimensions et mutation d'un élément de tableau de pointeurs.

Chaque exécution vérifie le résultat 42, les deux valeurs du tableau hôte,
la cible exacte de chacun des deux pointeurs, et le nombre exact d'appels.
La disposition du pont de test est comparée au bootstrap : taille 24 octets,
alignement 8, champs aux offsets 0, 8 et 16. Les empreintes complètes et les
positions de tous les paramètres sont comparées ; les octets bilingues et
images reproductibles, les capacités/tampons partiels et l'AST intact restent
contrôlés. Ces images n'ajoutent pas d'import statique, mais leur exécution
utilise explicitement les callbacks C++ et le stockage de l'hôte.

Les **15 refus bilingues** ajoutent 30 comparaisons de code/ligne/colonne :
liaisons incompatibles ou sur retour par valeur, qualifications supprimées,
mutation d'un référent constant, cible non appelable, mauvaise arité avant
argument inconnu, indice booléen, affectation de pointeur incompatible et
véritables affectations de tableaux/sous-tableaux avant valeur inconnue.
La matrice complète atteint **2 331 refus différentiels**, contre 2 301 avant
cette tranche.

### Validation

Les trois constructions et validations complètes réussissent :

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail
  --parallel 6`, puis `ctest --preset windows-release --output-on-failure` :
  **5/5 tests réussis** ;
- GNU/Linux, sous Ubuntu/WSL : `cmake --build --preset linux-release --target
  espace_travail --parallel 4`, puis `ctest --preset linux-release
  --output-on-failure` : **6/6 tests réussis**, intégration comprise ;
- Visual Studio 2026 natif sans CMake : construction Release/x64 de
  `GsPlusPlus.slnx`, puis de `VisualStudio/Validation.vcxproj` avec MSBuild :
  **construction et validation réussies**.

La conformité est **20/20 par chaîne**. Les trois matrices sémantiques passent
les 20 nouveaux corpus bilingues exécutés et les **2 331 refus différentiels**.
Les versions générées indiquent toutes `0.27.0-alpha.10`.

Les trois images `Frontend.GsE` reconstruites sont identiques : **449 983 octets**,
SHA-256 `3013076EC85961F9BA5FCF97572E397D4A564FFE0810FF3FF6242A9289FD2D2A`.
Chaque image est acceptée par `gseverifier` : GsE 1.0, 3 segments, 8 sections,
2 imports et 75 exports. La reconstruction du frontend produit 336 658 octets
de code, 184 octets de données, 120 octets de données nulles, 297 symboles et
1 743 relocalisations pour l'objet ; l'édition de liens résout 537 symboles et
2 614 relocalisations.

`ConteneursDynamiques.GsPP` reste inchangé, SHA-256
`4EB8F7384823BEB1D9E2C9B073F67487AFE5F4B3FC038C5387181C1E2C50EF67`.

Version alpha.10, formats 1.0 et ABI 1 conservés. Cette tranche reste locale,
distincte du commit `c58874b` poussé et validé par sa CI ; aucun nouveau commit,
push, tag, paquet ou release n'est créé après cette consolidation. Les retours
par référence de fonctions ordinaires Gs++ restent refusés ; le stockage de
l'hôte doit rester valide et aucun backend auto-hébergé n'est revendiqué.

## Références de callbacks paramétrés et stockage constant — 6 octobre 2026

**VALIDÉ dans le périmètre testé sur les trois chaînes de construction.**
`TesterReferencesCallbacksParametres` vérifie des callbacks retournant une
référence vers un callback `pointeur_fonction<entier32(entier32)>` stocké par
l'hôte, y compris les versions `constante`, `volatile` et `constante volatile`.

### Contrat et défaut corrigé

L'appel `lire(d)(41)` lit la fonction contenue dans l'emplacement retourné,
puis lui passe son argument. Une liaison ou prise d'adresse conserve cet
emplacement ; une copie conserve sa propre cible lorsque l'original est
remplacé. La mutation d'un tableau local de callbacks ne doit pas remplacer
le callback de l'hôte dont l'élément a été copié.

Le nouveau corpus de stockage constant a révélé un défaut commun au bootstrap
et au frontend : `lire(d) = autre` était accepté lorsque la référence et
`autre` portaient le même type de callback constant. Le diagnostic suivant,
sur `Absente` à la position 1:288 du corpus, était donc émis à la place du
refus de mutation. Les deux analyseurs excluaient tous les types d'adresse
de la constance de valeur, sans distinguer un callback d'un pointeur de données.

Le bootstrap protège désormais la valeur d'un callback constant de niveau zéro
dans les variables, retours par référence, champs, indexations et
déréférencements. Le frontend reconnaît également ce stockage constant avant
de contrôler la valeur affectée. Le diagnostic **71** et sa position sont
comparés au bootstrap corrigé, sans nouveau code ni changement de l'AST public.
La lecture et l'appel restent autorisés. Le backend machine n'est pas modifié.

Cette règle ne rend pas constant l'emplacement de `constante entier32*&` :
ce type désigne toujours un pointeur réaffectable vers des données constantes.
Les signatures et qualifications des callbacks restent exactes ; aucune
nouvelle conversion implicite ou explicite de qualification n'est introduite.
`volatile` ne fournit pas de synchronisation ni d'atomicité.

### Matrice

Les **22 corpus bilingues exécutés** couvrent appels imbriqués avec paramètre,
liaisons, remplacement direct ou via une fonction ordinaire, adresse du
stockage, indexation du pointeur vers ce stockage, copie indépendante,
structures et tableaux de callbacks, constructeur, champ par défaut,
qualifications et adresse du callback lecteur lui-même.

Chaque exécution contrôle le résultat 42, la cible exacte des deux emplacements
hôtes, le nombre de lectures, l'unique appel à la fonction cible et l'argument
effectivement reçu. Le pont ABI mesure 16 octets, alignement 8, champs aux
offsets 0 et 8. Les types et positions des paramètres, octets FR/EN,
reproductibilité des images, AST intact et capacités/tampons partiels restent
comparés. Les images n'ajoutent pas d'import statique, mais leur exécution
dépend explicitement du pont C++ et de son stockage valide.

Les **25 refus bilingues** ajoutent 50 comparaisons code/ligne/colonne :
liaisons ou signatures incompatibles, arité avant argument inconnu dans les
appels imbriqués, argument incompatible, perte de qualification lors de
la prise d'adresse, pointeur vers callback non appelable sans déréférencement,
tableaux et sous-tableaux non appelables, affectation de tableau entier,
et mutation de callback constant par retour, liaison, copie, champ,
déréférencement ou indexation. La matrice atteint **2 381 refus différentiels**,
contre 2 331 avant cette tranche.

`TesterStockageCallbacksConstants` ajoute huit refus unitaires au bootstrap,
dont le cas central FR/EN, ainsi que des compilations positives de lecture
d'un callback constant et de réaffectation FR/EN d'un pointeur vers des données
constantes. Les autres matrices restent exécutées, notamment les 20 corpus
de références de pointeurs de la tranche précédente.

### Validation

Les trois constructions et validations complètes réussissent :

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail
  --parallel 6`, puis `ctest --preset windows-release --output-on-failure` :
  **5/5 tests réussis** ;
- GNU/Linux, sous Ubuntu/WSL : `cmake --build --preset linux-release --target
  espace_travail --parallel 4`, puis `ctest --preset linux-release
  --output-on-failure` : **6/6 tests réussis**, intégration comprise ;
- Visual Studio 2026 natif sans CMake : construction Release/x64 de
  `GsPlusPlus.slnx`, puis de `VisualStudio/Validation.vcxproj` avec MSBuild :
  **construction et validation réussies**.

Conformité **20/20 par chaîne**, 22 nouveaux corpus bilingues exécutés et
**2 381 refus différentiels** réussis dans les trois matrices sémantiques.
Les trois en-têtes de version générés indiquent `0.27.0-alpha.10`.

Les trois images `Frontend.GsE` reconstruites sont identiques : **450 255 octets**,
SHA-256 `8B4929279E7ABDC1D08E51AEE0D40790D5F082051EC2B502CD28221839ED96AB`.
Chaque image est acceptée par `gseverifier` : GsE 1.0, 3 segments, 8 sections,
2 imports et 75 exports. L'objet sémantique contient 336 938 octets de code,
184 octets de données, 120 octets de données nulles, 297 symboles et
1 746 relocalisations ; le frontend lié résout 537 symboles et
2 617 relocalisations.

`ConteneursDynamiques.GsPP` reste inchangé, SHA-256
`4EB8F7384823BEB1D9E2C9B073F67487AFE5F4B3FC038C5387181C1E2C50EF67`.
Le contrôle de style et `git diff --check` réussissent.

Version alpha.10, formats 1.0 et ABI 1 conservés. Cette tranche reste locale,
non commitée et non poussée ; elle n'est pas incluse dans la CI du commit
`c58874b` ni dans les paquets alpha.10 publiés. Les retours par référence des
fonctions ordinaires Gs++ et le backend auto-hébergé restent hors de cette tranche.

## Arguments référencés des callbacks imbriqués — 6 octobre 2026

**VALIDÉ dans le périmètre testé sur les trois chaînes de construction.**
`TesterArgumentsReferencesCallbacksImbriques` compose trois mécanismes :
référence vers un callback, paramètre passé par référence et retour de
référence depuis le callback appelé. Cette couverture ne requiert pas de
correction supplémentaire des analyseurs ni du backend.

### Contrat et matrice

Un callback `pointeur_fonction<entier32&(entier32&)>` reçoit l'adresse du
stockage de son argument et peut retourner cette même adresse. Dans
`lire(d)(d->Valeurs[0])`, le premier appel fournit le callback ; le second
reçoit l'adresse de l'élément. Sa lecture charge la valeur finale, tandis que
la liaison, prise d'adresse et affectation conservent le référent réel.

La constance du callback stocké ne rend pas constants ses paramètres : un
callback constant dont la signature accepte `entier32&` peut modifier cet
argument. Inversement, le callback de lecture `entier32(constante entier32&)`
accepte un référent constant sans le modifier. Une copie par valeur du callback
conserve également les paramètres et retours par référence de sa signature.
Les liaisons à un temporaire, agrégat temporaire ou référent incompatible
restent refusées ; aucune conversion supplémentaire n'est introduite.

Les **24 corpus bilingues exécutés** couvrent lectures et mutations,
liaisons et adresses du retour, arguments extraits d'un tableau ou d'un pointeur,
passage à une fonction ordinaire, constructeur et initialisation de base,
champ par défaut, agrégat, adresse et copie du callback, stockage de callback
constant, argument constant, doubles appels sur des référents distincts ou
identiques et courts-circuits `faux && ...` / `vrai || ...` sans appel.

Le pont C++ utilise de véritables paramètres et retours par référence. Chaque
cas vérifie le résultat 42, les valeurs finales, les adresses exactes reçues
par l'hôte, les valeurs avant/après chaque appel, l'ordre des mutations ou
lectures, le nombre exact d'appels et les cibles des pointeurs/callbacks
restées intactes. Le pont ABI mesure 32 octets, alignement 8, quatre champs
aux offsets 0, 8, 16 et 24. Les empreintes et positions de tous les paramètres,
octets FR/EN, images reproductibles, capacités/tampons partiels et AST intact
restent contrôlés. Aucun import statique n'est ajouté aux images de test,
mais elles utilisent explicitement les callbacks et le stockage de l'hôte.

Les **20 refus bilingues** ajoutent 40 comparaisons code/ligne/colonne :
paramètres référencés recevant littéraux, calculs, agrégats temporaires,
référents constants ou types incompatibles ; mauvaise arité avant argument
inconnu ; indice booléen ; résultat par valeur passé comme référence ; retour
par valeur lié à une référence ; signature de retour incompatible ; affectation
incompatible sur le retour référencé ; priorités internes des champs par défaut,
bases et délégations. La matrice atteint **2 421 refus différentiels**, contre
2 381 avant cette tranche.

### Validation

Les trois constructions et validations complètes réussissent :

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail
  --parallel 6`, puis `ctest --preset windows-release --output-on-failure` :
  **5/5 tests réussis** ;
- GNU/Linux, sous Ubuntu/WSL : `cmake --build --preset linux-release --target
  espace_travail --parallel 4`, puis `ctest --preset linux-release
  --output-on-failure` : **6/6 tests réussis**, intégration comprise ;
- Visual Studio 2026 natif sans CMake : construction Release/x64 de
  `GsPlusPlus.slnx`, puis de `VisualStudio/Validation.vcxproj` avec MSBuild :
  **construction et validation réussies**.

Conformité **20/20 par chaîne**, 24 nouveaux corpus bilingues exécutés et
**2 421 refus différentiels** réussis dans les trois matrices sémantiques.
Les trois en-têtes de version générés indiquent `0.27.0-alpha.10`.

Les trois `Frontend.GsE` des constructions sont identiques et inchangés par
rapport à la tranche précédente : **450 255 octets**, SHA-256
`8B4929279E7ABDC1D08E51AEE0D40790D5F082051EC2B502CD28221839ED96AB`.
Chaque image est acceptée par `gseverifier` : GsE 1.0, 3 segments, 8 sections,
2 imports et 75 exports. Ce lot ajoute des tests et documentation, pas de
modification des sources du frontend ou du backend.

`ConteneursDynamiques.GsPP` reste inchangé, SHA-256
`4EB8F7384823BEB1D9E2C9B073F67487AFE5F4B3FC038C5387181C1E2C50EF67`.
Le contrôle de style et `git diff --check` réussissent.

Version alpha.10, formats 1.0, ABI 1, AST public et diagnostics conservés.
Cette tranche est locale, non commitée et non poussée ; les résultats ne sont
pas inclus dans la CI de `c58874b` ni dans les paquets alpha.10 publiés.
Les fonctions ordinaires Gs++ retournant une référence restent refusées ; le
stockage de l'hôte doit rester valide et aucun backend auto-hébergé n'est revendiqué.

## Références de structures et de pointeurs dans les callbacks imbriqués — 6 octobre 2026

### Contrats et couverture

`TesterReferencesStructuresPointeursCallbacksImbriques` étend les paramètres
et retours référencés aux agrégats et aux emplacements de pointeurs. Les
**24 corpus valides bilingues**, soit 48 exécutions natives, couvrent :

- `P&(P&)` : adresse réelle de l'agrégat, liaison et prise d'adresse du retour,
  mutation d'un champ ou élément, copie indépendante, argument déréférencé,
  deux agrégats distincts et deux mutations successives du même agrégat ;
- `constante P&(constante P&)` : lecture sans mutation, liaison et adresse
  qualifiées, éléments qualifiés et copie par valeur indépendante ;
- `entier32*&(entier32*&, entier32*)` : adresse réelle de l'emplacement du
  pointeur, changement de cible, liaison ou adresse du retour, mutation de
  la nouvelle cible et transmission à une fonction Gs++ prenant une référence ;
- la variante de pointeur vers `constante entier32` : redirection autorisée,
  retour du même emplacement, adresse doublement indirecte et protection de
  la donnée pointée ;
- constructions locales, champs par défaut et base recevant un agrégat
  référencé, ordre exact des événements et nombre d'évaluations des callbacks.

Le pont C++ utilise une vraie structure native, et non une vue obtenue par
réinterprétation d'un tableau : taille, alignement et décalages de chaque
champ de `P` et du stockage `Z` sont comparés au programme Gs++. Les signatures
complètes des paramètres sont également comparées aux symboles auto-hébergés.
Chaque exécution vérifie les adresses, valeurs avant/après, champs non modifiés,
cibles de pointeurs et cibles de callbacks. Les images produites sont
reproductibles ; leurs octets de code et données sont identiques en français
et en anglais. Leur génération machine utilise toujours le backend C++.

Les **24 corpus refusés bilingues** ajoutent 48 comparaisons, portant la
matrice locale à **2 469 refus différentiels**. Ils contrôlent diagnostic,
ligne, colonne et AST intact : agrégat temporaire, référent constant utilisé
comme mutable, autre structure de même disposition, champ inconnu, types et
qualifications de pointeurs incompatibles, mauvaise arité, liaison invalide,
mutation constante et type d'affectation incorrect. Les priorités des appels
imbriqués, champs par défaut et bases sont également vérifiées.

### Validation

Les validations complètes des trois chaînes réussissent :

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail
  --parallel 6`, puis `ctest --preset windows-release --output-on-failure` :
  **5/5 tests réussis** ;
- GNU/Linux sous Ubuntu/WSL : `cmake --build --preset linux-release --target
  espace_travail --parallel 4`, puis `ctest --preset linux-release
  --output-on-failure` : **6/6 tests réussis**, intégration comprise ;
- Visual Studio 2026 natif sans CMake : construction Release/x64 de
  `GsPlusPlus.slnx`, puis de `VisualStudio/Validation.vcxproj` avec MSBuild :
  **construction et validation réussies**.

Conformité **20/20 par chaîne**, 24 nouveaux corpus bilingues exécutés et
**2 469 refus différentiels** réussis dans les trois matrices sémantiques.
Les trois versions générées indiquent `0.27.0-alpha.10`.

Les trois `Frontend.GsE` sont identiques et inchangés : **450 255 octets**,
SHA-256 `8B4929279E7ABDC1D08E51AEE0D40790D5F082051EC2B502CD28221839ED96AB`.
Chaque image est acceptée par `gseverifier` : GsE 1.0, 3 segments, 8 sections,
2 imports et 75 exports. `ConteneursDynamiques.GsPP` reste inchangé,
SHA-256 `4EB8F7384823BEB1D9E2C9B073F67487AFE5F4B3FC038C5387181C1E2C50EF67`.
Le contrôle de style et `git diff --check` réussissent.

Aucune nouvelle correction des analyseurs ou du backend n'est nécessaire pour
ces cas : ce lot consolide la couverture de comportements déjà implémentés.
Version alpha.10, formats 1.0, ABI 1, AST public et diagnostics conservés.
La tranche reste locale, non commitée et non poussée ; elle n'est pas incluse
dans la CI de `c58874b` ni dans les paquets alpha.10 publiés. Les callbacks et
le stockage fournis par l'hôte doivent rester valides ; aucun retour référencé
de fonction ordinaire Gs++ ni backend auto-hébergé supplémentaire n'est revendiqué.

## Références des groupes mixtes et constructions — 6 octobre 2026

### Contrats et couverture

`TesterReferencesGroupesMixtesConstructions` compare les groupes réunissant
une méthode et une fonction libre de même nom qualifié. Les **22 corpus
valides bilingues**, soit 44 exécutions natives, vérifient :

- références scalaires mutables et constantes, temporaire accepté par une
  surcharge par valeur et exclusion d'une conversion numérique implicite
  non prise en charge pour un argument variable ;
- références de pointeurs mutables ou vers des données constantes, pointeur
  qualifié par conversion explicite et alias du type du récepteur ;
- correspondance exacte avec une classe dérivée contre conversion vers sa
  base, références de base constantes et disposition avec table virtuelle ;
- récepteur constant excluant la méthode mutable, récepteur volatile,
  appels par point, flèche et nom qualifié ;
- champs par défaut, champs explicitement initialisés, constructions de bases
  et membres, ainsi que délégation à un autre constructeur.

Pour chaque appel mixte, la déclaration choisie, sa position, son type de
retour et son statut méthode/fonction sont comparés au bootstrap C++. Le
comparateur parcourt les corps et les expressions des constructions, vérifie
le nombre exact de sélections et refuse les sélections manquantes, dupliquées
ou absentes du bootstrap. L'exécution vérifie la surcharge réellement appelée
et les mutations des référents. Les octets de code et données sont identiques
en français et en anglais ; les images GsE sont reproductibles et n'ajoutent
aucun import d'hôte. Leur génération machine reste effectuée par le backend C++.

Les **24 corpus refusés bilingues** ajoutent 48 comparaisons et portent la
matrice locale à **2 517 refus différentiels**, avec diagnostic, ligne,
colonne et AST intact contrôlés. Ils couvrent égalités de score, référent
constant ou temporaire incompatible, largeur ou niveau de pointeur incorrect,
qualification retirée lors d'une conversion vers une base et accès privé.
Les erreurs d'appel dans les champs par défaut, bases, membres et délégations
sont comparées aux erreurs suivantes du corps ou des champs ; une arité
incompatible reste prioritaire sur les arguments non visités dans ces cas.

Cette couverture confirme les règles actuelles, pas celles du C++ : une
référence et une valeur scalaire de même type peuvent être à égalité ; une
référence mutable et une référence constante compatibles avec le même argument
mutable peuvent également rendre l'appel ambigu. Aucune préférence automatique
pour les méthodes ou les références n'est introduite.

### Validation

Les validations complètes des trois chaînes réussissent :

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail
  --parallel 6`, puis `ctest --preset windows-release --output-on-failure` :
  **5/5 tests réussis** ;
- GNU/Linux sous Ubuntu/WSL : `cmake --build --preset linux-release --target
  espace_travail --parallel 4`, puis `ctest --preset linux-release
  --output-on-failure` : **6/6 tests réussis**, intégration comprise ;
- Visual Studio 2026 natif sans CMake : construction Release/x64 de
  `GsPlusPlus.slnx`, puis de `VisualStudio/Validation.vcxproj` avec MSBuild :
  **construction et validation réussies**.

Conformité **20/20 par chaîne**, 22 nouveaux corpus bilingues exécutés et
**2 517 refus différentiels** réussis dans les trois matrices sémantiques.
Les trois versions générées indiquent `0.27.0-alpha.10`.

Les trois `Frontend.GsE` sont identiques et inchangés : **450 255 octets**,
SHA-256 `8B4929279E7ABDC1D08E51AEE0D40790D5F082051EC2B502CD28221839ED96AB`.
Chaque image est acceptée par `gseverifier` : GsE 1.0, 3 segments, 8 sections,
2 imports et 75 exports. `ConteneursDynamiques.GsPP` reste inchangé,
SHA-256 `4EB8F7384823BEB1D9E2C9B073F67487AFE5F4B3FC038C5387181C1E2C50EF67`.
Le contrôle de style et `git diff --check` réussissent.

Aucune nouvelle correction des analyseurs ou du backend n'est nécessaire pour
ces cas. Version alpha.10, formats 1.0, ABI 1, AST public et diagnostics conservés.
La tranche reste locale, non commitée et non poussée ; ses résultats ne sont
pas inclus dans la CI de `c58874b` ni dans les paquets alpha.10 publiés. Elle
ne constitue pas une validation exhaustive des conversions, du frontend 0.27
complet ou d'un backend auto-hébergé.

## Références des opérateurs mixtes et constructions — 6 octobre 2026

### Contrats et couverture

`TesterReferencesOperateursMixtesConstructions` étend la couverture des
groupes mixtes aux opérateurs. Les **24 corpus valides bilingues**, soit
48 exécutions natives, vérifient :

- paramètres scalaires mutables et constants, éléments de tableaux et
  scalaires déréférencés ; les mutations restent visibles dans le stockage original ;
- références d'emplacements de pointeurs : changement de cible, identité de
  la nouvelle cible, conservation de l'ancienne donnée et variante vers une
  donnée constante ;
- correspondance exacte avec une classe dérivée contre conversion vers sa base,
  récepteurs constants ou volatiles, opérateurs binaires et unaires ;
- appels imbriqués et successifs, champs par défaut, champs explicitement
  initialisés, bases, membres et délégation entre constructeurs ;
- opérateurs correctement analysés mais non exécutés dans les opérandes
  ignorés de `&&` et `||` intégrés.

Chaque sélection auto-hébergée est comparée à sa déclaration C++ : position,
type de retour, statut méthode/fonction et nombre exact de sélections. Le
comparateur parcourt les expressions imbriquées et les contextes de construction,
et refuse une résolution omise, dupliquée ou absente du bootstrap.

Une globale publique `Trace`, lue directement dans l'image chargée après
exécution, sépare le typage de l'exécution. Elle contrôle la surcharge appelée,
le nombre d'appels et leur ordre. Les deux corpus à appels successifs ou
imbriqués enregistrent également leurs arguments distincts : une inversion
change la trace. Les courts-circuits gardent une trace nulle et le référent
inchangé, tout en conservant une résolution sémantique de l'opérateur ignoré.

Les octets de code et données sont identiques en français et en anglais ;
les images GsE sont reproductibles et n'ajoutent aucun import d'hôte. La
génération machine reste effectuée par le backend C++.

Les **24 corpus refusés bilingues** ajoutent 48 comparaisons et portent la
matrice locale à **2 565 refus différentiels**, avec diagnostic, ligne,
colonne et AST intact contrôlés. Les égalités de score, référents constants
ou temporaires incompatibles, niveaux et qualifications de pointeurs et accès
privés sont couverts. Les erreurs des expressions imbriquées sont confrontées
à un nom inconnu avant ou après l'opérateur ; les erreurs des constructions
sont confrontées à celles des corps. Les cibles d'affectation constantes ou
de tableau restent prioritaires sur l'opérateur invalide de la valeur affectée.
Un opérande ignoré à l'exécution reste refusé s'il est sémantiquement invalide.

### Validation

Les validations complètes des trois chaînes réussissent, avec les traces renforcées :

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail
  --parallel 6`, puis `ctest --preset windows-release --output-on-failure` :
  **5/5 tests réussis** ;
- GNU/Linux sous Ubuntu/WSL : `cmake --build --preset linux-release --target
  espace_travail --parallel 4`, puis `ctest --preset linux-release
  --output-on-failure` : **6/6 tests réussis**, intégration comprise ;
- Visual Studio 2026 natif sans CMake : construction Release/x64 de
  `GsPlusPlus.slnx`, puis de `VisualStudio/Validation.vcxproj` avec MSBuild :
  **construction et validation réussies**.

Conformité **20/20 par chaîne**, 24 nouveaux corpus bilingues exécutés et
**2 565 refus différentiels** réussis dans les trois matrices sémantiques.
Les trois versions générées indiquent `0.27.0-alpha.10`.

Les trois `Frontend.GsE` sont identiques et inchangés : **450 255 octets**,
SHA-256 `8B4929279E7ABDC1D08E51AEE0D40790D5F082051EC2B502CD28221839ED96AB`.
Chaque image est acceptée par `gseverifier` : GsE 1.0, 3 segments, 8 sections,
2 imports et 75 exports. `ConteneursDynamiques.GsPP` reste inchangé,
SHA-256 `4EB8F7384823BEB1D9E2C9B073F67487AFE5F4B3FC038C5387181C1E2C50EF67`.
Le contrôle de style et `git diff --check` réussissent.

Aucune nouvelle correction des analyseurs ou du backend n'est nécessaire pour
ces cas. Version alpha.10, formats 1.0, ABI 1, AST public et diagnostics conservés.
La tranche reste locale, non commitée et non poussée ; ses résultats ne sont
pas inclus dans la CI de `c58874b` ni dans les paquets alpha.10 publiés. Elle
ne constitue pas une validation exhaustive des opérateurs logiques surchargés,
du frontend 0.27 complet ou d'un backend auto-hébergé.

## Opérateurs des initialiseurs agrégés — 6 octobre 2026

Cette tranche compose les opérateurs mixtes référencés avec les valeurs entre
accolades. Elle vérifie l'analyseur Gs++ et l'exécution des images générées par
le backend C++ ; elle ne constitue pas une migration du backend.

### Corrections des affectations et retours

Deux écarts sont reproduits contre le bootstrap : dans une affectation ou un
retour, un agrégat de structure trop grand contenant aussi un opérateur invalide
signalait d'abord l'opérateur dans l'analyseur Gs++, au lieu de l'excès d'éléments.
Dans les reproductions, le bootstrap signale le code **43** en colonne **457**,
contre le code **21** en colonne **464** avant correction, ligne 1.

`ResoudreExpressionsSemantiques` reconnaît désormais ces agrégats et réutilise
le validateur contextuel existant : il vérifie leur forme avant de résoudre
leurs feuilles dans l'ordre, avec le type destination. Les descendants déjà
résolus ne sont pas reparcourus par la visite générique. La cible d'affectation
reste contrôlée avant sa valeur ; les conversions implicites et résolutions
d'opérateurs restent publiées par les mécanismes existants.

Aucun nouveau genre AST, diagnostic public ou contrat ABI n'est ajouté.
Le bootstrap C++ n'est pas modifié par cette tranche.

### Corpus et stockage vérifiés

`TesterOperateursInitialiseursAgreges` ajoute **27 corpus valides**, chacun
analysé et exécuté en français et en anglais :

- scalaires entre accolades imbriquées, structures, unions, tableaux fixes
  et multidimensionnels, tableaux de structures et structures contenant des tableaux ;
- alignements de champs, éléments omis à zéro et copies indépendantes ;
- affectations, arguments directs et de callbacks, retours de structures
  par valeur de 8 et 16 octets ;
- champs par défaut et explicites, bases, membres et délégations ;
- références constantes sélectionnant la fonction libre compatible,
  court-circuit logique intégré sans exécuter l'opérateur ignoré ;
- lectures après mutation et capture de deux valeurs depuis le même référent :
  le premier champ reste à 41, le deuxième reçoit 42, sans réécriture du premier ;
- pointeurs retournés placés dans des champs mutables ou constants, avec
  redirection des emplacements reçus sans altérer les anciennes données pointées.

Chaque programme contrôle les champs ou éléments et les référents, pas seulement
leur somme. La trace exportée vérifie l'ordre et le nombre d'appels. Le contrôle
partagé `VerifierOperateursReferencesBilingues` parcourt désormais les agrégats ;
il conserve également les 24 corpus de la tranche précédente. Déclarations
sélectionnées, types de retour, drapeaux de méthode, octets bilingues et
reproductibilité des images sont comparés ; les programmes de test n'ajoutent
aucun import d'hôte.

**42 refus bilingues**, soit **84 nouveaux refus**, portent la matrice locale
de **2 565 à 2 649**. Ils couvrent les priorités de forme, de type et d'opérateur
entre éléments, les références, l'affectation constante, les constantes hors
plage, les champs et constructions, l'ambiguïté et l'arité des appels, ainsi que
les affectations via pointeur ou membre et les retours scalaires, d'unions ou de
structures contenant des tableaux. Code, ligne, colonne et AST intact sont vérifiés.

### Validation

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail
  --parallel 6`, puis `ctest --preset windows-release --output-on-failure` :
  **5/5 tests réussis** ; le test différentiel ciblé a également réussi ;
- GNU/Linux sous Ubuntu/WSL : `cmake --build --preset linux-release --target
  espace_travail --parallel 4`, puis `ctest --preset linux-release
  --output-on-failure` : **6/6 tests réussis**, intégration comprise ;
- Visual Studio 2026 natif sans CMake : construction Release/x64 de
  `GsPlusPlus.slnx`, puis de `VisualStudio/Validation.vcxproj` avec MSBuild :
  **construction et validation réussies**.

Conformité **20/20 par chaîne** et **2 649 refus différentiels** réussis dans
les trois matrices sémantiques. Les trois versions générées indiquent
`0.27.0-alpha.10`.

Les trois `Frontend.GsE` reconstruits sont identiques : **451 599 octets**,
SHA-256 `AB0B4A84170ABB92FA23BFC59567FC4E7F33529619583EDBA3E6C4364F034CC7`.
Ils diffèrent de la tranche précédente car l'analyseur Gs++ est corrigé.
Chaque image est acceptée par `gseverifier` : GsE 1.0, 3 segments, 8 sections,
2 imports et 75 exports. `ConteneursDynamiques.GsPP` reste inchangé,
SHA-256 `4EB8F7384823BEB1D9E2C9B073F67487AFE5F4B3FC038C5387181C1E2C50EF67`.
Le contrôle de style et `git diff --check` réussissent.

Version alpha.10, formats 1.0, ABI 1 et contrats publics conservés. La tranche
reste locale, non commitée et non poussée ; ses résultats ne sont pas inclus
dans la CI de `c58874b` ni dans les paquets alpha.10 publiés. La couverture ne
constitue pas une validation exhaustive du frontend 0.27 ou du backend
auto-hébergé.

## Interfaces préparées en mémoire — 6 octobre 2026

### Entrée réelle d'analyse d'interface

`AnalyserDeclarationsInterface` et son alias anglais
`AnalyzeInterfaceDeclarations` sont maintenant exportés par `Frontend.GsE`,
dans `GalacticShrine::GsPP::Autohebergement`. Ils partagent le parseur et la
requête existante avec `AnalyserDeclarationsSource`, sans recopier le code ni
garder de mode global entre les appels.

La requête reste de 80 octets et les nœuds de 64 octets. L'interrogation de
capacité, l'écriture du préfixe dans un tampon trop petit et le remplissage
exact conservent leur protocole. Les structures publiques et anciens exports
ne sont pas modifiés ; les deux nouveaux noms forment une extension additive.

Le mode interface s'applique à tout le texte UTF-8 fourni :

- fonctions et globales externes implicites, sans export public de définition ;
- membres de classes externes, avec leur visibilité, qualifications et signatures ;
- types, énumérations, champs, alias et utilisations d'espaces analysés normalement ;
- refus des corps, globales initialisées et listes d'initialisation de
  constructeurs externes ; refus d'une liste sur une fonction libre avec le
  diagnostic du bootstrap, aussi en mode source.

Les tests passent l'AST réellement produit par cette entrée à l'analyseur
sémantique Gs++, sans le reconstruire ou en corriger les drapeaux côté hôte.
Les anciens tests de contextes mixtes conservent leurs preuves historiques.

### Couverture et limites

`TesterInterfacesEnMemoire` vérifie **22 corpus bilingues syntaxiques et
sémantiques** : signatures scalaires, pointeurs et références, structures et
unions, énumérations, callbacks et retours référencés de callbacks, membres
privés/protégés/publics, constructeurs et destructeurs, opérateurs libres et
membres, héritage virtuel/remplacement, surcharges, alias, utilisations et noms
qualifiés. Un corpus contrôle BOM UTF-8, noms accentués, CRLF et plusieurs lignes.

**6 interfaces bilingues de types ou données** sont analysées syntaxiquement,
dont une structure et une énumération seules dans leur propre contenu.
L'analyse sémantique actuelle exige toujours au moins une fonction, comme
le bootstrap : ces interfaces doivent être assemblées avec leurs consommateurs
avant cette passe. Cette tranche ne supprime pas cette contrainte.

**14 refus syntaxiques bilingues** comparent code, ligne et colonne. **15 refus
sémantiques bilingues**, soit **30 nouveaux refus**, portent la matrice de
**2 649 à 2 679**. Ils contrôlent types inconnus, signatures interdites, limites
d'arité, tableaux paramètres, globales `vide`, alias, cycles par valeur,
héritage, remplacements et arité d'opérateurs. Les champs par défaut requièrent
toujours un constructeur défini : son seul prototype ne suffit pas.

Les formes françaises/anglaises produisent la même structure d'AST. Les capacités
exactes et partielles sont protégées par sentinelles, le préfixe est comparé,
l'AST reste intact après validation ou refus sémantique, et les arènes sont
libérées. Requêtes nulles ou incohérentes, interface vide et alternance
source/interface sont aussi contrôlées. Les deux alias d'export renvoient
exactement la même adresse.

**Le contenu doit déjà être préparé.** Cette API ne lit pas de fichier et ne
traite pas `#inclure` / `#include` ou `#pragma once`. Elle n'assemble pas les
interfaces avec les sources et ne normalise pas les prototypes contre leurs
définitions. L'AST compact ne porte pas d'identité de fichier ; les positions
sont relatives au texte fourni. L'expansion et les origines restent assurées
par le bootstrap hôte dans `gsppc`. Un flux mixte de jetons avec modes et
origines, ainsi que l'assemblage des unités, restent à raccorder au frontend.

### Validation

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail
  --parallel 6`, puis `ctest --preset windows-release --output-on-failure` :
  **5/5 tests réussis** ; le test différentiel ciblé a également réussi ;
- GNU/Linux sous Ubuntu/WSL : `cmake --build --preset linux-release --target
  espace_travail --parallel 4`, puis `ctest --preset linux-release
  --output-on-failure` : **6/6 tests réussis**, intégration comprise ;
- Visual Studio 2026 natif sans CMake : construction Release/x64 de
  `GsPlusPlus.slnx`, puis de `VisualStudio/Validation.vcxproj` avec MSBuild :
  **construction et validation réussies**.

Conformité **20/20 par chaîne** et **2 679 refus différentiels** réussis dans
les trois matrices sémantiques. Les versions générées indiquent
`0.27.0-alpha.10`. Style et `git diff --check` réussissent.

Les trois `Frontend.GsE` sont identiques : **452 815 octets**, SHA-256
`031577A2703317CC89FC3983898B61F727B1F6FE0AF53A2F8B243C2E1902A3CD`.
Chaque image est acceptée par `gseverifier` : GsE 1.0, 3 segments, 8 sections,
2 imports d'allocation/libération et **77 exports**, contre 75 précédemment.
L'entrée mémoire n'ajoute aucun import de fichier.
`ConteneursDynamiques.GsPP` reste inchangé, SHA-256
`4EB8F7384823BEB1D9E2C9B073F67487AFE5F4B3FC038C5387181C1E2C50EF67`.

Formats 1.0, ABI 1, alpha.10 et structures publiques conservés. Le bootstrap
et le backend C++ ne sont pas modifiés par cette tranche. Les changements
restent locaux, non commités et non poussés ; ils ne sont pas inclus dans
la CI de `c58874b` ou les paquets alpha.10 publiés. Le frontend 0.27 complet
et son alimentation autonome à partir des fichiers ne sont pas déclarés validés.

## Assemblage préparé et origines des unités — 6 octobre 2026

### Contrat mémoire et frontières de compilation

`AssemblerDeclarationsPreparees`, également exporté sous
`AssemblePreparedDeclarations`, reçoit une liste ordonnée de
`UniteDeclarationsPreparee` : texte UTF-8, taille, mode source/interface
numérique 0/1 et champ réservé nul. Chaque unité est analysée séparément par
le parseur Gs++, avec son mode propre. Aucune lecture de fichier n'est ajoutée.

La nouvelle `RequeteAssemblageDeclarations` de **128 octets** contient trois
sorties appartenant à l'appelant : texte assemblé, AST et table d'origines.
L'interrogation avec des tampons nuls retourne les trois tailles exactes et
`CapaciteInsuffisante` (1). Toutes les unités sont validées avant ce résultat :
un défaut syntaxique reste prioritaire sur une capacité insuffisante.
Contrairement au préfixe permis par l'analyse d'une seule unité, **aucune des
trois sorties n'est publiée en cas d'erreur ou de capacité insuffisante**.
Les tampons doivent être distincts et ne pas recouvrir les entrées.

Le texte conserve tous les octets, sauf le BOM UTF-8 initial de chaque unité.
Un LF est ajouté après chaque unité, y compris la dernière et les unités
vides ; il empêche notamment un commentaire `//` final d'absorber le début
de l'unité suivante. Aucun terminateur nul n'est écrit. L'AST possède une
seule racine ; les parents, positions de lignes et tranches des noms sont
rebasés sur ce texte, sans modifier les entrées ni les drapeaux d'interface.

Chaque `OrigineUniteDeclarations` de **32 octets** indique le début et la
taille du contenu hors BOM et LF ajouté, sa première ligne synthétique, son
nombre de lignes (`1 + nombre de LF`), la taille du BOM retiré et un champ
réservé nul. Son rang est l'identité de l'unité ; le chemin reste une
métadonnée de l'hôte. Les positions syntaxiques d'erreur sont déjà locales
à l'unité, avec son rang, le code d'analyse des déclarations et le détail
lexical éventuel. Hors diagnostic localisé, `IndexUniteErreur` vaut le nombre
d'unités. La structure de résultat fait **64 octets**.

Les contrôles rejettent les pointeurs/capacités incohérents, modes ou champs
réservés invalides, plus d'un million d'unités et une taille brute cumulée
supérieure à un milliard d'octets, séparateurs compris, avant de lire les
textes. Le nombre de nœuds est également borné. Les refus d'allocation
libèrent toutes les arènes sans publication partielle. `NombreOctetsArene`
décrit le stockage de travail de l'assembleur, pas les arènes temporaires
successives des analyses individuelles.

### Sémantique par unité et diagnostics locaux

`AnalyserSemantiqueUnites` / `AnalyzeUnitSemantics` ajoute une requête de
**40 octets**, qui référence la `RequeteAnalyseSemantique` existante et la
table d'origines. Elle valide la couverture exacte du texte, les bornes,
LF séparateurs, nombres de lignes et champs réservés. Les anciens contrats
de 80 octets pour les déclarations, 120 pour la sémantique et 64 pour les
nœuds restent inchangés.

La recherche des noms importés limite les `utilisant espace` directs et
transitifs à leur **unité de compilation**, comme le bootstrap. Un import
présent dans une unité d'interface séparée ne devient pas actif dans une
autre source. Cela ne préjuge pas du futur flux d'une inclusion textuelle,
qui appartiendra à l'unité de son consommateur. Les déclarations de types,
fonctions et globales gardent leur visibilité habituelle dans le programme.

Le résultat de l'analyse référencée conserve ligne/colonne synthétiques ;
la nouvelle requête fournit aussi rang de l'unité, ligne et colonne locales.
En l'absence de diagnostic localisé, le rang vaut le nombre d'origines et
les positions locales sont nulles. L'AST, le texte et les origines restent
intacts après la passe. L'ancienne entrée `AnalyserSemantique` conserve son
comportement pour un texte unique ; il faut utiliser la nouvelle entrée
pour un assemblage multi-unité.

### Couverture et limites

`TesterAssemblageDeclarationsPreparees` passe les AST réellement produits
en Gs++ à la nouvelle entrée sémantique, après comparaison des analyses
individuelles avec le bootstrap C++. **13 corpus bilingues valides** couvrent
types et énumérations séparés, unions, alias, classes, références,
callbacks, plusieurs interfaces/sources, alternance des modes, espaces,
imports transitifs locaux, commentaires de fin, BOM multiples, noms accentués,
CRLF et unités vides. **12 refus sémantiques bilingues**, soit **24 nouveaux
refus**, comparent code et origine complète au bootstrap, dont quatre
régressions d'isolation des imports. La matrice passe de **2 679 à 2 703**.

**6 refus syntaxiques bilingues** contrôlent le diagnostic original sans
sortie partielle. Sentinelles, chacune des trois capacités insuffisantes,
échec injecté à chaque allocation, déterminisme, absence de fuite, tables
d'origines altérées, source tronquée, séparateur absent, tailles excessives
et requêtes incohérentes sont testés. Un corpus prototype/définition bilingue
supplémentaire vérifie explicitement que les deux déclarations restent présentes.

**L'assemblage est syntaxique, sans normalisation des déclarations.** La
fusion des prototypes avec leurs définitions, le traitement des répétitions
compatibles et la priorité des incompatibilités/doubles définitions restent
à implémenter ; un assemblage qui les contient n'est donc pas encore une
entrée sémantique générale exploitable. Lecture, expansion de `#inclure` /
`#include`, `#pragma once` et origines internes aux inclusions restent du
côté hôte. Le bootstrap et le backend C++ ne sont pas modifiés par cette tranche.

### Validation

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail
  --parallel 6`, puis `ctest --preset windows-release --output-on-failure` :
  **5/5 tests réussis** ; le premier test différentiel ciblé a aussi réussi ;
- GNU/Linux sous Ubuntu/WSL : `cmake --build --preset linux-release --target
  espace_travail --parallel 4`, puis `ctest --preset linux-release
  --output-on-failure` : **6/6 tests réussis**, intégration comprise ;
- Visual Studio 2026 sans CMake : construction Release/x64 de
  `GsPlusPlus.slnx`, puis `VisualStudio/Validation.vcxproj` : **réussies**.

Conformité **20/20 par chaîne**, **2 703 refus différentiels** dans chacune
des trois matrices. Les versions générées indiquent `0.27.0-alpha.10`.
Style et `git diff --check` réussissent.

Les trois `Frontend.GsE` sont identiques : **466 447 octets**, SHA-256
`CECDC9583A9DB2C49E4A13D7AA0B183871060BC72B7B40499468198A6FDBE0AE`.
Chaque image est acceptée par `gseverifier` : GsE 1.0, 3 segments, 8 sections,
2 imports d'allocation/libération et **81 exports**, contre 77 précédemment.
Les quatre noms ajoutés sont les deux nouvelles entrées et leurs alias anglais.
`ConteneursDynamiques.GsPP` reste inchangé, SHA-256
`4EB8F7384823BEB1D9E2C9B073F67487AFE5F4B3FC038C5387181C1E2C50EF67`.

Formats 1.0, ABI 1, alpha.10 et structures publiques existantes conservés.
Les changements restent locaux, non commités et non poussés ; ils ne sont
inclus ni dans la CI de `c58874b` ni dans les paquets alpha.10 publiés.
Le frontend 0.27 complet et la compilation autonome à partir des fichiers
ne sont pas déclarés validés.

## Normalisation préparée des déclarations libres — 6 octobre 2026

### Entrée additive et règles de sélection

`AssemblerDeclarationsNormalisees` / `AssembleNormalizedDeclarations` réutilise
la requête d'assemblage de **128 octets** et ajoute une entrée distincte de
l'assemblage brut. `AssemblerDeclarationsPreparees` conserve son comportement,
notamment la présence simultanée d'un prototype et de sa définition.
L'implémentation Gs++ se trouve dans
`AutoHebergement/AnalyseurDeclarations/NormalisationDeclarations.GsPP` ;
les constructions CMake et Visual Studio 2026 native l'intègrent explicitement.

Cette première normalisation couvre les **fonctions libres, opérateurs libres,
globales et alias racines**, dans les espaces qualifiés et imbriqués. Elle
reprend les règles du bootstrap `NormaliserDeclarations` dans ce périmètre :

- toutes les unités sont analysées avant la normalisation : une erreur
  syntaxique dans une unité ultérieure reste prioritaire ;
- fonctions libres d'abord, globales ensuite, alias enfin ; les conflits
  de chaque famille sont visités dans l'ordre des déclarations d'entrée ;
- clé d'une fonction : nom source complet et types de paramètres, avant
  résolution des alias ; le nom des paramètres ne participe pas à la clé ;
- un retour différent pour une même clé est incompatible ; deux définitions
  sont refusées, même sans corps utile ;
- les déclarations externes compatibles sont réunies ; une définition
  remplace un prototype à la **place de la première déclaration** ; une
  déclaration externe ultérieure ne remplace pas la définition ;
- les globales de même nom doivent avoir le même type et au plus une
  définition ; l'absence d'initialiseur ne rend pas une globale externe ;
- les alias de même nom et cible nominale exacte sont réunis ; deux cibles
  différentes restent incompatibles, même si une résolution future pourrait
  les rendre équivalentes.

Les noms qualifiés sont comparés composante par composante, sans les espaces
de séparation. Les types sont encodés structurellement : mots-clés bilingues,
qualifications indépendantes de leur ordre/répétition, noms nominaux exacts,
indirections, références, dimensions numériques et callbacks récursifs.
**Un hachage égal ne suffit jamais à fusionner deux déclarations.** Les
régressions existantes de collisions de signatures nominales et de callbacks
doivent donc atteindre la passe sémantique, et non devenir des doubles
définitions artificielles au stade de la normalisation.

Le texte assemblé et la table d'origines restent inchangés ; les nœuds de la
déclaration choisie conservent leur position et leurs noms d'origine. Les
sous-arbres sont recopiés intégralement avec leurs parents rebasés. Cette
sélection conserve notamment l'ordre des fonctions pour leurs diagnostics
de corps, même lorsque les définitions apparaissent dans l'ordre inverse
des prototypes.

### Diagnostics, capacités et sécurité mémoire

Les codes d'assemblage 0 à 5 sont conservés. Les nouveaux codes sont :

| Code | Diagnostic de normalisation |
|---|---|
| 6 | Fonction incompatible pour la même clé de surcharge |
| 7 | Fonction définie plusieurs fois |
| 8 | Globale incompatible |
| 9 | Globale définie plusieurs fois |
| 10 | Alias incompatible |

L'erreur désigne l'unité et la position locale de la déclaration fautive,
comme le bootstrap. Aucune sortie n'est publiée en cas de diagnostic.
Les trois tailles de sortie ne sont définitives qu'après réussite de la
normalisation : la capacité de l'AST est celle de l'AST normalisé, pas de
l'assemblage brut. Une interrogation valide retourne 1 avec ces besoins
exacts ; les erreurs de normalisation sont prioritaires sur ce résultat.

Les entrées et sorties doivent être distinctes. Les trois sorties restent
intactes en cas de capacité insuffisante ou de refus d'allocation, y compris
pendant l'assemblage brut interne et la préparation des clés exactes.
`NombreOctetsArene` décrit les allocations de travail du normaliseur, sans
additionner les arènes temporaires des appels internes. Les anciens types
publics, formats 1.0 et ABI 1 ne sont pas modifiés.

### Preuves différentielles et limites

`TesterNormalisationDeclarationsPreparees` appelle le **normaliseur C++ réel**
pour choisir les déclarations, puis compare ces choix aux sous-arbres issus
des analyses Gs++ réelles. **22 corpus bilingues valides** couvrent les deux
ordres prototype/définition, externes répétés, surcharges, espaces équivalents,
types séparés, références, qualifications, callbacks imbriqués, retours de
callbacks, globales avec/sans initialiseur, tableaux, alias et opérateurs,
BOM/CRLF et maintien d'une classe avec méthode unique.

**15 conflits bilingues**, soit **30 refus différentiels de normalisation**,
contrôlent les cinq codes, leur origine et la priorité entre familles.
Un **refus syntaxique bilingue** contrôle la priorité de l'analyse de toutes
les unités. **8 refus sémantiques bilingues**, soit **16 nouveaux refus**,
vérifient ordre des corps après fusion, signatures interdites, alias de
paramètres non fusionnés, collisions nominales/callbacks, type absent et
isolation des imports. La matrice sémantique passe de **2 703 à 2 719** ;
les 30 refus de normalisation sont comptés séparément.

Les trois capacités partielles, tailles exactes, sentinelles, déterminisme,
chaque point d'échec d'allocation jusqu'au succès, libération des arènes,
requêtes invalides et unité vide sont testés. AST, texte et origines restent
intacts pendant la sémantique.

**Les membres de classes ne sont pas normalisés par cette entrée.** Les
types et énumérations ne sont pas réunis non plus ; les interactions de
priorité impliquant des membres et des déclarations libres restent hors
du périmètre validé. L'entrée est une étape préparée, non le remplacement
général de `NormaliserDeclarations` dans `gsppc`. Lecture, inclusions,
`#pragma once` et origines internes aux inclusions restent côté hôte.
Le bootstrap et le backend C++ sont inchangés par cette tranche.

### Validation

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail
  --parallel 6`, puis `ctest --preset windows-release --output-on-failure` :
  **5/5 tests réussis** ; le test différentiel ciblé a également réussi ;
- GNU/Linux sous Ubuntu/WSL : `cmake --build --preset linux-release --target
  espace_travail --parallel 4`, puis `ctest --preset linux-release
  --output-on-failure` : **6/6 tests réussis**, intégration comprise ;
- Visual Studio 2026 sans CMake : construction Release/x64 de
  `GsPlusPlus.slnx`, puis `VisualStudio/Validation.vcxproj` : **réussies**.

Conformité **20/20 par chaîne**, **2 719 refus sémantiques différentiels**
et **30 refus de normalisation** réussis dans chacune des trois matrices.
Les versions générées indiquent `0.27.0-alpha.10`. Style et
`git diff --check` réussissent.

Les trois `Frontend.GsE` sont identiques : **490 623 octets**, SHA-256
`B3DA8C8912BB3CFA2828D35DF5CAADFC6DF059B46EEF04617541A37DDE09B894`.
Chaque image est acceptée par `gseverifier` : GsE 1.0, 3 segments, 8 sections,
2 imports d'allocation/libération et **83 exports**, contre 81 précédemment.
Les deux exports ajoutés désignent la normalisation et son alias anglais.
`ConteneursDynamiques.GsPP` reste inchangé, SHA-256
`4EB8F7384823BEB1D9E2C9B073F67487AFE5F4B3FC038C5387181C1E2C50EF67`.

Formats 1.0, ABI 1, alpha.10 et structures publiques existantes conservés.
Les changements restent locaux, non commités et non poussés ; ils ne sont
inclus ni dans la CI de `c58874b` ni dans les paquets alpha.10 publiés.
Le frontend 0.27 complet n'est pas déclaré validé.

## Normalisation des membres et groupes mixtes — 6 octobre 2026

**VALIDÉ dans le périmètre testé sur les trois chaînes, après le commit publié
`039bd3f` ; cette extension reste locale.** `AssemblerDeclarationsNormalisees`
conserve sa requête de 128 octets, ses codes et ses deux exports. L'assemblage
brut et les anciennes entrées d'analyse conservent leur comportement.

### Clés, sélection et représentation de l'AST

La passe des fonctions comprend maintenant les méthodes, constructeurs,
destructeurs et opérateurs membres, dans la même famille que les fonctions
libres. Les noms complets et types lexicaux sont comparés exactement avant
résolution des alias. Le récepteur implicite `Classe&` est encodé une fois
par classe et comparé au premier paramètre explicite d'une fonction libre de
même nom complet. Constructeurs et destructeurs ont des clés distinctes ;
les surcharges et les qualificatifs de références/pointeurs restent distincts.

Les définitions compatibles remplacent les prototypes dans l'ordre de
première déclaration des fonctions, même si la déclaration retenue change
de catégorie membre/libre ou de classe propriétaire. La position, visibilité,
virtualité, paramètres explicites et corps sont ceux de la déclaration choisie.
Les incompatibilités et doubles définitions sont recherchées dans l'ordre
des fonctions du bootstrap, avant les globales et alias, après analyse de toutes
les unités. Le diagnostic conserve son unité, sa ligne et sa colonne locales.

L'AST de sortie place d'abord les structures, unions, classes et énumérations
avec leurs enfants non fonctionnels, dans leur ordre source. Les fonctions
sont ensuite émises dans leur ordre logique de première déclaration, parmi
les autres déclarations restantes. Chaque fonction garde son sous-arbre
contigu ; tous les parents sont réindexés vers des nœuds antérieurs. Les
consommateurs utilisent `Parent` : les méthodes ne sont pas nécessairement
contiguës à l'en-tête de leur classe. Cela évite les parents en avant lorsqu'un
prototype libre antérieur sélectionne une méthode d'une classe ultérieure.
Le texte assemblé et les origines ne sont pas déplacés.

Les types eux-mêmes ne sont pas fusionnés : deux déclarations compatibles
d'une même classe gardent leurs deux en-têtes et sont encore refusées par
la passe sémantique, comme le bootstrap. Aucun mécanisme de définition de
méthode hors classe ni nouvelle syntaxe de langage n'est ajouté.

### Preuves différentielles

`TesterNormalisationDeclarationsPreparees` utilise le normaliseur C++ réel
pour déterminer la déclaration retenue, puis compare tous les nœuds issus
des analyses Gs++ réelles et l'ordre des fonctions au programme C++ normalisé.
La matrice passe de 22 à **39 corpus bilingues valides**, dont prototypes
membres répétés, constructeurs/destructeurs, opérateurs, deux directions de
sélection membre/libre, qualifications, callbacks imbriqués, virtualité,
surcharges distinctes et champs avant/après une méthode. Un espace avec 400
composantes puis un nom de 256 caractères, contenant 32 classes, vérifie la
mesure des noms complets répétés et leur encodage itératif sans récursion.

Les conflits passent de 15 à **32 corpus bilingues**, soit **64 refus
différentiels de normalisation**, comptés séparément des refus sémantiques.
Le refus syntaxique bilingue reste testé. Les refus sémantiques passent de
8 à **14 corpus bilingues**, contrôlant aussi l'ordre des corps mixtes,
les classes non fusionnées et la limite d'arité avec récepteur implicite.
Les douze nouveaux refus portent la matrice sémantique de **2 719 à 2 731**.

Tailles exactes, trois capacités partielles, sentinelles, chaque échec
d'allocation, libération des arènes, requêtes invalides et unité vide sont
vérifiés. Les refus de normalisation et de syntaxe ne publient rien, même
avec trois tampons assez grands ; les entrées restent intactes. L'analyse
sémantique conserve également le texte, les nœuds et les origines normalisés.

### Validation et publication

- CMake/MSVC : construction `espace_travail` avec `windows-release`, puis
  `ctest --preset windows-release --output-on-failure` : **5/5** ;
- GNU/Linux sous Ubuntu/WSL : construction `espace_travail` avec `linux-release`,
  puis `ctest --preset linux-release --output-on-failure` : **6/6**, intégration comprise ;
- Visual Studio 2026 sans CMake : `GsPlusPlus.slnx`, puis
  `VisualStudio/Validation.vcxproj`, Release/x64 : **réussis**.

Conformité **20/20 par chaîne**, **2 731 refus sémantiques** et **64 refus de
normalisation**. Les trois `Frontend.GsE` sont identiques : **498 479 octets**,
SHA-256 `5ED77FF0F18C78B15CBE3C0B0F93503BC105F3921FA5FDC3985AA4C4E57A7186`.
Vérification réussie : GsE 1.0, 3 segments, 8 sections, 2 imports et 83 exports.
Les versions générées indiquent `0.27.0-alpha.10`. Style et `git diff --check`
réussissent ; `ConteneursDynamiques.GsPP` reste inchangé.

Les structures publiques, formats 1.0, ABI 1, bootstrap/backend C++ et version
alpha.10 ne changent pas. L'extension reste non commitée/non poussée, distincte
de la CI du commit `039bd3f` à 2 719 refus et des paquets alpha.10 à 619 refus.
Lecture, expansion des inclusions, `#pragma once`, origines internes aux inclusions
et raccordement au flux autonome de compilation restent côté hôte. Le frontend
0.27 complet n'est pas déclaré validé.

## Origines des inclusions préparées — 6 octobre 2026

**VALIDÉ dans le périmètre testé sur les trois chaînes ; tranche locale après
la normalisation des membres.** Le nouveau fichier
`AutoHebergement/AnalyseurDeclarations/OriginesDeclarations.GsPP` est intégré
à CMake et à la construction Visual Studio 2026 native. Il ne crée pas un
nouvel exécutable : les quatre nouveaux exports appartiennent à `Frontend.GsE`.

### Contrat additif et modes mixtes

`AnalyserDeclarationsAvecOrigines` / `AnalyzeOriginAwareDeclarations` reçoit
`RequeteDeclarationsAvecOrigines` de **56 octets**, qui référence la requête
syntaxique historique de 80 octets. Une table d'`OrigineJetonDeclarations`
de **40 octets** par entrée contient, pour chaque jeton du texte développé :
début/taille en octets, indice du fichier d'origine, ligne/colonne locales,
mode source/interface et champ réservé nul. Le jeton de fin est obligatoire,
à la taille du texte et de taille nulle. Les noms de fichiers appartiennent
au catalogue de l'hôte, pas à l'ABI de l'AST.

L'analyseur contrôle bornes, indices, coordonnées, modes et champs réservés,
puis compare le nombre et les plages exactes au lexage Gs++ réel avant de
publier l'AST. Le mode d'une déclaration est celui de son jeton décisif, comme
le bootstrap C++, y compris lorsqu'une signature traverse une inclusion.
Le mode global d'interface prime sur tous les modes locaux. Une unité avec
des inclusions développées reste **une seule unité de traduction** : une
inclusion n'est pas une unité séparée avec des imports d'espaces isolés.

Les nœuds gardent leurs positions dans le texte développé, sans agrandir
`NoeudDeclaration`. Les refus syntaxiques exposent aussi l'indice du fichier
et les coordonnées locales. `LocaliserOrigineDeclarationsPreparees` /
`LocatePreparedDeclarationOrigin` traduit un début de jeton ou EOF après
l'analyse, notamment pour les diagnostics sémantiques. Il utilise la même
table validée et inchangée, contrôle ses bornes sans relancer le lexage,
et n'effectue ni allocation ni lecture. Il ne traduit pas une position dans
un espace ou à l'intérieur d'un jeton. Chaque appel remet les sorties locales
à `NombreFichiers`, zéro, zéro avant de chercher une origine.

Les anciennes entrées source/interface, structures et contrats restent
inchangés. La nouvelle analyse conserve leur **contrat de capacité syntaxique** :
mesure exacte et publication possible du préfixe de l'AST en cas de tampon
partiel. Cela ne transforme pas l'analyse en assemblage transactionnel à
trois sorties. Les erreurs d'argument, de syntaxe et d'allocation ne publient
pas l'AST ; les entrées et sentinelles restent protégées.

### Préparation réelle et preuves différentielles

`TesterDeclarationsAvecOrigines` crée ses fichiers de test dans les répertoires
de construction, puis utilise le vrai `GsPP::PreparerJetonsSource` C++ pour
lire et développer les inclusions. Il fournit au frontend Gs++ un texte
canonique et les origines de tous les jetons, fin comprise. Il compare l'AST
au parseur C++ avec les mêmes modes ; les positions locales des refus sont
comparées séparément à l'analyse C++ des jetons originaux, sans positions
synthétiques. Pour la sémantique, le programme original passe aussi par le
normaliseur C++ réel avant comparaison du fichier, code, ligne et colonne.

La matrice comporte **20 corpus bilingues syntaxiquement valides** et
**7 corpus bilingues refusés par la syntaxe**. Trois des vingt premiers
produisent les **3 refus sémantiques bilingues** attendus : type inconnu,
nom absent et modification d'un stockage constant. Les six nouveaux refus
portent le total sémantique de **2 731 à 2 737**. Les refus syntaxiques et
les 64 refus de normalisation de la tranche précédente ne sont pas ajoutés
à ce total. Certains corpus vérifient uniquement l'AST, notamment avant
fusion des prototypes/définitions ; ils ne sont pas présentés comme des
programmes sémantiquement acceptés.

Sont couverts : inclusions imbriquées, `#pragma once` répété et auto-inclusion
protégée, absence de déduplication implicite sans protection, chemins relatifs
et Unicode, BOM, LF/CRLF, chaînes échappées, inclusions dans les espaces/classes,
imports directs/transitifs, méthodes/constructeurs/destructeurs/opérateurs,
signatures réparties entre fichiers, modes source/interface et retour au
fichier principal. Chaque jeton et EOF est localisé indépendamment. Les
capacités exactes/partielles, sentinelles, chaque échec d'allocation, tables
altérées, arguments invalides, source vide et UTF-8 invalide sont vérifiés,
ainsi que l'absence de fuite, les entrées intactes et le retour aux anciennes
entrées sans persistance du mode.

### Validation et limites

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail --parallel 6`,
  puis `ctest --preset windows-release --output-on-failure` : **5/5** ;
- GNU/Linux sous Ubuntu/WSL : `cmake --build --preset linux-release --target espace_travail --parallel 4`,
  puis `ctest --preset linux-release --output-on-failure` : **6/6**, intégration comprise ;
- Visual Studio 2026 sans CMake : `GsPlusPlus.slnx`, puis
  `VisualStudio/Validation.vcxproj`, Release/x64 : **réussis**.

Conformité **20/20 par chaîne** et **2 737 refus sémantiques différentiels**.
Les trois `Frontend.GsE` sont identiques : **505 599 octets**, SHA-256
`71611570BFAAC74A71B8CAD4D578571F3142ED870F91739A940095CD0A92A58E`.
Les trois vérificateurs acceptent GsE 1.0, 3 segments, 8 sections, 2 imports
et **87 exports**, contre 83 auparavant. Les versions générées indiquent
`0.27.0-alpha.10` ; `ConteneursDynamiques.GsPP` reste inchangé. Style et
`git diff --check` réussissent.

**Lecture, résolution des chemins, expansion des inclusions, `#pragma once`
et détection des cycles restent réalisés par l'hôte C++.** Cette tranche
valide le raccord syntaxique au texte développé, pas un préprocesseur Gs++
autonome. Le contrat d'origines de jetons reste à intégrer à l'assemblage et
à la normalisation multi-unités, puis aux diagnostics de leurs passes.
Le bootstrap/backend C++, les formats 1.0, ABI 1 et alpha.10 ne changent pas.
Les changements restent locaux, non commités/non poussés, distincts de la
CI de `039bd3f` à 2 719 refus et des paquets alpha.10 à 619 refus ; aucune
nouvelle release ni validation complète du frontend 0.27 n'est revendiquée.

## Assemblage des inclusions et diagnostics originaux — 6 octobre 2026

Les entrées additives avec origines raccordent maintenant le texte développé
et ses tables de jetons à l'assemblage brut, à la normalisation et à la
sémantique multi-unités. Les anciennes requêtes de 80/128 octets, l'AST de
64 octets, les tables d'unités de 32 octets et la sémantique existante ne
changent pas. Les huit nouveaux exports sont dans le même `Frontend.GsE`.

### API et identité des unités/fichiers

`RequeteAssemblageAvecOrigines` de **40 octets** référence la requête
d'assemblage historique, un catalogue commun de fichiers par indices et
une table de `TableOriginesUniteDeclarations` de **16 octets par unité**.
Chaque entrée référence les origines de tous les jetons d'une unité développée,
EOF compris. Le texte, mode global et rang d'unité viennent de l'ancienne
requête ; les noms et chemins de fichiers restent au catalogue de l'hôte.

- `AssemblerDeclarationsAvecOrigines` / `AssembleOriginAwareDeclarations` :
  assemblage brut sans fusion, modes mixtes et diagnostics originaux ;
- `AssemblerDeclarationsNormaliseesAvecOrigines` /
  `AssembleNormalizedOriginAwareDeclarations` : même contrat, avec la
  normalisation des déclarations libres/membres et groupes mixtes déjà décrite ;
- `LocaliserOrigineAssemblagePrepare` / `LocatePreparedAssemblyOrigin` :
  localisation d'un début de jeton ou EOF dans le texte synthétique d'une
  unité, avant son rebasage dans le texte assemblé, sans allocation ;
- `AnalyserSemantiqueUnitesAvecOrigines` / `AnalyzeOriginAwareUnitSemantics` :
  requête de **40 octets** référençant la sémantique et le contexte d'un
  assemblage avec origines réussi, avec les mêmes tampons de texte/AST.

Un fichier inclus reste dans l'unité de traduction de son consommateur.
Le même fichier peut être inclus dans deux unités distinctes et partager
le même indice de fichier ; le rang d'unité reste distinct. Les imports
d'espaces restent visibles dans leur unité, inclusions comprises, sans
fuite vers une unité séparée. Les classes/types répétés ne sont toujours
pas fusionnés ; leur conflit relève de la sémantique.

Les plages exactes et modes sont validés par le lexage/analyse de chaque
unité. Toutes les syntaxes passent avant les conflits de normalisation,
puis les familles fonctions, globales et alias gardent leur ordre de priorité.
La déclaration choisie garde son texte, ses positions synthétiques et son
origine de jeton. Les diagnostics de syntaxe, conflits et sémantique sont
comparés au fichier, ligne et colonne d'origine du bootstrap.

### Contrats mémoire conservés

L'assemblage/normalisation garde ses trois sorties transactionnelles : aucune
publication de texte, AST ou table d'unités en cas de refus, d'allocation
impossible ou de capacité insuffisante. La sémantique vérifie les tables, les
plages lexicales et l'égalité du texte assemblé avec les unités, BOM retirés,
avant sa passe. Elle ne modifie ni le contexte d'assemblage ni ses tampons.

La sémantique conserve en revanche son contrat historique de préfixes de
symboles/résolutions, y compris lorsque sa passe échoue ; ce n'est pas le
contrat transactionnel de l'assemblage. Une incohérence détectée pendant la
préparation des origines ne publie aucun de ces tampons. Les erreurs de
capacité, d'argument et d'allocation ne sont pas attribuées à un fichier
fautif : indices aux nombres d'unités/fichiers et coordonnées nulles.
Les diagnostics synthétiques historiques restent dans `Analyse.Resultat`.

### Matrice différentielle

`TesterAssemblageAvecOrigines` prépare de vrais fichiers avec
`GsPP::PreparerJetonsSource`. Le parseur C++ des jetons originaux détermine
les diagnostics locaux ; le parseur C++ du texte développé détermine l'AST
brut. Le normaliseur C++ réel détermine les déclarations retenues et l'ordre
des fonctions. Les nœuds retenus, parents réindexés, tranches nominales,
texte et tables d'unités sont vérifiés séparément.

La matrice ajoute **12 corpus bilingues valides**, **6 conflits bilingues de
normalisation** (12 refus différentiels), **3 refus syntaxiques bilingues** et
**7 refus sémantiques bilingues**. Les quatorze nouveaux refus sémantiques
portent le total de **2 737 à 2 751** ; conflits et syntaxe ne sont pas ajoutés
à ce total. Sont notamment couverts : prototypes/définitions entre unités,
globales et alias inclus répétés, méthodes/constructeurs/destructeurs/opérateurs,
inclusions Unicode/relatives imbriquées avec `#pragma once`, signatures à
cheval sur deux fichiers, unités vides/BOM et EOF, classes répétées,
priorité des fonctions sur les globales et d'une syntaxe tardive sur les conflits.

Chaque jeton et EOF est localisé après normalisation. Capacités exactes et
les trois capacités partielles, sorties refusées avec tampons suffisants,
chaque échec d'allocation des deux assemblages et de la sémantique, sentinelles,
arguments invalides, tables altérées et texte assemblé incohérent sont vérifiés.
Les sources, unités, tables et AST restent intacts et les arènes sont libérées.

### Validation et limites

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail --parallel 6`,
  puis `ctest --preset windows-release --output-on-failure` : **5/5** ;
- GNU/Linux sous Ubuntu/WSL : construction `espace_travail` avec `linux-release`,
  puis `ctest --preset linux-release --output-on-failure` : **6/6**, intégration comprise ;
- Visual Studio 2026 sans CMake : `GsPlusPlus.slnx`, puis
  `VisualStudio/Validation.vcxproj`, Release/x64 : **réussis**.

Conformité **20/20 par chaîne**, **2 751 refus sémantiques différentiels**,
64 refus de normalisation préparée et 12 refus de normalisation avec inclusions.
Les trois images construites sont identiques : **516 863 octets**, SHA-256
`19582C4A977E90CFEE96CB6B5A2D45E0796B74E181C831DB348B15BC294475E3`.
Les trois vérificateurs acceptent GsE 1.0, 3 segments, 8 sections,
2 imports et **95 exports**, contre 87 auparavant.
Les trois versions générées indiquent `0.27.0-alpha.10`. Style et
`git diff --check` réussissent ; `ConteneursDynamiques.GsPP` reste inchangé.

Lecture, résolution des chemins, expansion, `#pragma once` et détection des
cycles restent côté hôte C++. Les tables restent celles des unités d'entrée,
sans nouveau tampon de jetons concaténés. Ce raccordement préparé ne remplace
pas encore le chemin de compilation de fichiers de `gsppc`. Formats 1.0,
ABI 1 et alpha.10 sont conservés ; bootstrap/backend C++ inchangés.
Les changements restent locaux, non commités/non poussés, distincts de la
CI de `039bd3f` et des paquets alpha.10 publiés ; pas de nouvelle release
ni de validation du frontend 0.27 complet.

## Préparation lexicale avec origines — 6 octobre 2026

La construction du texte développé et de sa table d'origines est maintenant
réalisée par `Frontend.GsE`, et non par les deux assembleurs de texte C++ des
tests d'inclusions. L'hôte conserve la sélection des fragments originaux après
expansion. Le lexeur Gs++ fournit leurs plages ; les genres/hachages et positions
sélectionnés sont comparés aux jetons originaux du bootstrap. Aucune lecture de
fichier n'est ajoutée au frontend et le pilote habituel de `gsppc` est inchangé.

### Entrée et garanties

`PreparerDeclarationsAvecOrigines` / `PrepareOriginAwareDeclarations` reçoit
une `RequetePreparationDeclarations` de **112 octets**, des fragments originaux
de **40 octets** et publie un résultat de **48 octets**. Chaque fragment non
final doit contenir exactement un jeton, sans BOM, séparateur périphérique ou
commentaire résiduel ; le dernier fragment vide porte l'EOF racine. L'indice
du fichier, les coordonnées locales et le mode source/interface sont fournis
par l'hôte. Ils ne sont pas vérifiés contre un catalogue de fichiers par cette API.

Les lexèmes sont copiés sans décodage/réencodage des chaînes ; un séparateur LF
ou CRLF est ajouté après chaque jeton non final et le BOM de sortie est optionnel.
Les plages d'origine excluent ces ajouts. L'EOF est exactement à la taille du
texte préparé, avec son fichier et sa position originaux. Les directives non
développées sont refusées. Lecture, résolution des chemins, expansion textuelle,
`#pragma once` et cycles restent côté hôte.

La passe valide **toutes les entrées avant de publier** le texte ou les origines.
Les deux sorties sont transactionnelles : mesure exacte et absence de préfixe
sur refus, même tardif, ou capacité insuffisante. Les diagnostics lexicaux sont
rébasés dans le fichier original sans débordement silencieux ; les tailles et
métadonnées sont contrôlées avant lecture, la borne cumulée avant lexage.
Entrées, requête et sorties doivent rester stables et ne pas se recouvrir.
La préparation n'alloue rien ; les imports d'hôte restent limités à allocation/
libération pour les autres passes. Les anciens contrats ne changent pas.

### Tests et raccordement

`TesterPreparationDeclarations` vérifie **40 lexèmes** français/anglais et
Unicode, nombres décimaux, ponctuation et chaînes échappées dans **quatre modes
BOM/LF/CRLF**. Le texte et toutes les plages sont comparés octet par octet ;
un nouveau lexage C++ vérifie les genres/textes et l'absence de fusion de jetons.
Sont également couverts **21 fragments refusés**, les neuf arguments globaux
invalides, les métadonnées de jeton/EOF, unités vides avec/sans BOM, fins non
vides, refus tardifs avec tampons suffisants, débordements de coordonnées et
taille cumulée, deux capacités partielles, sentinelles, appels répétés et absence
d'allocation.

`PreparerFragmentsInclus` sélectionne les plages dans les sources originales,
puis appelle réellement le nouvel export pour la mesure et la publication.
`TesterDeclarationsAvecOrigines` et `TesterAssemblageAvecOrigines` utilisent
ses sorties : les matrices syntaxiques et celles d'assemblage brut, normalisation,
diagnostics originaux et sémantique par unité restent inchangées et passent.
Les chaînes échappées de la matrice mono-unité sont préservées. Le total reste
**2 751 refus sémantiques différentiels** ; les refus de préparation lexicale
ne sont pas comptabilisés comme de nouveaux refus sémantiques.

### Validation locale

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail --parallel 6`,
  puis `ctest --preset windows-release --output-on-failure` : **5/5** ;
- GNU/Linux Ubuntu/WSL : construction `espace_travail` avec `linux-release`,
  puis `ctest --preset linux-release --output-on-failure` : **6/6**, intégration comprise ;
- Visual Studio 2026 sans CMake : `GsPlusPlus.slnx` puis
  `VisualStudio/Validation.vcxproj`, Release/x64 : **réussis**.

Conformité **20/20 par chaîne**. Les trois images sont identiques :
**524 319 octets**, SHA-256
`8CA3386E54530C384BC53E04BEA39C115B5A17E5AB3C120D6C2E1E3BE01DE5D2`.
Les trois vérificateurs acceptent GsE 1.0, trois segments, huit sections,
deux imports et **97 exports**, contre 95 au raccordement précédent.
Les trois versions générées indiquent `0.27.0-alpha.10` ; formats 1.0 et ABI 1
conservés. Style et `git diff --check` réussissent, `ConteneursDynamiques.GsPP`
reste inchangé. Travail local non commité/non poussé, sans nouvelle release
ni modification des paquets alpha.10 publiés ; distinct de la CI de `039bd3f`.
Cette préparation validée ne signifie ni expansion auto-hébergée des inclusions,
ni remplacement du pilote de fichiers, ni frontend 0.27 complet.

## Expansion des inclusions avec origines — 6 octobre 2026

`Frontend.GsE` développe désormais les inclusions textuelles en mémoire avec
`DevelopperInclusionsDeclarations` / `ExpandDeclarationIncludes`. L'hôte lit
les fichiers, attribue leurs identités canoniques/noms de diagnostic et résout
les chemins ; il ne choisit plus les jetons ni les états `once`/cycles dans le
chemin raccordé des tests. Le pilote habituel de `gsppc` reste inchangé.

### Contrat et ordre des diagnostics

Le contrat public est dans `ExpansionDeclarations.HGsPP` : fichier et lien de
**32 octets**, requête additive de **128 octets** et résultat de **48 octets**.
Les indices distinguent les noms de diagnostic, les identités partagées
regroupent les alias pour `once` et les cycles. Les liens sont uniques, triés par
fichier/début de directive et portent cible disponible, introuvable ou extension
incompatible. L'hôte garantit contenus, identités, modes et cibles ; le frontend
ne les vérifie pas contre le système de fichiers. Un lien manquant pour une
directive valide est un argument invalide, distinct d'un lien déclaré introuvable.

Le fichier entier est lexé **avant ses directives**, comme dans le bootstrap.
Une directive commence une ligne logique, possède un argument sur cette ligne
et aucun jeton supplémentaire. Seul `#pragma once` est pris en charge ; les
chemins de `#inclure` / `#include` sont cités, non vides et sans NUL/CR/LF décodé.
`once` prend effet lorsqu'il est rencontré, pas par préscan. Une identité déjà
protégée est ignorée avant lexage/réentrée/profondeur. Sans protection, un cycle
est signalé sur la directive appelante ; la 129e entrée active est refusée à
1:1 du fichier cible. L'état est local à chaque appel et unité de traduction.

La sélection utilise une pile itérative de **128 cadres**, un cache lexical et
une arène. Elle appelle ensuite la préparation lexicale validée précédemment,
avec lexèmes originaux, BOM/LF/CRLF, modes par fichier et seul EOF racine.
Texte et origines restent **transactionnels**, y compris après un refus tardif
ou un échec d'allocation. Capacité insuffisante : deux tailles exactes, aucun
préfixe ; autres refus : tailles nulles. Les erreurs d'argument/capacité/
allocation ne désignent aucun fichier fautif ; les refus de langue gardent
fichier et coordonnées originaux. Les entrées restent intactes et l'arène
est toujours libérée. Aucun nouvel import de fichiers n'est ajouté.

### Matrice et raccordement

`TesterExpansionDeclarations` compare **41 corpus déclinés en français/anglais**
au véritable `GsPP::PreparerJetonsSource`. Sur les dossiers utilisés par les
trois chaînes, ils donnent **15 corpus bilingues valides et 26 refus bilingues**.
La variante de casse vérifie l'existence réelle du chemin : un répertoire
sensible à la casse la classe en refus, pas en réussite. Le test ne suppose
pas que tout dossier WSL est sensible à la casse ; les identités suivent la
normalisation d'hôte du bootstrap.

Sont couverts : inclusions répétées, relatives/Unicode et alias de chemin,
`once` tardif ou immédiat, réentrée directe/mutuelle protégée, cycles directs/
indirects, chaînes échappées, fichiers vides/BOM, limites de profondeur 128/129,
réentrée protégée à profondeur maximale, ordre lexical avant directives,
extensions incompatibles et refus de syntaxe des directives. Chaque diagnostic
est comparé au fichier/ligne/colonne C++ ; le code attendu et le détail lexical
sont vérifiés séparément. Textes et plages d'origine sont comparés à la
sélection du bootstrap, sans réencoder les lexèmes.

Capacités exactes/partielles, refus avec tampons suffisants, injection de chaque
échec d'allocation jusqu'au succès, sentinelles, appels répétés, états non
persistants, fichiers non visités, arguments/métadonnées/liens invalides,
ordre/unicité des liens et lien requis absent sont vérifiés.

À cette étape, `CreerCatalogueExpansion` est un adaptateur de test C++ de lecture/résolution.
Il construit le graphe sans exécuter `#pragma once` ni sélectionner les jetons.
`PreparerFragmentsInclus` appelle maintenant l'expansion Gs++ ; la sélection C++
reste uniquement l'oracle. Les deux matrices existantes utilisent ses sorties
jusqu'à la syntaxe, l'assemblage brut/normalisé et la sémantique avec origines.
Le total reste **2 751 refus sémantiques différentiels** : les nouveaux refus
d'expansion ne sont pas comptabilisés comme refus sémantiques.

### Validation locale et reproductibilité

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail --parallel 6`,
  puis `ctest --preset windows-release --output-on-failure` : **5/5** ;
- GNU/Linux Ubuntu/WSL : construction `espace_travail` avec `linux-release`,
  puis `ctest --preset linux-release --output-on-failure` : **6/6**, intégration comprise ;
- Visual Studio 2026 sans CMake : `GsPlusPlus.slnx`, puis
  `VisualStudio/Validation.vcxproj`, Release/x64 : **réussis**.

Conformité **20/20 par chaîne**, **2 751 refus sémantiques différentiels**.
L'ordre du nouvel en-tête a été synchronisé entre CMake et le constructeur
MSBuild natif ; des ordres différents donnaient des agencements d'image
différents malgré un code fonctionnel. Les trois images finales sont identiques :
**542 527 octets**, SHA-256
`31D9AAFF085D707E323426287AB0A1E909739EA131B46D6D8DF2AA27342DD1D4`.
Les trois vérificateurs acceptent GsE 1.0, trois segments, huit sections,
deux imports et **99 exports**, contre 97 à la préparation précédente.
Les trois versions générées indiquent `0.27.0-alpha.10`. Style et
`git diff --check` réussissent, `ConteneursDynamiques.GsPP` reste inchangé.
Travail local non commité/non poussé, distinct de la CI de `039bd3f` et des
paquets alpha.10 publiés ; aucune nouvelle release.

L'intégration au pilote de compilation de fichiers reste ouverte ; ni ce raccord
de tests ni le succès de l'expansion ne prouvent un frontend 0.27 complet.
Les identités/cibles sont des garanties de l'hôte, non une nouvelle API de
résolution de fichiers. Version alpha.10, formats 1.0 et ABI 1 conservés,
sans modification de `ConteneursDynamiques.GsPP` ni du bootstrap/backend C++.

## Catalogue hôte réutilisable et préparation du produit — 6 octobre 2026

### Raccordement implémenté

`Compiler/include/GsPP/CatalogueInclusions.hpp` et
`Compiler/src/CatalogueInclusions.cpp` remplacent l'adaptateur de test par un
composant de `gspp_compiler`, enregistré dans CMake et la solution native
Visual Studio 2026. Les miroirs hôtes des origines et de la requête d'expansion
sont partagés, avec tailles et décalage du résultat vérifiés à la compilation.
L'export reste celui du frontend Gs++ : aucun nouvel export, import ou format.

`CreerCatalogueInclusions` construit un graphe itératif, sans sélection de
jetons ni interprétation de `once`. Le catalogue possède les textes dans des
nœuds stables, partage un instantané par identité canonique, conserve des
indices de diagnostic distincts pour les alias et sépare chemin physique et
nom virtuel de diagnostic. Un préfixe connu garde ses indices pour les unités
successives. Copies interdites et déplacements explicites évitent les vues
pendantes, même pour les chaînes courtes ; l'objet déplacé est vidé.

Limites hôtes configurables : 4 096 fichiers, 100 000 liens, 16 Mio par fichier
et 64 Mio de textes distincts par défaut. Lecture bornée par blocs, aucune
lecture des fichiers absents ou d'extension incompatible ; les dépassements
et échecs d'E/S abandonnent le catalogue complet. Les limites hôtes ne changent
pas le diagnostic de profondeur de 128 fichiers actifs, appliqué par Gs++.

`PreparerSourceAvecOrigines` appelle l'export Gs++ pour la mesure puis la
publication, avec convention Microsoft x64 explicite sur GNU/Linux et option
GNU adaptée au fichier appelant. La sortie propriétaire contient texte,
origines et résultat ABI. Refus du frontend : sorties vides et diagnostic
conservé. Capacités excessives ou contrat incohérent : exception hôte. Les
deux matrices d'inclusions emploient désormais cette API jusqu'à la syntaxe,
l'assemblage brut/normalisé et la sémantique avec origines ; le bootstrap
continue de fournir un oracle indépendant.

### Régressions ajoutées et limites de la tranche

`TesterCatalogueInclusionsProduit` vérifie en français/anglais les noms
virtuels, préfixes connus, alias et identité partagée, BOM source/UTF-8/CRLF,
quatre modes de sortie, déplacements/auto-déplacement, refus des copies,
instantané conservé après modification sur disque et nouveau catalogue à jour.
Bornes exactes/dépassées, compte des identités sans doublonner les alias,
arguments invalides, extension/absence dans l'ordre, diagnostic lexical Gs++,
refus sans sortie et libération de l'arène sont couverts. Un graphe de
**512 niveaux** est construit sans récursion hôte, puis refusé au niveau 129
par le frontend. Sept contrats ABI incohérents et un échec de publication
simulés contrôlent les refus de l'adaptateur.

La découverte des chemins emploie encore le lexeur C++, et la lecture de tout
le graphe est anticipée : un fichier finalement ignoré par `once` peut provoquer
un échec d'E/S ou de limite hôte avant un diagnostic de langue. Cette limite
est explicite, pas une preuve de parité complète des E/S. La lecture/résolution
à la demande et le raccord au pilote par défaut restent ouverts. Aucun
remplacement du backend ou du parcours habituel de `gsppc` n'est revendiqué.

### Validation locale de l'adaptateur

- CMake/MSVC : `cmake --build --preset windows-release --target espace_travail --parallel 6`,
  puis `ctest --preset windows-release --output-on-failure` : **5/5** ;
- Ubuntu/WSL : `cmake --build --preset linux-release --target espace_travail --parallel 4`,
  puis `ctest --preset linux-release --output-on-failure` : **6/6**, intégration comprise ;
- Visual Studio 2026 sans CMake : construction de `GsPlusPlus.slnx`, puis
  validation par `VisualStudio/Validation.vcxproj`, Release/x64 : **réussies**.

Les trois matrices valident le raccordement du produit, **15 corpus bilingues
d'expansion valides et 26 refus**, et **2 751 refus sémantiques différentiels**.
Conformité **20/20 par chaîne** ; contrôles de style et cohérence des sept
cibles CMake/Visual Studio réussis. Les trois images restent identiques et
acceptées par leurs vérificateurs : **542 527 octets**, GsE 1.0, trois segments,
huit sections, deux imports et **99 exports** ; SHA-256
`31D9AAFF085D707E323426287AB0A1E909739EA131B46D6D8DF2AA27342DD1D4`.
Les trois `VersionProduit.hpp` générés indiquent `0.27.0-alpha.10` ; formats 1.0
et ABI 1 conservés. `git diff --check` réussit et `ConteneursDynamiques.GsPP`
reste inchangé. Le travail reste local, non commité/non poussé, sans nouvelle
release ; ces résultats ne mettent pas à jour la preuve CI de `039bd3f` ni les
paquets alpha.10 publiés.

## Travaux restant dans Gs++ 0.27

- compléter les combinaisons de conversions et qualifications encore
  absentes de la matrice différentielle ;
- compléter la lecture/résolution à la demande du catalogue hôte, puis intégrer
  le chemin préparé avec origines au pilote de fichiers, sans confondre les inclusions
  textuelles avec les unités de traduction séparées ;
- compléter les autres familles sémantiques encore prises en charge par le
  bootstrap, notamment les contextes des constructions et opérateurs et les
  interactions de priorité entre passes non encore testées, dont les contrôles
  non couverts d'initialiseurs globaux et les interactions avec les constructions
  locales non couvertes, qualifications et signatures locales non représentées,
  ainsi que les contextes de bases/champs non représentés
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
