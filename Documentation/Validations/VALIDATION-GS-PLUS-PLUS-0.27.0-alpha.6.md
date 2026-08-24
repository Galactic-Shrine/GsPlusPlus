# Validation Gs++ 0.27.0-alpha.6

**VALIDÉ POUR L’AST SYNTAXIQUE DES DÉCLARATIONS, INSTRUCTIONS ET EXPRESSIONS —
FRONTEND 0.27 PARTIEL — 25 août 2026.**

## Objet

Gs++ 0.27.0-alpha.6 poursuit la migration du frontend auto-hébergé. Après le
lexeur, les déclarations de code et de données, les membres exécutables de
classes, puis les blocs et instructions, cette préversion construit en Gs++
l’AST interne des expressions reconnu par le frontend bootstrap C++.

Cette validation ne déclare pas encore le frontend 0.27 complet. L’AST
syntaxique de la tranche actuelle est comparé au bootstrap, mais les premières
résolutions sémantiques restent assurées par le code C++. Le statut global du
frontend demeure donc `PARTIEL`.

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

### Genres d’expressions

L’alpha.6 ajoute onze genres publics sans renuméroter les valeurs 0 à 21 :

| Valeur | Français | Anglais | Charge utile |
| ---: | --- | --- | --- |
| 22 | `LitteralEntier` | `IntegerLiteral` | valeur dans `HachageType` |
| 23 | `LitteralChaine` | `StringLiteral` | hachage du texte décodé |
| 24 | `ReferenceVariable` | `VariableReference` | nom simple ou qualifié |
| 25 | `ExpressionUnaire` | `UnaryExpression` | opérateur et un enfant |
| 26 | `ExpressionBinaire` | `BinaryExpression` | opérateur et deux enfants |
| 27 | `Affectation` | `Assignment` | cible et valeur |
| 28 | `Appel` | `Call` | cible puis arguments |
| 29 | `AccesMembre` | `MemberAccess` | membre et objet |
| 30 | `Indexation` | `Index` | objet et indice |
| 31 | `Conversion` | `Cast` | type cible et valeur |
| 32 | `Agregat` | `Aggregate` | éléments dans l’ordre source |

Le parcours est préordre. Une expression racine est enfant de la déclaration,
du membre exécutable ou de l’instruction qui la porte. Chaque sous-expression
est ensuite enfant de son opérateur, appel, accès, indexation, conversion ou
agrégat. Le parent est toujours émis avant l’enfant.

Les priorités reproduites sont, de la plus faible à la plus forte :

1. affectation `=` associative à droite ;
2. `||` ;
3. `&&` ;
4. `|` ;
5. `^` ;
6. `&` ;
7. `==` et `!=` ;
8. `<`, `<=`, `>` et `>=` ;
9. `<<` et `>>` ;
10. `+` et `-` ;
11. `*`, `/` et `%` ;
12. unaires `+`, `-`, `!`, `~`, `&` et `*` ;
13. postfixes appel, indexation et accès membre.

Les drapeaux supplémentaires sont :

| Bit | Français | Anglais |
| ---: | --- | --- |
| 16384 | `LitteralBooleen` | `BooleanLiteral` |
| 32768 | `AccesViaPointeur` | `PointerMemberAccess` |
| 65536 | `ReferenceBase` | `BaseReference` |

Les mots-clés `soi` / `this` et `parent` / `super` partagent le nom canonique
`soi`. Le dernier couple est distingué par `ReferenceBase`. Les chaînes
conservent leur tranche littérale source, mais leur hachage porte le texte
décodé, conformément au lexeur.

### Porteurs d’expressions

La tranche émet désormais les expressions attachées aux :

- variables globales ;
- valeurs explicites d’énumérations ;
- valeurs par défaut de champs de classes ;
- arguments de délégation, d’initialisation de base et de champs des
  constructeurs ;
- retours ;
- instructions d’expression ;
- initialiseurs ou arguments de construction des variables locales ;
- conditions de `si` et de `tantque`.

## Contrat ABI de l’AST

Les dispositions x86-64 restent compatibles avec les alpha.2 à alpha.5 :

| Structure | Taille |
| --- | ---: |
| `NoeudDeclaration` / `NoeudSyntaxique` | 64 octets |
| `ResultatAnalyseDeclarations` | 48 octets |
| `RequeteAnalyseDeclarations` | 80 octets |

