# Format objet GsObj 1.0

**NORMATIF — Gs++ 0.26.0, cible produit Gs++ 1.0.**

GsObj 1.0 est le format objet natif de Galactic-Shrine. Il transporte le code,
les données, la taille de la zone zéro, les symboles, les signatures ABI, les
positions de source et les relocalisations nécessaires à GsA et GsE.

Tous les entiers sont non signés et encodés en petit-boutiste. Les offsets sont
relatifs au début du fichier. Les fichiers locaux portant une ancienne
signature ou une ancienne ABI doivent être reconstruits depuis leurs sources.

## En-tête de 112 octets

| Position | Taille | Champ |
| ---: | ---: | --- |
| 0 | 7 | signature ASCII `GSOBJ:0` |
| 7 | 1 | réservé, nul |
| 8 | 2 | version majeure, `1` |
| 10 | 2 | version mineure, `0` |
| 12 | 2 | architecture, `0x8664` pour AMD64 |
| 14 | 2 | ABI, `1` |
| 16 | 4 | taille de l’en-tête, `112` |
| 20 | 4 | réservé, nul |
| 24 | 4 | nombre de symboles |
| 28 | 4 | nombre de relocalisations |
| 32 | 8 | taille de la section texte |
| 40 | 8 | taille de la section données |
| 48 | 8 | taille de la section zéro en mémoire |
| 56 | 8 | position de la section texte |
| 64 | 8 | position de la section données |
| 72 | 8 | position de la table des symboles |
| 80 | 8 | position de la table des relocalisations |
| 88 | 8 | position de la table de chaînes |
| 96 | 8 | taille de la table de chaînes |
| 104 | 8 | réservé, nul |

La section zéro ne possède aucun octet dans le fichier. Sa taille participe à
la disposition mémoire et à la validation des symboles.

## Ordre canonique des blocs

L’écrivain courant produit les blocs dans l’ordre suivant :

1. en-tête ;
2. table des symboles ;
3. table des relocalisations ;
4. table de chaînes ;
5. remplissage nul jusqu’à un alignement de 16 octets ;
6. section texte ;
7. remplissage nul jusqu’à un alignement de 16 octets ;
8. section données.

Le lecteur utilise les positions enregistrées et ne doit jamais déduire les
positions à partir du seul ordre physique. Chaque plage doit être entièrement
contenue dans le fichier.

## Sections logiques

| Valeur | Nom | Signification |
| ---: | --- | --- |
| 0 | texte | code machine |
| 1 | données | données initialisées |
| 2 | zéro | données initialisées à zéro au chargement |
| 3 | indéfinie | symbole externe sans stockage dans cet objet |

Un symbole défini ne peut pas utiliser la section indéfinie. Un symbole non
défini doit utiliser la section indéfinie.

## Entrée de symbole de 48 octets

| Position | Taille | Champ |
| ---: | ---: | --- |
| 0 | 4 | position relative du nom dans la table de chaînes |
| 4 | 2 | longueur du nom en octets, hors terminaison nulle |
| 6 | 1 | genre : `1` fonction, `2` objet |
| 7 | 1 | section logique |
| 8 | 4 | drapeaux : bit 0 public, bit 1 défini |
| 12 | 4 | décalage dans la section logique |
| 16 | 4 | taille du symbole |
| 20 | 4 | position relative de la signature ABI |
| 24 | 2 | longueur de la signature ABI |
| 26 | 2 | réservé, nul |
| 28 | 4 | position relative du chemin source |
| 32 | 2 | longueur du chemin source |
| 34 | 2 | réservé, nul |
| 36 | 4 | ligne source |
| 40 | 4 | colonne source |
| 44 | 4 | réservé, nul |

Le nom et la signature ABI ne peuvent pas être vides. Un chemin source vide
est autorisé. Les noms de symboles sont uniques dans un objet.

Les limites courantes sont :

