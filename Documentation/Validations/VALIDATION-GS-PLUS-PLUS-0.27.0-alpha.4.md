# Validation Gs++ 0.27.0-alpha.4

**VALIDÉ POUR LES CATÉGORIES DE DÉCLARATIONS, Y COMPRIS LES MEMBRES
EXÉCUTABLES DE CLASSES — FRONTEND 0.27 PARTIEL — 24 août 2026.**

## Objet

Gs++ 0.27.0-alpha.4 poursuit la migration du frontend auto-hébergé. Après le
lexeur, les fonctions libres et les déclarations de données, cette préversion
ajoute les méthodes, constructeurs, destructeurs et surcharges d’opérateurs de
classes à l’AST compact écrit en Gs++.

Cette validation ne déclare pas l’analyseur syntaxique complet. Les corps et
initialiseurs sont correctement délimités, mais leur hiérarchie d’instructions
et d’expressions reste à migrer. Les premières étapes sémantiques sont
également encore assurées par le bootstrap C++. Le statut global du frontend
0.27 demeure donc `PARTIEL`.

## Périmètre livré

Les fichiers canoniques restent :

- `AutoHebergement/AnalyseurDeclarations/AnalyseurDeclarations.HGsPP` ;
- `AutoHebergement/AnalyseurDeclarations/AnalyseurDeclarations.GsPP`.

L’export de l’image auto-hébergée est :

```text
Gs::Autohebergement::AnalyserDeclarationsSource(
    RequeteAnalyseDeclarations*) -> ErreurAnalyseDeclarations
```

L’alpha.4 ajoute quatre genres publics :

- `Methode` / `Method` ;
- `FonctionConstructeur` / `ConstructorFunction` ;
- `FonctionDestructeur` / `DestructorFunction` ;
- `SurchargeOperateur` / `OperatorOverload`.

Chaque membre exécutable est enfant de sa classe. Ses paramètres explicitement
écrits dans la source sont enfants du membre. Le paramètre de traitement
`soi`, synthétisé par le bootstrap C++ pour le passage sémantique, n’est pas
présenté comme un paramètre source.

Le nœud conserve :

- la visibilité publique, protégée ou privée ;
- la présence du corps ;
- les modificateurs `virtuel` et `remplacer` ;
- l’initialisation explicite de la base ;
- la délégation vers un autre constructeur de la même classe ;
- la présence d’un ou plusieurs initialiseurs de champs.

Pour une méthode ordinaire, la tranche de nom désigne son identifiant. Pour
une surcharge, elle désigne le symbole de l’opérateur, indépendant du mot-clé
français ou anglais. Pour un constructeur ou destructeur, elle désigne le nom
source de la classe propriétaire ; le genre du nœud distingue les deux
opérations. Cette normalisation permet aux corpus français et anglais de
produire le même AST structurel tout en conservant une tranche source valide.

Les corps et arguments d’initialisation imbriqués sont délimités avec suivi
des parenthèses, accolades et crochets. Leur présence est décrite, mais aucun
AST d’instruction ou d’expression n’est revendiqué dans cette tranche.

## Contrat ABI de l’AST

Les dispositions x86-64 restent compatibles avec les alpha.2 et alpha.3 :

| Structure | Taille |
| --- | ---: |
| `NoeudDeclaration` | 64 octets |
| `ResultatAnalyseDeclarations` | 48 octets |
| `RequeteAnalyseDeclarations` | 80 octets |

Les nouveaux genres utilisent les valeurs 12 à 15. Les nouveaux drapeaux
utilisent les bits 64 à 1024, sans renuméroter les valeurs déjà publiées. Les
empreintes de types restent normalisées entre les syntaxes française et
anglaise.

Les jetons et nœuds de travail sont alloués dans `AreneMemoire`. Le stockage
final appartient à l’appelant. Les contrôles vérifient que toutes les arènes
sont détruites, y compris après une erreur, une interrogation de capacité ou
une sortie partielle.

