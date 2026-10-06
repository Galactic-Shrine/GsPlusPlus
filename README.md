<div align="center">

<img src="Assets/Gs++.png" alt="Logo Gs++" width="200"/>

**Un langage système natif bilingue, du code source au code machine.**

[![Validation Gs++](https://github.com/Galactic-Shrine/GsPlusPlus/actions/workflows/validation.yml/badge.svg)](https://github.com/Galactic-Shrine/GsPlusPlus/actions/workflows/validation.yml)
[![Publication GitHub](https://img.shields.io/github/v/release/Galactic-Shrine/GsPlusPlus?include_prereleases&label=publication)](https://github.com/Galactic-Shrine/GsPlusPlus/releases)
[![Plateformes](https://img.shields.io/badge/plateformes-Windows%20%7C%20Linux-5865f2)](#construction)
[![Licence MPL-2.0](https://img.shields.io/badge/licence-MPL--2.0-blue.svg)](LICENSE)

[Français](README.md) · [English](README.en.md)

</div>

## Qu’est-ce que Gs++ ?

Gs++ est un langage de programmation système natif créé par
**⋞Galactic-Shrine⋟**. Il est destiné aux logiciels proches du matériel, aux
bibliothèques système et aux applications natives qui demandent une maîtrise
explicite des données, de la mémoire, de l’ABI et de la durée de vie des objets.

Le compilateur `gsppc` transforme directement les sources Gs++ en code machine.
Gs++ n’est pas un transpileur vers C++ : il possède son propre frontend, son
générateur x86-64, son éditeur de liens et ses formats binaires GsObj, GsA et
GsE.

Le français est la syntaxe canonique du langage. Les mots-clés anglais
documentés sont des alias officiels avec la même sémantique et la même
génération de code.

> **État actuel des sources — `0.27.0-alpha.10`**
>
> Cette préversion permet d’évaluer et de développer avec la chaîne Gs++
> actuelle. Les formats binaires 1.0 et l’ABI 1 sont validés, mais le frontend
> auto-hébergé reste en développement. Son image unique `Frontend.GsE` regroupe
> le classificateur, le lexeur, l’AST compact et la passe sémantique. Celle-ci
> couvre maintenant les surcharges, membres, constructeurs, initialiseurs,
> agrégats imbriqués, indexations, adresses, déréférencements et appels
> indirects déjà typables. Le développement suivant l’alpha.8 valide aussi les
> valeurs gauches, indices, cibles, arités, types et références de ces formes,
> ainsi que la résolution typée des opérateurs libres binaires et unaires. Les
> vingt-quatre formes intrinsèques unaires et binaires sont aussi validées avec
> leurs adaptations de littéraux, pointeurs ordinaires ou de fonction et neuf
> familles de diagnostics. La passe produit en outre les plans ordonnés de
> construction, de destruction et de tables virtuelles des objets locaux et
> sous-objets de constructeurs. Elle applique aussi les contraintes
> structurelles des déclarations globales aux objets de classe, références,
> types `vide`, imports publics et constantes non initialisées. Les
> initialiseurs globaux sont maintenant contrôlés récursivement : listes
> requises pour les agrégats, cibles directes des pointeurs de fonction, refus
> des pointeurs de données initialisés et formes constantes structurelles. Elle
> calcule désormais les constantes numériques, y compris les valeurs
> d’énumération implicites ou explicites, les opérations signées et non
> signées, les conversions et les courts-circuits logiques ; elle refuse les
> divisions par zéro et les valeurs hors plage. L’API `EmettreGlobales` produit
> maintenant les octets initiaux, la disposition des zones données/zéro et les
> relocalisations de fonctions, comparés au bootstrap. L’alpha.9 vérifie aussi
> les conversions explicites, les signatures de fonctions et les constantes
> converties hors plage. L'alpha.10 étend les adaptations
> implicites aux constantes composées et contrôle les surcharges et
> qualifications associées. Elle ajoute les signatures de callbacks imbriquées,
> la résolution contextuelle des types nommés, les contraintes de signatures,
> les alias de champs et les alias racines de types, fonctions et globales,
> puis les appels via alias de méthodes avec récepteur explicite `Classe&`.
> Les conversions implicites restantes et les autres
> familles sémantiques restent à migrer ;
> cette API ne remplace pas encore le backend ni l’écriture des fichiers objets.

## Principes du langage

| Principe | Ce que Gs++ fournit |
|---|---|
| Compilation native | Production directe de code machine x86-64 |
| Syntaxe bilingue | Français canonique et alias anglais équivalents |
| Programmation système | Pointeurs, structures, unions, tableaux, globales et atomiques |
| Modèle objet | Classes, visibilité, héritage simple, virtualité, constructeurs et destructeurs |
| Durée de vie explicite | Initialisation ordonnée, RAII et destruction déterministe |
| Compilation séparée | Interfaces, objets GsObj, bibliothèques GsA et contrôle ABI à la liaison |
| Profils d’exécution | Profil freestanding minimal et services hébergés explicitement liés |
| Auto-hébergement progressif | Composants du compilateur réécrits et validés en Gs++ |
| Reproductibilité | Formats versionnés, cartes de liens et matrice de conformité portable |

Les API livrées par Gs++ utilisent le préfixe d’espace de noms canonique
`GalacticShrine::GsPP::`. Par exemple, les services hébergés sont exposés sous
`GalacticShrine::GsPP::Hebergee` et les imports fournis par l’hôte sous
`GalacticShrine::GsPP::Hote`.

## Un premier programme

```cpp
espace Shrine::Exemples {

    /**
     * <résumé>Additionne deux entiers signés de 32 bits.</résumé>
     * @Paramètre(entier32: gauche) Première valeur.
     * @Paramètre(entier32: droite) Deuxième valeur.
     * @Retourner(entier32) Somme des deux valeurs.
     **/
    publique entier32 Additionner(entier32 gauche, entier32 droite) {

        retourner gauche + droite;
    }

    /**
     * <résumé>Exécute le programme d'exemple.</résumé>
     * @Retourner(entier32) Résultat de l'exécution.
     **/
    publique entier32 Principal() {

        entier32 résultat = Additionner(20, 22);

        si (résultat == 42) {
            retourner résultat;
        }

        retourner 0;
    }
}
```

La même API peut être écrite avec les alias anglais tels que `namespace`,
`public`, `return`, `if` et `else`.

### Inclure des fichiers et utiliser leurs noms

Les sources de développement après alpha.10 permettent également :

```cpp
#inclure "Types.HGsPP"
utilisant espace GalacticShrine::GsPP::Types;
```

Les alias anglais sont `#include "Types.HGsPP"` et
`using namespace GalacticShrine::GsPP::Types;`. L'inclusion insère les
déclarations ; l'utilisation permet d'écrire les noms sans leur préfixe.
`#pragma once` protège les inclusions répétées. Le projet XML conserve la
responsabilité de compiler les sources et de lier les bibliothèques.

Cet ajout fonctionne dans le bootstrap `gsppc` et possède un
[exemple bilingue exécutable](Exemples/Directives/Application.GsPj).
Les analyseurs auto-hébergés reconnaissent également les utilisations d'espaces
et résolvent leurs noms, alias et surcharges. L'expansion des fichiers inclus
reste assurée par le bootstrap hôte ; il ne s'agit pas d'un préprocesseur C++ complet. Les
[règles et limites actuelles](Documentation/SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md#inclusion-textuelle-et-utilisation-despaces-de-noms)
précisent les formes prises en charge. Les paquets alpha.10 publiés restent
inchangés.

## Chaîne de production

```text
Sources et interfaces
  .Gs++ / .GsPP / .GsPlusPlus
  .HGs++ / .HGsPP / .HeaderGsPlusPlus
                │
                ▼
              gsppc
                │
                ├── .GsObj  objet natif Gs++
                ├── .GsA    bibliothèque native Gs++
                └── .GsE    image exécutable Gs++
```

Les signatures canoniques sont `GSOBJ:0`, `GSA:0` et `GSE:0`. Les trois
formats binaires sont en version 1.0 et leurs champs ABI valent 1. La cible
actuelle utilise la signature de liaison `GsAbi:x64-ms-v1`.

### Compilation multi-cible prévue

Le plan produit prévoit une cible native par défaut et la possibilité de
choisir explicitement un autre système. Les premières cibles sont Windows,
GNU/Linux et la plateforme native Galactic-Shrine, sur x86-64. La chaîne doit
produire le format exécutable et utiliser les conventions de liaison et le
SDK de la destination ; la compilation croisée dépend de leur disponibilité.

Cette sélection et les sorties natives Windows PE (`.exe`) et Linux ELF sont
**prévues, non encore implémentées**. Les constructions Windows/Linux actuelles
valident le compilateur sur ces hôtes et le contrat Gs++ existant.
Le [plan produit](Documentation/PLAN_PRODUIT_GS_PLUS_PLUS_1.0.md) décrit cette
évolution et ses critères de validation.

### Séparation compilation / construction prévue

Pour le jalon **0.28**, un outil dédié est prévu sous le nom proposé
**GsBuild**, avec la commande `gsbuild`. Il prendra en charge les projets XML
`.GsPj`/`.GsProject` et les solutions `.GsPs`, puis pilotera les compilations,
la création des bibliothèques et l'édition de liens. `gsppc` se concentrera
uniquement sur la compilation des sources et interfaces Gs++ en objets.

**Non implémenté actuellement :** la chaîne 0.27 conserve les commandes
`gsppc` présentées ici. GsBuild aura un rôle analogue à celui de MSBuild pour
les projets Gs++, sans annoncer une compatibilité avec les projets MSBuild.
CMake et Visual Studio/MSBuild resteront utilisables pour construire la
toolchain elle-même. Le [plan produit](Documentation/PLAN_PRODUIT_GS_PLUS_PLUS_1.0.md#séparer-compilation-et-construction--décision-du-5-octobre-2026)
décrit cette séparation et la migration prévue.

## Extensions

| Usage | Extensions |
|---|---|
| Sources | `.Gs++`, `.GsPP`, `.GsPlusPlus` |
| Interfaces | `.HGs++`, `.HGsPP`, `.HeaderGsPlusPlus` |
| Projets | `.GsPj`, `.GsProject` |
| Solutions | `.GsPs` |
| Objets | `.GsObj` |
| Bibliothèques | `.GsA` |
| Exécutables | `.GsE` |

Prévu pour **0.28.0** : `.Glib` remplacera `.GsA` pour les bibliothèques
statiques ; `.GdLib` est réservé aux bibliothèques dynamiques si leur support
est introduit. `.GsE` reste inchangé. La chaîne 0.27 utilise encore `.GsA`.

Les projets et solutions utilisent un schéma XML strict en version 1.0 :

```xml
<?xml version="1.0" encoding="UTF-8"?>
<GsProjet Version="1.0" Nom="Bonjour" Type="executable">
    <Source Chemin="Bonjour.Gs++" />
    <Construction Sortie="Construction/Bonjour.GsE" />
</GsProjet>
```

Le vocabulaire XML anglais équivalent utilise `GsProject`, `Source Path` et
`Build Output`.

Les structures et énumérations peuvent avoir chacune leur propre interface
`.HGsPP`, déclarée dans le projet et fournie à chaque source compilée séparément.
L’exemple [TypesParFichier](Exemples/TypesParFichier/Application.GsPj) sépare
`Point`, `Etat` et leur utilisation ; son point d’entrée retourne 42.
La présence d’un fichier dans le dossier ne suffit pas à exposer ses types.

## Construction

### Prérequis

Pour la solution native, installer Visual Studio 2026 avec les outils C++
MSVC v145 et le SDK Windows. **CMake n'est pas requis dans ce mode.**
Les prérequis suivants concernent la construction CMake :

- CMake 4.2 ou plus récent sous Windows pour le générateur Visual Studio 2026 ;
- CMake 3.20 ou plus récent sous Linux ;
- un compilateur C++20 ;
- Python 3 pour la conformité ;
- Ninja, Bash et les outils GNU usuels pour l’intégration Linux.

### Windows — solution native Visual Studio 2026, sans CMake

Ouvrir [`GsPlusPlus.slnx`](GsPlusPlus.slnx), puis construire `Release | x64`.
Depuis un terminal développeur Visual Studio :

```powershell
msbuild GsPlusPlus.slnx /m /p:Configuration=Release /p:Platform=x64
msbuild VisualStudio/Validation.vcxproj /m /p:Configuration=Release /p:Platform=x64
```

Les sorties sont dans `Construction/MSBuild/x64/Release/`.
Voir [le guide Visual Studio](VisualStudio/README.md) pour les cibles et les tests.

### Windows — CMake avec Visual Studio 2026

```powershell
cmake --preset windows-release
cmake --build --preset windows-release --target espace_travail
ctest --preset windows-release
```

### Linux — GNU et Ninja

```bash
cmake --preset linux-release
cmake --build --preset linux-release --target espace_travail
ctest --preset linux-release
```

Les sorties CMake restent locales, ignorées par Git, dans
`Construction/CMake/...`. Les outils sont placés dans le
sous-dossier `Bin` et les bibliothèques Gs++ dans
`Artefacts/GsPlusPlus`.

Le fichier racine [`VERSION`](VERSION) est l’unique source de vérité technique
pour la version du produit. CMake et MSBuild la propagent aux outils et aux
métadonnées GsE. Les tests, le benchmark et les noms de paquets CMake utilisent
également ce fichier.

Après une construction Windows :

```powershell
./Construction/CMake/VisualStudio/Release/Bin/gsppc.exe `
  Exemples/Bonjour.Gs++ `
  --format gsobj `
  -o Bonjour.GsObj
```

Sous Linux :

```bash
./Construction/CMake/Ninja/Release/Bin/gsppc \
  Exemples/Bonjour.Gs++ \
  --format gsobj \
  -o Bonjour.GsObj
```

## Télécharger une préversion

La [release `0.27.0-alpha.10`](https://github.com/Galactic-Shrine/GsPlusPlus/releases/tag/v0.27.0-alpha.10)
propose des paquets x86-64 pour Windows et Linux. Chaque paquet contient les
outils, les en-têtes SDK, les bibliothèques Gs++, les exemples et la
documentation Markdown. Le fichier `SHA256SUMS.txt` permet de vérifier les
téléchargements.

Les contrôles des sources et des paquets extraits sont décrits dans la
[matrice alpha.10](Documentation/Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.10.md).

## Organisation du dépôt

```text
GsPlusPlus/
├── Compiler/          compilateur natif, éditeur de liens et outils GsE
├── SDK/               en-têtes des formats et contrats publics
├── Bibliotheques/     bibliothèques système et hébergée
├── AutoHebergement/   composants écrits en Gs++
├── Exemples/          programmes de découverte
├── Tests/             tests unitaires, intégration et conformité
├── Benchmarks/        mesures reproductibles
└── Documentation/     spécifications et preuves de validation
```

## Documentation

- [Index de la documentation courante](Documentation/README.md)
- [Notes de la dernière publication](RELEASE_NOTES.md)
- [Spécification candidate du langage 1.0](Documentation/SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md)
- [Conventions de code Gs++ 1.0](Documentation/CONVENTIONS_CODE_GS_PLUS_PLUS_1.0.md)
- [Format XML des projets et solutions 1.0](Documentation/FORMAT_PROJETS_GS_PLUS_PLUS_1.0.md)
- [Formats GsObj 1.0](Documentation/FORMAT_GSOBJ_1.0.md), [GsA 1.0](Documentation/FORMAT_GSA_1.0.md) et [GsE 1.0](Documentation/FORMAT_GSE_1.0.md)
- [ABI native x86-64](Documentation/ABI_GS_PLUS_PLUS_X64_MS_V1.md)
- [Matrice de conformité](Documentation/CONFORMITE_GS_PLUS_PLUS_1.0.md)
- [Frontend auto-hébergé 0.27](Documentation/FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md)
- [Validation de `0.27.0-alpha.10`](Documentation/Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.10.md)
- [Validation historique de la publication alpha.9](Documentation/Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.9.md)
- [Feuille de route](Documentation/FEUILLE_DE_ROUTE_GS_PLUS_PLUS.md)

Toute la documentation normative est maintenue en Markdown comme source
principale.

## Validation actuelle

- conformité portable : **20/20** sous MSVC et GNU ;
- CTest des sources alpha.10 : **5/5** sous Windows et **6/6** sous
  Linux (**4/4** et **5/5** pour la tranche alpha.9 publiée) ;
- quatre scénarios de benchmark smoke réussis sur chaque hôte ;
- CI GitHub Windows et Linux ;
- l’unique image auto-hébergée `Frontend.GsE`, qui réunit les quatre étapes du
  frontend, est comparée entre les deux chaînes validées ;
- typage récursif différentiel des indexations, de `&`, de `*` et des appels
  indirects, y compris les callbacks imbriqués ;
- sélection différentielle des opérateurs libres, y compris les surcharges,
  références qualifiées, ambiguïtés, arités et opérateurs unaires ;
- validation différentielle des vingt-quatre opérateurs intrinsèques, avec
  adaptation des littéraux ;
- liaisons de références qualifiées, conversions d’héritage, affectations et
  retours alignés sur le bootstrap ;
- contraintes structurelles et numériques des déclarations, énumérations et
  initialiseurs globaux et conversions explicites alignées sur le bootstrap,
  avec **2 719 corpus négatifs** dans les sources de développement (**619** dans
  l'alpha.10 publiée), dont le code, la ligne et la colonne sont contrôlés ;
- références de callbacks, signatures imbriquées et tableaux de pointeurs à
  indirections profondes couverts par les tests différentiels de développement ;
- résolution contextuelle des types nommés dans les signatures, distinction des
  homonymes, appels de callbacks stockés dans les champs et AST d'entrée préservé ;
- contraintes des signatures de fonctions et callbacks, limites de paramètres
  avec récepteur implicite et opérateurs libres sur structures/unions ;
- chaînes d'alias de champs vers leur stockage canonique, avec validation des
  alias inutilisés, des cycles et des cibles introuvables ;
- résolution canonique des alias de types, fonctions libres et globales,
  avec déclarations anticipées, chaînes, noms qualifiés et refus des cibles
  absentes, cycles et fonctions surchargées ambiguës ;
- appels via alias de méthodes non liées, avec récepteur `Classe&` explicite,
  contrôle des arguments et de la visibilité des appels directs, signatures de
  callbacks et relocalisations vers la méthode canonique ;
- déclarations d'héritage et bases canoniques, avec refus des bases absentes,
  non-classes ou non publiques, auto-héritage et cycles indirects, validés
  localement après alpha.10 ;
- remplacements de méthodes, destructeurs et opérateurs virtuels, avec
  signatures canoniques et ordre base/dérivée ; dispositions polymorphes
  des tableaux d'objets comparées au bootstrap dans le périmètre testé ;
- refus des surcharges déclarées plusieurs fois, avec paramètres canoniques,
  alias de types, méthodes, constructeurs, destructeurs et opérateurs libres
  ou membres ; le type de retour seul ne distingue pas une surcharge ;
- signatures non liées comparées avec le récepteur implicite en première
  position, y compris les collisions entre une méthode et une fonction libre
  de même nom complet dans un espace homonyme de sa classe ;
- refus des collisions entre symboles de liaison calculés pour des surcharges
  distinctes, avec types canoniques, alias, callbacks, récepteurs implicites et
  espaces qualifiés ou UTF-8 ; diagnostic bilingue 119 aligné sur le bootstrap ;
- sélection des appels qualifiés, par objet ou pointeur, dans les groupes
  mélangeant méthodes et fonctions libres de même nom complet ; comparaison de
  la déclaration retenue, du retour et des drapeaux, avec visibilité contrôlée
  après sélection et ambiguïtés conservées ;
- sélection des opérateurs unaires et binaires dans les groupes mixtes, avec
  récepteur, constance, références, héritage, masquage et visibilité ;
- priorité déterministe des groupes de surcharges invalides suivant leur
  première déclaration, puis collisions de liaison et corps de fonctions dans
  l'ordre source, vérifiée sur les cas indépendants couverts ;
- priorité des instructions successives, blocs imbriqués, expressions de
  conditions, branches et boucles dans un même corps ;
- priorité des opérandes, de l'objet avant l'indice, de la cible d'affectation
  avant sa valeur et du type cible de conversion avant sa source, dans le
  périmètre différentiel testé ;
- arguments d'appels dans l'ordre source, avec contrôle préalable de la cible
  indirecte et de l'arité, rejet des groupes sans signature d'arité et de
  récepteur recevables, sélection différée et appels imbriqués ; abandon des
  candidats après un préfixe incompatible, dans l'ordre de déclaration et le
  périmètre testé, avant l'analyse de l'argument suivant ; erreurs de types de
  conversions signalées seulement lorsque leur expression est visitée ;
- arguments agrégés analysés avec le type de la signature retenue, après
  sélection et visibilité pour les groupes directs, dans l'ordre des arguments
  pour les callbacks ; formes scalaires, structures, unions, tableaux de champs
  imbriqués et appels imbriqués couverts dans la matrice différentielle ;
- calcul et contrôle de plage des conversions constantes lors de leur visite,
  avant les erreurs suivantes, avec arité et abandon de candidats prioritaires,
  agrégats contextuels et courts-circuits comparés au bootstrap ;
- initialiseurs locaux avec contrôle de la forme et de la capacité avant
  leurs éléments, feuilles analysées dans l'ordre et refus avant l'instruction
  suivante ; références, callbacks et plages numériques couverts dans la matrice ;
- constructions locales sélectionnées et planifiées avant l'instruction suivante,
  avec arité, abandon des candidats, visibilité et agrégats contextuels ;
  contrôles des constructeurs et destructeurs des objets, tableaux, bases et
  sous-objets comparés au bootstrap dans le périmètre testé ;
- constructions récursives des bases et champs vérifiées avant l'initialiseur
  suivant, sans publier d'étapes supplémentaires ; plan final dans l'ordre
  canonique, choix des champs réutilisés et `parent()` sans constructeur propre
  pris en charge dans la matrice différentielle ;
- déclarations locales contrôlées lors de leur visite : types, noms répétés,
  variables `vide`, références et constantes sans initialiseur, construction
  explicite réservée aux classes ; priorité des diagnostics et portées des
  branches sans accolades comparées au bootstrap ;
- génération machine C++ des noms locaux réutilisés dans des portées distinctes,
  avec emplacements propres aux déclarations ; dix corpus bilingues exécutés,
  références, callbacks, tableaux, branches, boucles et destructions lors des
  retours anticipés, ainsi qu'un exemple d'intégration bilingue ;
- recherche lexicale des noms dans les espaces parents même sans import,
  notamment depuis les méthodes de classes ; types, alias, énumérations,
  globales, callbacks et opérateurs, avec masquage des homonymes externes ;
  douze corpus bilingues exécutés avec résultat 42 ;
- recherche démarrant à la classe dans les méthodes, constructeurs et
  destructeurs, y compris les champs par défaut ; masquage des fonctions
  parentes et importées, callbacks et récepteurs explicites ; quatorze corpus
  bilingues exécutés, cibles choisies comparées au bootstrap ;
- portées d'opérateurs dans les méthodes, constructeurs, destructeurs et champs
  par défaut : groupes mixtes, accès privés/protégés et choix de surcharge par
  constructeur ; dix-sept corpus bilingues exécutés avec cibles exactes,
  résultat 42 et images reproductibles ;
- conversions des champs par défaut résolues depuis la portée du constructeur,
  y compris les alias masquant les types parents/importés et les signatures
  de callbacks imbriquées ; quinze corpus bilingues exécutés couvrent aussi
  références, bases, champs objets et délégations, avec constructeurs choisis
  comparés au bootstrap ;
- callbacks des champs par défaut réévalués selon chaque constructeur : appels
  directs via callback, déréférencement, indexation, signatures imbriquées,
  références et agrégats ; dix corpus bilingues exécutés et un contrôle
  sémantique bilingue des références de callbacks constantes/volatiles ;
- arguments agrégés des callbacks évalués avec la signature de chaque constructeur,
  dans les champs par défaut et les initialisations de champs objets, bases et
  délégations ; neuf corpus bilingues exécutés supplémentaires ; les diagnostics
  internes des expressions restent distincts de l'incompatibilité avec le champ ;
- retours par référence des callbacks : lectures, liaisons, mutations, adresses,
  champs et appels imbriqués ; vingt-cinq corpus bilingues exécutés avec callbacks
  C++ fournis par l'hôte et nombre d'appels vérifié ; constance conservée par le
  bootstrap, qualifications `volatile` et `constante volatile` conservées pour
  les champs/éléments adressés par le frontend Gs++, y compris par flèche ; cela
  n'ajoute pas les retours par référence aux fonctions ordinaires Gs++ ;
- références de pointeurs retournées par les callbacks : copies, liaisons,
  changements de cible, référents constants et tableaux de pointeurs ; vingt
  corpus bilingues exécutés avec pointeurs et valeurs pointées contrôlés séparément ;
  le déréférencement d'un élément n'est plus confondu avec un tableau entier ;
- références vers des callbacks paramétrés : vingt-deux corpus bilingues exécutés
  avec copies indépendantes, remplacement, appels, arguments et stockage vérifiés ;
  les deux analyseurs protègent les callbacks constants sans interdire leur appel
  ni la réaffectation des pointeurs vers des données constantes ;
- paramètres référencés des callbacks imbriqués : vingt-quatre corpus bilingues
  exécutés avec adresses et traces de mutation exactes, constructions, références
  retournées et courts-circuits vérifiés ; un callback constant peut recevoir
  un argument mutable lorsque sa signature l'autorise ;
- structures et emplacements de pointeurs référencés dans les callbacks imbriqués :
  vingt-quatre corpus bilingues exécutés vérifient l'identité, les copies indépendantes,
  champs et éléments, constructions et redirections ; un pointeur vers une donnée
  constante peut changer de cible sans autoriser la mutation de cette donnée ;
- références dans les groupes mêlant méthodes et fonctions : vingt-deux corpus
  bilingues exécutés vérifient la cible sélectionnée, les mutations, qualifications,
  conversions vers une base et contextes de construction ; les égalités de score
  restent ambiguës, sans préférence implicite de style C++ pour une référence ;
- opérateurs mixtes recevant des références : vingt-quatre corpus bilingues
  exécutés vérifient mutations, redirections de pointeurs, conversions de classes,
  constructions et expressions imbriquées ; une trace exportée contrôle l'ordre
  des appels et l'absence d'exécution dans les courts-circuits logiques intégrés ;
- opérateurs dans les initialiseurs agrégés : vingt-sept corpus bilingues exécutés
  vérifient structures, unions, tableaux imbriqués, affectations, retours et
  constructions ; chaque valeur est capturée dans l'ordre des éléments ;
  la forme agrégée est contrôlée avant ses feuilles dans les affectations et retours ;
- analyse auto-hébergée d'interfaces préparées en mémoire : prototypes, globales
  externes implicites, visibilité des membres, types et signatures ; vingt-deux
  corpus bilingues syntaxiques/sémantiques et six interfaces de types/données ;
  cette API ne lit pas les fichiers et ne développe pas les inclusions ;
- assemblage auto-hébergé de sources et interfaces préparées : texte, AST et
  table d'origines, avec diagnostics locaux et imports d'espaces isolés par unité ;
  treize corpus bilingues valides, sorties protégées et échecs d'allocation testés ;
  la normalisation des membres/groupes mixtes et le raccordement des inclusions restent à faire ;
- normalisation auto-hébergée des fonctions/opérateurs libres, globales et alias
  préparés : définitions préférées aux prototypes, ordre des premières déclarations
  conservé, noms et types exacts comparés avant résolution des alias ; vingt-deux
  corpus bilingues valides et trente refus de normalisation différentiels ;
- initialiseurs globaux contrôlés entièrement dans l'ordre source, avec typage
  de toutes les feuilles avant leur passe constante ; priorité des contrôles
  structurels des champs par défaut, dans le périmètre différentiel testé ;
- émission auto-hébergée des données globales et relocalisations de fonctions,
  avec comparaison des octets, alignements, cibles et limites des tampons.

## Licence

Gs++ est distribué sous la [Mozilla Public License 2.0](LICENSE).
