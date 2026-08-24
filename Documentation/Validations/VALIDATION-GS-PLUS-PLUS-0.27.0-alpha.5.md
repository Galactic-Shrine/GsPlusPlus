# Validation Gs++ 0.27.0-alpha.5

**VALIDÉ POUR LES DÉCLARATIONS ET LA HIÉRARCHIE DES INSTRUCTIONS — FRONTEND
0.27 PARTIEL — 24 août 2026.**

## Objet

Gs++ 0.27.0-alpha.5 poursuit la migration du frontend auto-hébergé. Après le
lexeur, les déclarations de code et de données et les membres exécutables de
classes, cette préversion ajoute l’AST des blocs et instructions écrit en Gs++.

Cette validation ne déclare pas encore l’analyseur syntaxique complet. Les
expressions sont correctement délimitées et leur présence est conservée, mais
leur AST interne reste à migrer. Les premières étapes sémantiques sont
également encore assurées par le bootstrap C++. Le statut global du frontend
0.27 demeure donc `PARTIEL`.

## Périmètre livré

Les fichiers canoniques restent :

- `AutoHebergement/AnalyseurDeclarations/AnalyseurDeclarations.HGsPP` ;
- `AutoHebergement/AnalyseurDeclarations/AnalyseurDeclarations.GsPP`.

L’export historique compatible reste :

```text
Gs::Autohebergement::AnalyserDeclarationsSource(
    RequeteAnalyseDeclarations*) -> ErreurAnalyseDeclarations
```

Les alias publics `AnalyserSyntaxeSource` / `AnalyzeSourceSyntax` offrent un
nom générique au même contrat. `NoeudSyntaxique` / `SyntaxNode`,
`ResultatAnalyseSyntaxique` / `SyntaxAnalysisResult` et
`RequeteAnalyseSyntaxique` / `SyntaxAnalysisRequest` restent des alias ABI des
structures compactes existantes.

L’alpha.5 ajoute six genres publics :

| Valeur | Français | Anglais |
| ---: | --- | --- |
| 16 | `BlocInstructions` | `StatementBlock` |
| 17 | `InstructionRetour` | `ReturnStatement` |
| 18 | `InstructionExpression` | `ExpressionStatement` |
| 19 | `VariableLocale` | `LocalVariable` |
| 20 | `Conditionnelle` | `Conditional` |
| 21 | `BoucleTantQue` | `WhileLoop` |

Le parcours est préordre : le bloc du corps est enfant de sa fonction ou de
son membre exécutable ; chaque instruction est enfant de son bloc ; les corps
et branches sont enfants de la conditionnelle ou de la boucle qui les porte.
Les blocs et instructions sans nom conservent une tranche et un hachage de nom
nuls. Une variable locale conserve sa tranche source, son hachage de nom et
son empreinte de type normalisée.

Les drapeaux supplémentaires sont :

| Bit | Français | Anglais |
| ---: | --- | --- |
| 2048 | `ExpressionPresente` | `HasExpression` |
| 4096 | `BrancheSinonPresente` | `HasElseBranch` |
| 8192 | `ConstructionExplicite` | `HasExplicitConstruction` |

Le bit 4 existant décrit aussi la présence de l’initialiseur d’une variable
locale. Il est combiné au bit 2048 parce que cet initialiseur contient une
expression.

## Contrat ABI de l’AST

Les dispositions x86-64 restent compatibles avec les alpha.2 à alpha.4 :

| Structure | Taille |
| --- | ---: |
| `NoeudDeclaration` / `NoeudSyntaxique` | 64 octets |
| `ResultatAnalyseDeclarations` | 48 octets |
| `RequeteAnalyseDeclarations` | 80 octets |

Les genres 0 à 15 et les drapeaux 1 à 1024 ne sont pas renumérotés. Les
empreintes de types et d’espaces de noms restent normalisées entre les syntaxes
française et anglaise.

Les jetons et nœuds de travail sont alloués dans `AreneMemoire`. Le stockage
final appartient à l’appelant. Les contrôles vérifient que toutes les arènes
sont détruites, y compris après une erreur, une interrogation de capacité ou
une sortie partielle.

## Preuve différentielle

`gspp_autohebergement_tests` charge réellement
`AnalyseurDeclarations.GsE`, résout ses deux imports d’hôte et compare chaque
nœud au `Programme` produit par `GsPP::AnalyseurSyntaxique`.

La matrice alpha.5 ajoute deux corpus d’instructions structurellement
équivalents, l’un en français et l’autre en anglais, qui couvrent :

- cinq blocs, dont le corps de fonction et quatre blocs imbriqués ;
- trois retours, avec et sans expression ;
- trois instructions d’expression ;
- quatre variables locales : sans initialiseur, initialisées par expression ou
  agrégat, et construites explicitement ;
- deux conditionnelles imbriquées, toutes deux avec branche `sinon` ;
- une boucle `tantque` ;
- tous les parents de la hiérarchie fonction-bloc-contrôle-branche ;
- les genres, positions, drapeaux, empreintes de nom, d’espace et de type.

Huit diagnostics supplémentaires sont comparés au bootstrap avec ligne et
colonne identiques : condition vide de `si`, condition vide de `tantque`,
parenthèse ouvrante absente, nom de variable locale absent, instruction
d’expression vide, point-virgule local absent, point-virgule de retour absent
et point-virgule d’expression absent. Avec les quatorze diagnostics des
tranches précédentes, vingt-deux cas syntaxiques sont maintenant comparés.

Les requêtes nulles ou incohérentes, les capacités nulle, partielle et exacte,
la propagation des erreurs lexicales, les libérations invalides et les
allocations résiduelles restent également contrôlées.

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
66 441 octets de code
639 octets de données
16 octets zéro
150 symboles
554 relocalisations
```

L’édition de liens de `AnalyseurDeclarations.GsE` contient 213 symboles et
589 relocalisations.

## Reproductibilité des images auto-hébergées

Les images MSVC et GNU sont identiques bit à bit :

| Image | Taille | SHA-256 |
| --- | ---: | --- |
| `ClassificateurMotsCles.GsE` | 12 300 octets | `c966ad3a6d15a0af95b665c443844dd78cecf810756a9d46038ae3e23b61ea5c` |
| `Lexeur.GsE` | 40 897 octets | `0acf3de55700a6a2049b2f88ddcb8c2515eb3d37bc3f5bb22ef71c4916e81114` |
| `AnalyseurDeclarations.GsE` | 87 489 octets | `e782093f0c700eb8aeb36875d2891e88acae57eb1f796e01b304aa229cd0081a` |

Le vérificateur confirme pour `AnalyseurDeclarations.GsE` :

```text
GsE 1.0, 3 segment(s), 8 section(s), 2 import(s), 71 export(s)
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
annoncent `0.27.0-alpha.5`, le vérificateur accepte l’analyseur livré et le
compilateur produit un GsObj valide à partir d’un exemple fourni sous Windows
comme sous GNU/Linux.

## Conclusion

Les déclarations et la hiérarchie des instructions de Gs++
0.27.0-alpha.5 sont `VALIDÉES` sous MSVC et GNU. Le frontend 0.27 reste
`PARTIEL` : la prochaine tranche doit construire l’AST des expressions, puis
les premières étapes sémantiques, avant toute revendication d’analyseur
syntaxique auto-hébergé complet.
