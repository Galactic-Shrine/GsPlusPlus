# Format XML des projets et solutions Gs++ 1.0

**NORMATIF — format 1.0 — Gs++ 0.27.0-alpha.1.**

Les fichiers `.GsPj` et `.GsProject` décrivent un projet Gs++. Les fichiers
`.GsPs` décrivent une solution, c’est-à-dire une séquence ordonnée de projets.
Depuis Gs++ 0.27.0-alpha.1, ces trois extensions utilisent exclusivement le
profil XML défini ci-dessous. L’ancien format texte `clé = valeur` est refusé.

Le numéro `Version="1.0"` versionne le schéma de projet ou de solution. Il est
indépendant des formats binaires GsObj, GsA et GsE, qui restent eux aussi en
version 1.0, et du champ ABI, qui reste à 1.

## Règles XML prises en charge

- encodage UTF-8, avec ou sans marque BOM ;
- déclaration `<?xml version="1.0" encoding="UTF-8"?>` facultative ;
- noms d’éléments et d’attributs sensibles à la casse ;
- valeurs d’attribut obligatoirement placées entre guillemets simples ou
  doubles ;
- entités nommées `&amp;`, `&lt;`, `&gt;`, `&quot;` et `&apos;` ;
- commentaires XML autorisés entre les éléments ;
- aucun texte libre, espace de noms, DTD, instruction de traitement ou élément
  non défini par ce document.

Un attribut inconnu, dupliqué ou fourni à la fois sous son nom français et son
alias anglais est une erreur. Tous les chemins relatifs sont résolus par
rapport au fichier `.GsPj`, `.GsProject` ou `.GsPs` qui les contient.

## Projet français `.GsPj`

```xml
<?xml version="1.0" encoding="UTF-8"?>
<GsProjet
    Version="1.0"
    Nom="Application"
    Type="executable"
    ModeCompilation="separee"
    PointEntree="Exemple::Principal"
    VersionApplication="1.0.0"
    Editeur="Galactic Shrine">
    <Interface Chemin="Calculs.HGsPP" />
    <Source Chemin="Principal.GsPP" />
    <Bibliotheque Chemin="Mathematiques.GsA" />
    <Construction
        RepertoireObjets="Construction/Objets"
        Sortie="Construction/Application.GsE"
        Carte="Construction/Application.map" />
</GsProjet>
```

## Projet anglais `.GsProject`

```xml
<?xml version="1.0" encoding="UTF-8"?>
<GsProject
    Version="1.0"
    Name="Application"
    Type="executable"
    CompilationMode="separate"
    EntryPoint="Example::Main"
    ApplicationVersion="1.0.0"
    Publisher="Galactic Shrine">
    <Interface Path="Calculations.HeaderGsPlusPlus" />
    <Source Path="Main.GsPlusPlus" />
    <Library Path="Mathematics.GsA" />
    <Build
        ObjectDirectory="Construction/Objects"
        Output="Construction/Application.GsE"
        Map="Construction/Application.map" />
</GsProject>
```

Les vocabulaires français et anglais sont des alias strictement équivalents et
peuvent être utilisés avec l’une ou l’autre racine. Il est cependant recommandé
de conserver un vocabulaire homogène dans un même fichier.

## Attributs du projet

| Français | Anglais | Obligatoire | Rôle |
|---|---|---:|---|
| `Version` | `Version` | oui | doit valoir exactement `1.0` |
| `Nom` | `Name` | non | nom du produit ; le nom du fichier est utilisé par défaut |
| `Type` | `Type` | non | `executable` par défaut, `bibliotheque` ou `library` pour une GsA |
| `ModeCompilation` | `CompilationMode` | non | `separee`/`separate` par défaut ou `agregee`/`aggregate` |
| `PointEntree` | `EntryPoint` | non | symbole public à utiliser comme point d’entrée du GsE |
| `VersionApplication` | `ApplicationVersion` | non | version inscrite dans les métadonnées GsE |
| `Editeur` | `Publisher` | non | éditeur inscrit dans les métadonnées GsE |

`VersionApplication` décrit l’application compilée, pas la version du format
XML ni celle de l’ABI. Lorsqu’il est omis, le compilateur utilise la version du
produit Gs++ lue depuis le fichier racine `VERSION`.

Les variantes accentuées `séparée` et `agrégée` sont également acceptées.

## Éléments enfants du projet

| Français | Anglais | Multiplicité | Attribut de chemin |
|---|---|---:|---|
| `Source` | `Source` | une ou plusieurs | `Chemin` ou `Path` |
| `Interface` | `Interface` | zéro ou plusieurs | `Chemin` ou `Path` |
| `Bibliotheque` | `Library` | zéro ou plusieurs | `Chemin` ou `Path` |
| `Construction` | `Build` | zéro ou un | voir ci-dessous |

Un projet doit contenir au moins une source. Chaque élément enfant est vide et
doit donc être fermé avec `/>` ou une balise fermante sans contenu.

`Construction`/`Build` accepte les trois paires d’alias facultatives :

| Français | Anglais | Rôle |
|---|---|---|
| `RepertoireObjets` | `ObjectDirectory` | répertoire des `.GsObj` intermédiaires |
| `Sortie` | `Output` | chemin du `.GsA` ou du `.GsE` produit |
| `Carte` | `Map` | carte de liens déterministe d’un exécutable |

Lorsque ces chemins ne sont pas fournis, le compilateur utilise un répertoire
`construction` voisin du fichier de projet.

## Solution `.GsPs`

```xml
<?xml version="1.0" encoding="UTF-8"?>
<GsSolution Version="1.0">
    <Projet Chemin="Bibliotheque.GsPj" />
    <Project Path="Application.GsProject" />
</GsSolution>
```

La racine `GsSolution` accepte uniquement l’attribut `Version="1.0"`. Chaque
enfant est un `Projet Chemin="…"` ou un `Project Path="…"`. Au moins un projet
est requis. Les projets sont construits dans l’ordre du document ; une
application peut ainsi lier une bibliothèque produite par le projet précédent.

```bash
Construction/Bin/gsppc Tests/Integration/Separation/Compilation.GsPs
```

## Compatibilité

Le passage à XML est volontairement incompatible avec le prototype texte. Les
projets étant encore locaux au moment de cette décision, aucune migration à
l’exécution n’est fournie : les anciens fichiers doivent être réécrits en XML.
Cette rupture ne change ni les extensions propriétaires ni les signatures
internes `GSOBJ:0`, `GSA:0` et `GSE:0`.
