# Compilation séparée de Gs++ 0.13

## Vue d’ensemble

Gs++ 0.13 sépare quatre opérations qui étaient auparavant confondues :

```text
interface + source
        │
        ▼
   compilation
        │
        ▼
    GsObj 1.0 ─────► GsA 1.0
        │               │
        └──────┬────────┘
               ▼
       édition de liens
               │
               ▼
             GsE 1.0
```

Un objet `GsObj` est autonome : il contient les octets des sections, les
symboles, les relocalisations et les contrats nécessaires à une liaison
ultérieure. L’éditeur de liens n’a donc pas besoin des AST ni des sources des
autres unités.

`GsObj 1.0` désigne le format binaire interne et sa signature `GSOBJ:0`. Un
octet réservé nul complète les huit premiers octets. Les fichiers qui le
contiennent portent l’extension `.GsObj`.

## Interfaces

Les extensions reconnues sont :

- `.HGs++` ;
- `.HGsPP` ;
- `.HeaderGsPlusPlus`.

Les extensions historiques `.GsPPH` et `.GsPlusPlusHeader` ont été retirées
en 0.22.1 et sont désormais refusées.

Une interface utilise la syntaxe habituelle du langage. Un prototype terminé
par `;` et une variable globale sans initialiseur deviennent implicitement des
déclarations externes ; le mot-clé `externe`/`extern` reste accepté mais n’est
pas nécessaire.

```gspp
espace Exemple::Calculs
{
    structure Resultat
    {
        entier32 Valeur;
        booléen Valide;
    };

    publique entier32 Doubler(entier32 valeur);
    publique entier32 CompteurPartage;
}
```

L’interface peut être fournie avec l’implémentation. Le compilateur rapproche
alors la déclaration externe et la définition du même nom. Deux prototypes
différents sont refusés avant la génération de code.

```bash
Construction/Bin/gsppc Calculs.HGsPP Calculs.GsPP \
    --format gsobj -o Calculs.GsObj
```

Elle peut aussi être fournie à un consommateur :

```bash
Construction/Bin/gsppc Calculs.HGsPP Principal.GsPP \
    --format gsobj -o Principal.GsObj
```

## Format objet GsObj 1.0

Tous les nombres du format sont non signés et encodés en petit-boutiste, sauf
indication contraire. L’architecture est AMD64 (`0x8664`) et l’ABI courante
est l’extension Gs++ de Microsoft x64, version 1.

L’en-tête de 112 octets commence par les sept octets `GSOBJ:0`, suivis d’un
octet réservé nul pour conserver l’alignement, puis contient :

- la version majeure et mineure ;
- l’architecture et la version ABI ;
- le nombre de symboles et de relocalisations ;
- les tailles de `.texte`, `.donnees` et `.zero` ;
- la position de chaque bloc et de la table de chaînes.

Une entrée de symbole de 48 octets conserve :

- le nom UTF-8 ;
- la nature `fonction` ou `objet` ;
- la section et les drapeaux défini/public ;
- la position et la taille dans la section ;
- la signature ABI canonique ;
- le fichier, la ligne et la colonne de la déclaration.

Une entrée de relocalisation de 24 octets conserve :

- la position dans la section source ;
- l’index du symbole cible ;
- la section source ;
- le type `Relatif32` ou `Adresse64` ;
- un ajout réservé, nul dans cette version.

Les noms de symboles sont limités à 1 024 octets UTF-8. Les signatures ABI et
chemins source utilisent des longueurs sur 16 bits. Le lecteur contrôle toutes
les plages, index, terminaisons nulles, chaînes UTF-8, sections, tailles de
symboles et largeurs de relocalisation avant de produire un `CodeMachine`.

`GsObj 1.0` est un format interne versionné. Une évolution incompatible devra
augmenter sa version majeure.

## Contrat ABI entre unités

Chaque fonction et chaque globale possède une signature commençant par :

```text
GsAbi:x64-ms-v1:
```

La signature encode :

- la nature fonction ou objet ;
- le type de retour et les paramètres ;
- signedness, largeur, pointeurs, tableaux, `const` et `volatile` ;
- le nom qualifié des structures et énumérations ;
- pour une structure, taille, alignement, nature structure/union, champs,
  décalages et types récursifs.

Le lien refuse un symbole lorsqu’une déclaration et une définition n’ont pas
la même nature ou la même signature. Le diagnostic indique les deux positions
source, par exemple :

```text
incompatibilité ABI pour Exemple::Doubler entre
Interface.HGsPP:3:5 et Calculs.GsPP:5:5
```

Une définition publique ne peut apparaître qu’une fois. Les symboles privés
reçoivent un nom local propre à leur objet pendant la liaison, ce qui permet à
deux unités d’utiliser le même nom privé sans collision.

## Bibliothèques statiques GsA 1.0