Les genres 0 à 21 et les drapeaux 1 à 8192 ne sont pas renumérotés. Les
empreintes de types et d’espaces de noms restent normalisées entre les syntaxes
française et anglaise.

Les expressions de travail utilisent une représentation interne distincte,
allouée dans `AreneMemoire`, avec liens premier enfant, dernier enfant et enfant
suivant. Seul le nœud compact de 64 octets est exposé à l’appelant. Les jetons,
nœuds de déclaration et nœuds d’expression de travail sont détruits après
chaque appel, y compris après une erreur, une interrogation de capacité ou une
sortie partielle.

## Preuve différentielle

`gspp_autohebergement_tests` charge réellement
`AnalyseurDeclarations.GsE`, résout ses deux imports d’hôte et compare chaque
nœud au `Programme` produit par `GsPP::AnalyseurSyntaxique`.

La matrice alpha.6 conserve les corpus des tranches précédentes et ajoute deux
corpus d’expressions structurellement équivalents, l’un en français et l’autre
en anglais. Ils couvrent :

- les onze genres 22 à 32 ;
- les six opérateurs unaires ;
- les dix-huit opérateurs binaires ;
- l’affectation associative à droite ;
- appels avec arguments, accès membre direct et via pointeur, indexation,
  conversion, agrégat et nom qualifié ;
- littéraux entiers, booléens et chaînes échappées ;
- références `soi` / `this` et `parent` / `super` ;
- expressions rattachées aux déclarations, constructeurs, instructions,
  retours, conditions et boucles ;
- genre, position, drapeaux, empreintes de nom, d’espace et de type ;
- ordre préfixe et validité de chaque relation parent-enfant.

Onze diagnostics supplémentaires sont comparés au bootstrap avec ligne et
colonne identiques : opérande binaire absent, parenthèse d’expression absente,
crochet d’indexation absent, indice absent, membre absent, parenthèse d’appel
absente, chevrons ouvrant ou fermant de conversion absents, parenthèse ouvrante
de conversion absente, accolade d’agrégat absente et dépassement d’un
`naturel64`. Avec les vingt-deux diagnostics des tranches précédentes,
trente-trois cas syntaxiques sont maintenant comparés.

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
83 675 octets de code
639 octets de données
24 octets zéro
174 symboles
773 relocalisations
```

L’édition de liens de `AnalyseurDeclarations.GsE` contient 237 symboles et
808 relocalisations.

## Reproductibilité des images auto-hébergées

Les images MSVC et GNU sont identiques bit à bit :

| Image | Taille | SHA-256 |
| --- | ---: | --- |
| `ClassificateurMotsCles.GsE` | 12 300 octets | `0a352f898d1ed752ae9b8130c1b5d8e70a2fa56479667f7e61ddf9b3aaddd9ec` |
| `Lexeur.GsE` | 40 897 octets | `3cc82b52d5d55c00284a8074fb5fc8fad6b71aa4ddb6c2437506f68af812d5d2` |
| `AnalyseurDeclarations.GsE` | 104 721 octets | `d568b98d27e20cce133ec6134d0a37f6484865bec09b5950c5d67496bd8164f3` |

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
bibliothèques Gs++, les exemples, le logo, la documentation Markdown et la
licence MPL-2.0 canonique.

Chaque archive a été extraite dans un dossier neuf. Les outils extraits
annoncent `0.27.0-alpha.6`, le vérificateur accepte l’analyseur livré et le
compilateur produit un GsObj valide à partir d’un exemple fourni. Le logo
installé conserve l’empreinte
`1ecde5344e1292e21722241f398e6922abd2021ff57b330b89113fb56629bb7a` sur
les deux plateformes et les deux README résolvent le chemin relatif
`Assets/Gs++.png`.

La matrice locale consolidée est conservée sous :

```text
D:\GSLSE\Construction\GsPlusPlus-Public\Validation-0.27.0-alpha.6\
```

## Conclusion

L’AST syntaxique des déclarations, instructions et expressions de Gs++
0.27.0-alpha.6 est `VALIDÉ` sous MSVC et GNU. Le frontend 0.27 reste `PARTIEL` :
la prochaine tranche doit migrer les premières résolutions sémantiques avant
toute revendication de frontend auto-hébergé complet.