- nom de symbole : 1 à 1 024 octets UTF-8 ;
- signature ABI : 1 à 65 535 octets UTF-8 ;
- chemin source : 0 à 65 535 octets UTF-8 ;
- au plus 1 000 000 de symboles acceptés par le lecteur.

Pour un symbole défini, `décalage + taille` doit rester dans la section
associée. Les additions sont vérifiées sans dépassement.

## Entrée de relocalisation de 24 octets

| Position | Taille | Champ |
| ---: | ---: | --- |
| 0 | 4 | décalage de l’emplacement à modifier |
| 4 | 4 | indice du symbole cible |
| 8 | 1 | section source : `0` texte ou `1` données |
| 9 | 1 | type de relocalisation |
| 10 | 2 | réservé, nul |
| 12 | 8 | ajout, nul dans GsObj 1.0 courant |
| 20 | 4 | réservé, nul |

Les types de relocalisation sont :

| Valeur | Nom | Largeur | Calcul lors de la liaison |
| ---: | --- | ---: | --- |
| 0 | `Relatif32` | 4 | cible − adresse de l’instruction suivante |
| 1 | `Adresse64` | 8 | adresse absolue ou relocalisable de la cible |

L’indice cible doit désigner une entrée existante. La plage modifiée doit être
entièrement contenue dans la section source. Le lecteur courant accepte au plus
4 000 000 de relocalisations.

## Table de chaînes

La table commence obligatoirement par un octet nul. Chaque référence contient
une position et une longueur. L’octet situé immédiatement après la longueur
doit être nul. Une chaîne :

- est encodée en UTF-8 valide ;
- ne contient aucun octet nul interne ;
- respecte la limite propre au champ ;
- reste entièrement dans la table.

Les chaînes identiques peuvent être dédupliquées. Le lecteur ne dépend pas de
leur ordre, mais l’écrivain canonique les ajoute lors du parcours déterministe
des symboles.

## Contrat ABI

Le champ ABI de l’en-tête vaut `1`. Chaque symbole porte en plus une signature
textuelle commençant par `GsAbi:x64-ms-v1`. Cette signature décrit
récursivement le retour, les paramètres, les références, les callbacks et les
types structurés nécessaires au contrôle inter-unités.

Deux définitions ou déclarations de même nom dont les signatures ABI diffèrent
sont incompatibles. L’éditeur de liens doit les refuser avant de produire GsA
ou GsE. Le contrat complet se trouve dans
[`ABI_GS_PLUS_PLUS_X64_MS_V1.md`](ABI_GS_PLUS_PLUS_X64_MS_V1.md).

## Règles de refus

Le lecteur GsObj refuse notamment :

- les fichiers plus petits que 112 octets ;
- l’ancienne signature `GSOBJ` suivie d’un zéro ;
- toute signature différente de `GSOBJ:0` + zéro ;
- une version différente de 1.0 ;
- une architecture différente d’AMD64 ;
- une ABI différente de 1 ;
- une taille d’en-tête différente de 112 ;
- un champ réservé non nul ;
- une table ou une section hors fichier ;
- une section texte, données ou zéro supérieure à 4 Gio ;
- une table de chaînes vide ou mal terminée ;
- une chaîne UTF-8 invalide ;
- un genre, une section ou des drapeaux inconnus ;
- un symbole dupliqué ou hors de sa section ;
- une relocalisation dont la cible, le type ou la plage est invalide.

## Reproductibilité

À entrées, options et version de compilateur identiques, l’écrivain GsObj doit
produire les mêmes octets. Aucun horodatage, identifiant aléatoire ou chemin de
construction implicite n’est ajouté au format. Les chemins de source explicites
font partie des données et doivent être normalisés par l’orchestrateur lorsque
la reproductibilité entre racines différentes est exigée.

## Compatibilité

Les formats GsObj, GsA et GsE restent tous en version 1.0 et utilisent l’ABI 1.
L’ajout de nouvelles capacités au compilateur ne modifie pas automatiquement
ces numéros. Toute évolution incompatible future exige une décision normative,
des tests de rejet et une stratégie explicite de reconstruction ou migration.
