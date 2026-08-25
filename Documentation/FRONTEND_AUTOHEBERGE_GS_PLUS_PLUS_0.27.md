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
noms. La branche de développement suivante sélectionne maintenant les
surcharges libres à partir des types déjà déterminables dans l’AST compact.
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
  accès membre, indexation, conversion ou agrégat ;
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

La passe auto-hébergée sélectionne désormais une fonction libre précise
lorsqu’une référence placée en cible d’appel désigne plusieurs surcharges. Le
choix tient compte :

- du nombre de paramètres ;
- des empreintes de type des paramètres et des arguments déjà résolus ;
- des paramètres, variables locales et globales utilisés comme arguments ;
- des conversions explicites présentes dans l’AST ;
- du type canonique des littéraux booléens, chaînes et entiers ;
- de l’adaptation d’un littéral entier lorsque sa valeur est représentable
  dans le type du paramètre ;
- du score minimal de conversion, avec détection des ex æquo.

La résolution produite conserve le drapeau `GroupeSurcharges`, mais son
`IndexSymbole` désigne maintenant la surcharge effectivement choisie et son
`HachageType` le type de retour de cette fonction. Deux appels bilingues
sélectionnent différentiellement les variantes `entier32` et `entier64` de
`Choisir`. Deux corpus négatifs supplémentaires vérifient les diagnostics
`AucuneSurchargeCompatible` et `AppelSurchargeAmbigu`, portant le total courant
à dix-sept corpus sémantiques négatifs.

Cette tranche est validée localement par la reconstruction réelle de
`AnalyseurSemantique.GsE` et par les suites complètes MSVC et GNU. Elle ne
couvre pas encore les surcharges de membres, les références, les agrégats,
l’héritage ni toutes les conversions implicites du bootstrap C++.

## Travaux restant dans Gs++ 0.27

- compléter la résolution et la comparaison des types de toutes les
  expressions ;
- étendre la sélection typée aux membres, références, agrégats, conversions
  implicites et relations d’héritage ;
- étendre la résolution aux membres, conversions et appels de constructeurs ;
- migrer les vérifications de visibilité, héritage et durée de vie ;
- étendre la conformité seulement lorsque cette tranche forme un frontend
  cohérent ;
- reconstruire les benchmarks avant la version 0.27.0 finale ;

Les outils de la tranche publique annoncent `0.27.0-alpha.7`. Aucun statut
`VALIDÉ` ni `stable` n’est revendiqué pour Gs++ 0.27 dans son ensemble.
