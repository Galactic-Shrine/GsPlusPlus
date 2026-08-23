# Validation Gs++ 0.27.0-alpha.3

**VALIDÉ POUR L’AST DES DÉCLARATIONS DE CODE ET DE DONNÉES — FRONTEND 0.27
PARTIEL — 23 août 2026.**

## Objet

Gs++ 0.27.0-alpha.3 poursuit la migration du frontend auto-hébergé. Après le
lexeur de l’alpha.1 et les fonctions libres de l’alpha.2, cette préversion
étend l’AST compact aux déclarations de données.

Cette validation ne déclare pas l’analyseur syntaxique complet. Les méthodes,
constructeurs, destructeurs et opérateurs de classes, puis les instructions et
expressions, restent à migrer. Le statut global du frontend 0.27 demeure donc
`PARTIEL`.

## Périmètre livré

Les fichiers canoniques restent :

- `AutoHebergement/AnalyseurDeclarations/AnalyseurDeclarations.HGsPP` ;
- `AutoHebergement/AnalyseurDeclarations/AnalyseurDeclarations.GsPP`.

L’export de l’image auto-hébergée est :

```text
Gs::Autohebergement::AnalyserDeclarationsSource(
    RequeteAnalyseDeclarations*) -> ErreurAnalyseDeclarations
```

L’alpha.3 ajoute :

- les variables globales publiques, privées, externes et initialisées ;
- les globales et champs en tableaux fixes ou multidimensionnels ;
- les structures et unions ;
- les classes de données avec visibilité et héritage simple ;
- les champs et initialiseurs par défaut des champs de classes ;
- les énumérations et énumérateurs implicites ou explicitement initialisés ;
- les alias de déclarations et alias de champs ;
- les noms d’alias et cibles qualifiés ;
- les relations parent-enfant entre agrégats et champs, énumérations et
  énumérateurs, fonctions et paramètres.

Les corps et initialiseurs sont délimités avec suivi des parenthèses,
accolades et crochets imbriqués. Leur présence est portée par les drapeaux du
nœud, mais leur AST d’instructions ou d’expressions n’est pas encore construit.

## Contrat ABI de l’AST

Les dispositions x86-64 restent compatibles avec l’alpha.2 :

| Structure | Taille |
| --- | ---: |
| `NoeudDeclaration` | 64 octets |
| `ResultatAnalyseDeclarations` | 48 octets |
| `RequeteAnalyseDeclarations` | 80 octets |

Les genres publics couvrent désormais :

1. programme ;
2. fonction et paramètre ;
3. variable globale ;
4. structure, union et classe ;
5. champ ;
6. énumération et énumérateur ;
7. alias et alias de champ.

Les drapeaux décrivent la visibilité publique, protégée ou privée, le
caractère externe, la présence d’une définition ou d’un initialiseur et la
présence d’un héritage. Les empreintes de types restent normalisées entre les
syntaxes française et anglaise.

Les jetons et nœuds de travail sont alloués dans `AreneMemoire`. Le stockage
final appartient à l’appelant. Les contrôles vérifient que toutes les arènes
sont détruites, y compris après une erreur, une interrogation de capacité ou
une sortie partielle.

## Preuve différentielle

`gspp_autohebergement_tests` charge réellement
`AnalyseurDeclarations.GsE`, résout ses deux imports d’hôte et compare chaque
nœud au `Programme` produit par `GsPP::AnalyseurSyntaxique`.

La matrice comprend notamment :

- des corpus de fonctions français et anglais équivalents ;
- des corpus de déclarations de données français et anglais équivalents ;
- des espaces simples, imbriqués et qualifiés ;
- des globales publiques, externes, initialisées et en tableaux ;
- un initialiseur agrégat avec virgules imbriquées ;
- des structures pleines et vides ;
- une union ;
- des classes de données, trois niveaux de visibilité et un héritage public ;
- un initialiseur de champ de classe ;
- des énumérations pleines et vides, avec valeur implicite, expression
  explicite et virgule terminale ;
- un alias qualifié et des alias de champs ;
- des types utilisateur qualifiés, pointeurs, références et tableaux
  multidimensionnels ;
- une capacité nulle, partielle puis exacte ;
- les requêtes et tampons incohérents ;
- huit diagnostics syntaxiques comparés au bootstrap avec ligne et colonne ;
- une erreur lexicale propagée avec son code détaillé ;
- le refus distinct d’une méthode de classe pourtant acceptée par le
  bootstrap, afin de matérialiser la frontière de la tranche ;
