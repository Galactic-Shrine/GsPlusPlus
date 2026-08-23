# ABI native Gs++ x64-ms-v1

**NORMATIF — ABI 1, cible produit Gs++ 1.0.**

Ce document consolide le contrat d’appel, de disposition et de liaison utilisé
par GsObj, GsA, GsE, le chargeur hébergé et Sanctuaire SE. Le préfixe textuel
canonique est `GsAbi:x64-ms-v1` et les champs ABI binaires valent `1`.

## Domaine

- architecture AMD64/x86-64 ;
- ordre des octets petit-boutiste ;
- convention d’appel Microsoft x64 pour le code Gs++ actuel ;
- profils freestanding et hébergé ;
- compilation monolithique ou séparée ;
- objets, bibliothèques et exécutables natifs Galactic-Shrine.

Cette ABI n’implique pas la compatibilité avec Win32, COM ou les bibliothèques
système Windows. Elle décrit uniquement le contrat machine des appels générés.

## Registres et pile

- les quatre premiers paramètres scalaires utilisent `RCX`, `RDX`, `R8` et
  `R9` ;
- les paramètres supplémentaires sont placés sur la pile ;
- l’appelant réserve 32 octets d’espace d’accueil ;
- la pile est alignée sur 16 octets avant `call` ;
- un résultat scalaire, pointeur, référence ou callback est retourné dans
  `RAX` ;
- les appels indirects suivent les mêmes règles que les appels directs.

Le backend doit conserver les registres non volatils exigés par la convention
Microsoft x64. Le code freestanding n’exige aucun runtime implicite.

## Types scalaires

| Type | Taille | Alignement | Règle d’extension |
| --- | ---: | ---: | --- |
| `booléen`/`bool` | 1 | 1 | zéro, valeur normalisée 0 ou 1 |
| `octet`/`byte` | 1 | 1 | zéro |
| `caractère`/`char` | 1 | 1 | selon le type déclaré |
| `entier8`/`int8` | 1 | 1 | signe |
| `naturel8`/`uint8` | 1 | 1 | zéro |
| `entier16`/`int16` | 2 | 2 | signe |
| `naturel16`/`uint16` | 2 | 2 | zéro |
| `entier32`/`int32` | 4 | 4 | signe |
| `naturel32`/`uint32` | 4 | 4 | zéro |
| `entier64`/`int64` | 8 | 8 | largeur complète |
| `naturel64`/`uint64` | 8 | 8 | largeur complète |
| pointeur | 8 | 8 | largeur complète |
| référence | 8 | 8 | adresse non nulle selon le contrat du langage |
| pointeur de fonction | 8 | 8 | adresse de code |

Une énumération suit `entier32`. `const` et `volatile` ne modifient pas la
taille, mais participent aux contrôles de type et de conversion.

## Structures, unions et classes

- chaque champ est placé au prochain décalage compatible avec son alignement ;
- la taille finale est arrondie à l’alignement maximal ;
- les champs d’une union commencent au décalage zéro ;
- les tableaux sont contigus, en ordre de lignes, sans remplissage entre
  éléments ;
- une base unique commence au décalage zéro ;
- la disposition des classes inclut, lorsque nécessaire, le pointeur de table
  virtuelle et les champs hérités ;
- les signatures ABI de classe décrivent la hiérarchie et les emplacements
  virtuels afin de détecter une divergence inter-unité.

## Passage et retour structurés

- une structure, union ou classe passée par valeur arrive sous forme d’adresse
  vers une copie, puis est copiée dans le cadre local du destinataire ;
- un résultat structuré utilise une adresse de résultat cachée dans `RCX` ;
- les paramètres explicites sont alors décalés vers `RDX`, `R8`, `R9`, puis la
  pile ;
- un appel retournant une structure accepte actuellement au plus trois
  paramètres explicites ;
- les tableaux fixes ne sont ni transmis ni retournés par valeur dans le
  contrat courant.

La copie structurée respecte la taille exacte de la disposition. Les
constructeurs et destructeurs relèvent du contrat du langage, pas d’une
allocation ou d’un runtime caché de l’ABI.

## Méthodes et virtuel

Le receveur caché `soi`/`this` est une adresse et consomme le premier
emplacement de paramètre. Un appel virtuel lit la cible depuis l’emplacement
déterministe de la table virtuelle. Un remplacement conserve l’emplacement de
la méthode héritée ; une nouvelle méthode virtuelle étend la table.

L’appel `parent.Methode()`/`super.Method()` cible directement l’implémentation
de base et ne réalise aucun dispatch virtuel.

## Pointeurs de fonction

Une signature de callback encode récursivement son retour et ses paramètres.
L’appel indirect réserve le même espace d’accueil et utilise les mêmes
registres qu’un appel direct. Une signature différente n’est pas compatible,
même si l’adresse machine occupe toujours huit octets.

## Signatures de symboles

Chaque symbole GsObj possède une signature UTF-8 non vide commençant par :

```text
GsAbi:x64-ms-v1
```

La signature distingue au minimum :

- nature de fonction ou d’objet ;
- type de retour ;
- paramètres et leur ordre ;
- valeur, pointeur ou référence ;
- qualificateurs pertinents ;
- callbacks récursifs ;
- dispositions structurées ;
- hiérarchie de classe et table virtuelle.

L’encodage textuel exact produit par le compilateur reste couvert par la suite
de conformité. Une unité consommatrice et une unité productrice doivent porter
la même signature pour un symbole lié.

## Formats binaires associés

| Conteneur | Version | Champ ABI | Signature |
| --- | ---: | ---: | --- |
| GsObj | 1.0 | 1 | `GSOBJ:0` + un zéro |
| GsA | 1.0 | 1 | `GSA:0` + trois zéros |
| GsE | 1.0 | 1 | `GSE:0` + trois zéros |

GsA valide chaque GsObj membre. GsE porte l’ABI dans son en-tête et dans chaque
entrée d’import. Les chargeurs refusent une ABI différente de 1.

## Profils

Le profil freestanding utilise cette ABI sans runtime, exception, allocation
ou initialisation cachée. Le profil hébergé utilise la même ABI native et peut
ajouter des imports explicitement déclarés. Les règles de profil sont définies
dans
[`PROFILS_GS_PLUS_PLUS_1.0.md`](PROFILS_GS_PLUS_PLUS_1.0.md).

## Compatibilité et évolution

Les numéros 1.0 et ABI 1 sont gelés pour la convergence vers Gs++ 1.0. Une
extension compatible doit préserver l’interprétation de toutes les données
existantes. Une évolution incompatible exige un nouveau contrat explicite ;
elle ne peut pas être introduite silencieusement sous le même champ ABI.