## Preuve différentielle

`gspp_autohebergement_tests` charge réellement
`AnalyseurDeclarations.GsE`, résout ses deux imports d’hôte et compare chaque
nœud au `Programme` produit par `GsPP::AnalyseurSyntaxique`.

La matrice alpha.4 ajoute notamment :

- deux corpus de classes structurellement équivalents, l’un en français et
  l’autre en anglais ;
- une méthode ordinaire, une méthode virtuelle et une méthode de
  remplacement ;
- des constructeurs avec paramètre, sans paramètre, avec base explicite, avec
  champ explicite et avec délégation par `soi` / `this` ;
- des destructeurs virtuels et de remplacement ;
- des surcharges des opérateurs `+` et `==` ;
- les trois niveaux de visibilité ;
- l’entrelacement source de champs et de membres exécutables ;
- les relations parent-classe et parent-membre ;
- exactement six paramètres source, sans les neuf paramètres implicites
  `soi` des membres du corpus ;
- la cohérence de chaque tranche et de chaque hachage de nom ;
- six nouveaux diagnostics comparés au bootstrap avec ligne et colonne :
  opérateur manquant, modificateur dupliqué, constructeur virtuel, paramètre de
  destructeur, liste d’initialisation sur une méthode et délégation mélangée à
  un autre initialiseur.

Avec les huit diagnostics de l’alpha.3, quatorze cas syntaxiques sont comparés
au bootstrap. Les requêtes nulles ou incohérentes, les capacités nulle,
partielle et exacte, la propagation des erreurs lexicales, les libérations
invalides et les allocations résiduelles restent également contrôlées.

## Environnements

### Windows

- Windows x64 ;
- Visual Studio Community 2026 18.9 ;
- MSVC 19.51.36256.0, toolset v145 ;
- SDK Windows 10.0.26100.0 ;
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

La compilation de l’analyseur produit les mêmes métriques sur Windows et GNU :

```text
7 unités
59 548 octets de code
639 octets de données
16 octets zéro
146 symboles
508 relocalisations
```

L’édition de liens de `AnalyseurDeclarations.GsE` contient 209 symboles et
543 relocalisations.

## Reproductibilité des images auto-hébergées

Les images MSVC et GNU sont identiques bit à bit :

| Image | Taille | SHA-256 |
| --- | ---: | --- |
| `ClassificateurMotsCles.GsE` | 12 300 octets | `aa1416f68e369e7ef89cccaea785c1d408cd941431c52bca8b5336c692199f93` |
| `Lexeur.GsE` | 40 897 octets | `39111f5550c47e434f8263064f67d4bf90546e81510f004bb518aa36330f660b` |
| `AnalyseurDeclarations.GsE` | 80 449 octets | `d49fcfddfa5a4bd57ccf61c819898eb41a2d855631c582da9d11a5b8c71b67f8` |

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

## Distribution

La construction autonome installe trois images auto-hébergées :

- `ClassificateurMotsCles.GsE` ;
- `Lexeur.GsE` ;
- `AnalyseurDeclarations.GsE`.

Les paquets Windows et Linux comprennent aussi les outils, le SDK, les
bibliothèques Gs++, les exemples, la documentation Markdown et la licence
MPL-2.0 canonique.

Chaque archive a été extraite dans un dossier neuf. Les outils extraits
annoncent `0.27.0-alpha.4`, le vérificateur accepte l’analyseur livré et le
compilateur produit un GsObj valide de 347 octets de code à partir de l’exemple
français sous Windows comme de l’exemple anglais sous GNU/Linux.

## Conclusion

Les catégories de déclarations, y compris les membres exécutables de classes,
de Gs++ 0.27.0-alpha.4 sont `VALIDÉES` sous MSVC et GNU. Le frontend 0.27
reste `PARTIEL` : la prochaine tranche doit construire l’AST des instructions,
puis celui des expressions, avant toute revendication d’analyseur syntaxique
auto-hébergé complet.