- l’absence de libération invalide ou d’allocation résiduelle.

## Environnements

### Windows

- Windows x64 ;
- Visual Studio Community 2026 18.9 ;
- MSVC 19.51.36256.0, toolset v145 ;
- CMake 4.4.0 ;
- générateur `Visual Studio 18 2026` ;
- configuration `Release`.

Résultat CTest :

```text
1/3 gspp_tests                   Passed
2/3 gspp_autohebergement_tests  Passed
3/3 gspp_conformite             Passed

100% tests passed, 0 tests failed out of 3
```

### GNU/Linux

- Ubuntu sous WSL, construction dans le stockage Linux natif ;
- GNU C++ 11.4.0 ;
- CMake 3.22.1 ;
- Ninja 1.10.1 ;
- Python 3.10.12 ;
- configuration `Release`.

Résultat CTest :

```text
1/4 gspp_tests                   Passed
2/4 gspp_autohebergement_tests  Passed
3/4 gspp_conformite             Passed
4/4 gspp_integration            Passed

100% tests passed, 0 tests failed out of 4
```

La conformité portable réussit à **20/20** sur les deux chaînes.

## Construction auto-hébergée

La compilation de l’analyseur produit :

```text
7 unités
51 100 octets de code
639 octets de données
16 octets zéro
142 symboles
452 relocalisations
```

L’édition de liens de `AnalyseurDeclarations.GsE` contient 205 symboles et
487 relocalisations.

## Reproductibilité des images auto-hébergées

Les images MSVC et GNU sont identiques bit à bit :

| Image | Taille | SHA-256 |
| --- | ---: | --- |
| `ClassificateurMotsCles.GsE` | 12 300 octets | `efe102b61e172ddc207d5cc5dcdfd0310f03edde62b8325e58a9413e19377699` |
| `Lexeur.GsE` | 40 897 octets | `5a0895e7b8ad1f1c960af564391e2cbb1bb991b2efaad603f5840c8c50b75e54` |
| `AnalyseurDeclarations.GsE` | 72 001 octets | `34580a465c2ca8a9369d1754cb200fa8533b882d40b607370c353d1a72a46378` |

Le vérificateur confirme pour `AnalyseurDeclarations.GsE` :

```text
GsE 1.0, 3 segment(s), 8 section(s), 2 import(s), 69 export(s)
GsE valide.
```

Les deux imports restent exclusivement :

- `Gs::Hote::AllouerMemoire` ;
- `Gs::Hote::LibererMemoire`.

Les formats `.GsObj`, `.GsA` et `.GsE` restent en version 1.0, leurs champs
ABI valent 1 et leurs signatures sont `GSOBJ:0`, `GSA:0` et `GSE:0`.

## Benchmark fonctionnel

Le mode `smoke` réussit sur Windows et GNU pour les quatre scénarios :

- `s_source_gsobj` ;
- `m_monolithic_gse` ;
- `m_separate_gse` ;
- `l_system_library`.

Ces exécutions démontrent le fonctionnement du protocole et l’absence de
régression fonctionnelle. Elles ne constituent aucune revendication de
performance comparative.

Les preuves locales consolidées sont conservées dans :

```text
D:\GSLSE\Construction\GsPlusPlus-Public\Validation-0.27.0-alpha.3\
├── Windows\
│   ├── ctest.log
│   ├── conformite.json
│   └── Benchmark\
└── GNU\
    ├── ctest.log
    ├── conformite.json
    └── Benchmark\
```

## Distribution

La construction autonome installe trois images auto-hébergées :

- `ClassificateurMotsCles.GsE` ;
- `Lexeur.GsE` ;
- `AnalyseurDeclarations.GsE`.

Les paquets Windows et Linux comprennent aussi les outils, le SDK, les
bibliothèques Gs++, les exemples, la documentation Markdown et la licence
MPL-2.0 canonique.

## Conclusion

L’AST des déclarations de code et de données de Gs++ 0.27.0-alpha.3 est
`VALIDÉ` sous MSVC et GNU. Le frontend 0.27 reste `PARTIEL` : la prochaine
tranche doit migrer les méthodes et autres membres exécutables des classes,
puis construire les AST d’instructions et d’expressions avant toute
revendication d’analyseur syntaxique auto-hébergé complet.
