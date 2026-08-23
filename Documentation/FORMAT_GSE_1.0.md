# Format exécutable GsE 1.0

GsE 1.0 est le format exécutable canonique de Gs++ 0.26.0 et de Sanctuaire SE.
Tous les entiers sont non signés et encodés en petit-boutiste, sauf indication
contraire. Sa disposition conserve toutes les fonctionnalités de l’ancienne
numérotation locale GsE 2.0 ; seule la base de version est remise à 1.0 avant
publication. Les artefacts locaux antérieurs doivent être reconstruits.

## En-tête de 112 octets

| Position | Taille | Champ |
|---:|---:|---|
| 0 | 5 | signature ASCII `GSE:0` |
| 5 | 3 | réservé, nul |
| 8 | 2 | version majeure, `1` |
| 10 | 2 | version mineure, `0` |
| 12 | 2 | taille de l’en-tête, `112` |
| 14 | 2 | architecture, `0x8664` pour AMD64 |
| 16 | 2 | type de fichier, `1` pour exécutable |
| 18 | 2 | drapeaux |
| 20 | 2 | ABI, `1` |
| 22 | 2 | réservé, nul |
| 24 | 4 | nombre de segments |
| 28 | 4 | nombre de sections |
| 32 | 8 | RVA du point d’entrée |
| 40 | 8 | base préférée, actuellement nulle |
| 48 | 8 | taille de l’image en mémoire |
| 56 | 8 | position de la table des segments |
| 64 | 8 | position de la table des sections |
| 72 | 8 | position des métadonnées |
| 80 | 8 | taille des métadonnées |
| 88 | 24 | réservé, nul |

Une entrée de segment et une entrée de section occupent chacune 64 octets. Les
segments chargés respectent W^X : aucun segment n’est simultanément modifiable
et exécutable.

## Types de sections

| Type | Section | Contenu |
|---:|---|---|
| 1 | `.texte` | code machine |
| 2 | `.donnees` | données initialisées |
| 3 | `.zero` | données initialisées à zéro |
| 4 | `.imports` | imports de symboles |
| 5 | `.exports` | symboles publics |
| 6 | `.relog` | relocalisations d’import et de base interne |
| 7 | `.meta` | métadonnées textuelles GsC |
| 8 | `.chaines` | noms de symboles UTF-8 |

## Table de chaînes

`.chaines` contient une suite de chaînes UTF-8 terminées par un octet nul. La
longueur enregistrée dans une référence exclut cette terminaison. Chaque nom :

- contient entre 1 et 1 024 octets UTF-8 ;
- ne contient aucun octet nul interne ;
- commence au début exact d’une chaîne de la table ;
- possède un encodage UTF-8 valide.

Les chaînes identiques sont dédupliquées par l’écrivain GsE.

## Entrée d’import de 32 octets

| Position | Taille | Champ |
|---:|---:|---|
| 0 | 4 | position relative du nom dans `.chaines` |
| 4 | 2 | longueur du nom en octets |
| 6 | 2 | type de symbole, `1` pour fonction |
| 8 | 2 | ABI, `1` pour l’ABI Gs++ x86-64 courante |
| 10 | 2 | réservé, nul |
| 12 | 4 | drapeaux, bit 0 pour import obligatoire |
| 16 | 8 | réservé, nul |
| 24 | 8 | réservé, nul |

## Entrée d’export de 32 octets

| Position | Taille | Champ |
|---:|---:|---|
| 0 | 4 | position relative du nom dans `.chaines` |
| 4 | 2 | longueur du nom en octets |
| 6 | 2 | réservé, nul |
| 8 | 8 | RVA du symbole |
| 16 | 4 | taille du symbole |
| 20 | 2 | section logique : `0` code, `1` données, `2` zéro |
| 22 | 2 | type : `1` fonction, `2` objet |
| 24 | 4 | drapeaux |
| 28 | 4 | réservé, nul |

## Entrée de relocalisation de 24 octets

| Position | Taille | Champ |
|---:|---:|---|
| 0 | 8 | RVA de l’emplacement à modifier |
| 8 | 4 | indice d’import, ou `0xFFFFFFFF` pour `BASE64` |
| 12 | 2 | type de relocalisation |
| 14 | 2 | section source logique : `0` code, `1` données |
| 16 | 8 | ajout signé pour un import, RVA cible pour `BASE64` |

Les types définis sont :

| Type | Nom | Calcul |
|---:|---|---|
| 1 | `REL32` | adresse de l’import + ajout − adresse de l’instruction suivante |
| 2 | `Adresse64` | adresse de l’import + ajout |
| 3 | `BASE64` | base réelle de l’image + RVA cible |

`BASE64` matérialise notamment les adresses de fonctions stockées dans les
données globales. Son indice d’import est obligatoirement `0xFFFFFFFF` et son
RVA cible doit appartenir à un segment chargé de la même image.

## Validation et chargement

`gseverifier` vérifie la version, toutes les bornes, les tailles d’entrées, les
références de chaînes, UTF-8, les doublons, les segments, W^X, les exports et
les relocalisations. `gsechargeur` applique les trois types. `BOOTX64.EFI`
applique les relocalisations internes `BASE64` mais refuse encore les imports
externes. Les deux chargeurs n’acceptent que la signature `GSE:0`, le format
GsE canonique 1.0 et l’ABI 1. Les fichiers portant l’ancienne signature ou les
anciennes versions locales 1.1 ou 2.0 doivent être recompilés.