Une bibliothèque commence par `GSA:0` complété à huit octets, sa version, le nombre de membres et sa
taille totale. Chaque membre possède un nom UTF-8 et un objet `GsObj` complet,
alignés sur 16 octets. Le lecteur valide chaque objet avant de l’accepter.

```bash
Construction/Bin/gsppc Calculs.GsObj Trigonometrie.GsObj \
    --format gsa -o Mathematiques.GsA
```

Lors de la liaison, un membre est extrait seulement s’il définit publiquement
un symbole actuellement requis. Les nouvelles dépendances de ce membre sont
ensuite examinées jusqu’à stabilisation. Si plusieurs bibliothèques proposent
la même définition, l’ordre de la ligne de commande détermine la première
bibliothèque sélectionnée.

## Édition de liens et carte

```bash
Construction/Bin/gsppc Principal.GsObj Mathematiques.GsA \
    --format gse \
    --point-entree Exemple::Principal \
    --carte Application.map \
    -o Application.GsE
```

L’éditeur :

1. recherche les membres de bibliothèques requis ;
2. vérifie les contrats ABI et les définitions publiques ;
3. aligne et fusionne les sections de chaque objet ;
4. renomme les symboles privés localement ;
5. résout les références internes et conserve les véritables imports externes ;
6. transmet le résultat à l’écrivain GsE 1.0.

La carte de liens est un fichier texte déterministe. Elle indique les RVA des
sections et, pour chaque symbole défini, son adresse, sa visibilité, sa nature,
sa taille, sa position source et sa signature ABI.

## Projets GsPj et solutions GsPs

Depuis Gs++ 0.27.0-alpha.1, les projets utilisent le format XML 1.0 normatif
décrit dans
[`FORMAT_PROJETS_GS_PLUS_PLUS_1.0.md`](FORMAT_PROJETS_GS_PLUS_PLUS_1.0.md).
L’ancien prototype texte présenté dans la version initiale de ce document est
retiré. Les noms français et anglais suivants sont reconnus :

```xml
<?xml version="1.0" encoding="UTF-8"?>
<GsProjet
    Version="1.0"
    Nom="Application"
    Type="executable"
    PointEntree="Exemple::Principal"
    VersionApplication="0.27.0-alpha.1"
    Editeur="Galactic Shrine">
    <Interface Chemin="Calculs.HGsPP" />
    <Source Chemin="Principal.GsPP" />
    <Bibliotheque Chemin="Mathematiques.GsA" />
    <Construction
        RepertoireObjets="construction/objets"
        Sortie="construction/Application.GsE"
        Carte="construction/Application.map" />
</GsProjet>
```

Pour une bibliothèque, `Type="bibliotheque"` ou `Type="library"` produit une
`GsA`. Toutes les interfaces du projet sont analysées avec chacune de ses
sources, mais chaque source génère son propre objet. Les chemins relatifs sont
résolus par rapport au fichier de projet.

Une solution utilise une racine `GsSolution` et construit ses projets dans
l’ordre indiqué :

```xml
<?xml version="1.0" encoding="UTF-8"?>
<GsSolution Version="1.0">
    <Projet Chemin="Bibliotheque.GsPj" />
    <Project Path="Application.GsProject" />
</GsSolution>
```

```bash
Construction/Bin/gsppc Compilation.GsPs
```

L’ordre explicite permet à un projet d’application de consommer la sortie
d’une bibliothèque construite juste avant lui.

## Compatibilité

- le mode historique multi-source vers COFF ou GsE reste disponible ;
- GsE utilise la base canonique 1.0 et le chargeur UEFI partage ces constantes ;
- les objets COFF restent produits avec `--format obj` ;
- GsObj et GsA sont nouveaux en 0.13 et n’ont pas de lecteur dans les versions
  antérieures du compilateur.
- Gs++ 0.15 avait porté localement le champ ABI à 2 pour le passage et le
  retour des structures par valeur. Gs++ 0.17.1 conserve toutes ces règles,
  mais les renumérote comme première ABI canonique 1 ; les objets portant
  l’ancienne numérotation locale 2 sont refusés et doivent être recompilés.

Les pointeurs de fonction ne faisaient pas partie de 0.13.2. Ils sont ajoutés
en 0.14.0, avec leur signature récursive dans le contrat ABI GsObj ; deux unités
qui déclarent un même callback avec des paramètres ou un retour différents sont
donc refusées à la liaison.

## Mode agrégé pour les projets monolithiques

L’attribut facultatif suivant compile toutes les interfaces et sources du
projet dans une même unité avant l’édition de liens :

```xml
<GsProjet Version="1.0" ModeCompilation="agregee" Type="executable">
    <!-- interfaces, sources et construction -->
</GsProjet>
```

Son alias anglais est `CompilationMode="aggregate"`. Le mode historique reste
la compilation séparée par source. Sanctuaire SE peut utiliser le mode agrégé,
mais il ne fait pas partie des dépendances de construction ou de publication de
Gs++.
