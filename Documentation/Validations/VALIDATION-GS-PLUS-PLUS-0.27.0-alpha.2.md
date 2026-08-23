# Validation Gs++ 0.27.0-alpha.2

**VALIDÉ POUR LA TRANCHE AST DES DÉCLARATIONS — FRONTEND 0.27 PARTIEL —
23 août 2026.**

## Objet

Gs++ 0.27.0-alpha.2 poursuit la migration du frontend après le lexeur de
l’alpha.1. Cette préversion ajoute une représentation AST compacte et un
analyseur auto-hébergé pour les fonctions libres, leurs paramètres et leurs
espaces de noms.

Cette validation ne déclare pas l’analyseur syntaxique complet. Les structures,
classes, unions, énumérations, alias, variables globales, instructions et
expressions restent à migrer. Le statut global du frontend 0.27 demeure donc
`PARTIEL`.

## Périmètre livré

Les nouveaux fichiers canoniques sont :

- `AutoHebergement/AnalyseurDeclarations/AnalyseurDeclarations.HGsPP` ;
- `AutoHebergement/AnalyseurDeclarations/AnalyseurDeclarations.GsPP`.

L’export de l’image auto-hébergée est :

```text
Gs::Autohebergement::AnalyserDeclarationsSource(
    RequeteAnalyseDeclarations*) -> ErreurAnalyseDeclarations
```

Le contrat couvre :

- les fonctions libres avec corps ;
- les déclarations de fonctions externes ;
- les paramètres ;
- les espaces de noms simples, imbriqués et qualifiés ;
- les types natifs français et anglais ;
- les types utilisateur qualifiés ;
- `constante`/`const`, `volatile`, pointeurs et références ;
- les tableaux fixes et multidimensionnels ;
- l’interrogation de capacité et la sortie partielle bornée ;
- les erreurs lexicales distinctes des erreurs syntaxiques ;
- les positions de diagnostic ligne/colonne.

Les corps sont actuellement délimités par leurs accolades, mais leur arbre
d’instructions et d’expressions n’est pas encore construit.

## Contrat ABI de l’AST

Les dispositions validées sur x86-64 sont :

| Structure | Taille |
| --- | ---: |
| `NoeudDeclaration` | 64 octets |
| `ResultatAnalyseDeclarations` | 48 octets |
| `RequeteAnalyseDeclarations` | 80 octets |

Un nœud contient le genre, la position, les drapeaux, l’index du parent, la
tranche et le hachage du nom, l’empreinte de l’espace et l’empreinte normalisée
du type. Les formes françaises et anglaises d’un même type produisent la même
empreinte structurelle.

Les jetons et les nœuds de travail sont construits dans `AreneMemoire`. Le
stockage final appartient à l’appelant. Chaque appel détruit son arène avant le
retour, y compris après une erreur ou une capacité insuffisante.

## Preuve différentielle

`gspp_autohebergement_tests` charge réellement
`AnalyseurDeclarations.GsE`, résout ses deux imports d’hôte et compare ses
nœuds aux objets `Programme`, `Fonction` et `Parametre` produits par le
bootstrap C++.

La matrice comprend :

- deux corpus français/anglais structurellement équivalents ;
- un espace qualifié avec tableaux fixes et multidimensionnels ;
- des types utilisateur qualifiés, pointeurs et références ;
- une capacité nulle, une capacité partielle et une capacité exacte ;
- une requête nulle et une source nulle incohérente ;
- une parenthèse de paramètres manquante ;
- un nom de fonction manquant ;
- une accolade de corps manquante ;
- une erreur lexicale propagée avec son code détaillé ;
- le refus explicite d’une variable globale encore hors périmètre ;
- l’absence de libération invalide ou d’allocation résiduelle.

Les trois diagnostics syntaxiques produisent la même ligne et la même colonne
que le bootstrap C++.

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
3/3 gspp_conformite              Passed

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
3/4 gspp_conformite              Passed
4/4 gspp_integration             Passed

100% tests passed, 0 tests failed out of 4
```

La conformité portable reste réussie à **20/20** sur les deux chaînes.

## Reproductibilité des images auto-hébergées

Les images MSVC et GNU sont identiques bit à bit :

| Image | Taille | SHA-256 |
| --- | ---: | --- |
| `Lexeur.GsE` | 40 897 octets | `0f9b48bbd55042fc20e9eb2e871275e31faac8fe6e2fc78829788d1e78cc9fe7` |
| `AnalyseurDeclarations.GsE` | 58 897 octets | `833506013c3fa46e0e5700abb491ad85d8592ca6cd5321a3e492fd82a3572915` |

Le vérificateur confirme pour `AnalyseurDeclarations.GsE` :

```text
GsE 1.0, 3 segment(s), 8 section(s), 2 import(s), 69 export(s)
GsE valide.
```

Les deux imports sont exclusivement :

- `Gs::Hote::AllouerMemoire` ;
- `Gs::Hote::LibererMemoire`.

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
D:\GSLSE\Construction\GsPlusPlus-Public\Validation-0.27.0-alpha.2\
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

La construction autonome installe désormais trois images auto-hébergées :

- `ClassificateurMotsCles.GsE` ;
- `Lexeur.GsE` ;
- `AnalyseurDeclarations.GsE`.

Les paquets Windows et Linux comprennent aussi les outils, le SDK, les
bibliothèques Gs++, les exemples, la documentation Markdown et la licence
MPL-2.0 canonique.

## Conclusion

La tranche AST des déclarations de Gs++ 0.27.0-alpha.2 est `VALIDÉE` sous
MSVC et GNU. Elle constitue une progression vérifiable de l’auto-hébergement,
mais le frontend 0.27 reste `PARTIEL`. La prochaine tranche doit étendre l’AST
aux autres déclarations, puis aux instructions et expressions avant toute
revendication d’analyseur syntaxique auto-hébergé complet.
