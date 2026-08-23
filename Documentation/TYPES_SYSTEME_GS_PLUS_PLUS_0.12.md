# Types système de Gs++ 0.12

## Types scalaires

Le français reste la syntaxe canonique. Les alias anglais sont normalisés vers
les mêmes types internes et produisent le même code machine.

| Français | Anglais | Taille | Alignement | Interprétation |
|---|---|---:|---:|---|
| `entier8` | `int8` | 1 | 1 | signé, −128 à 127 |
| `entier16` | `int16` | 2 | 2 | signé, −32 768 à 32 767 |
| `entier32` | `int32` | 4 | 4 | signé, −2³¹ à 2³¹−1 |
| `entier64` | `int64` | 8 | 8 | signé, −2⁶³ à 2⁶³−1 |
| `naturel8` | `uint8` | 1 | 1 | non signé, 0 à 2⁸−1 |
| `naturel16` | `uint16` | 2 | 2 | non signé, 0 à 2¹⁶−1 |
| `naturel32` | `uint32` | 4 | 4 | non signé, 0 à 2³²−1 |
| `naturel64` | `uint64` | 8 | 8 | non signé, 0 à 2⁶⁴−1 |
| `booléen` / `booleen` | `bool` | 1 | 1 | type distinct, `faux` ou `vrai` |
| `octet` | `byte` | 1 | 1 | octet non signé distinct |
| `caractère` / `caractere` | `char` | 1 | 1 | caractère signé sur 8 bits |

Un littéral décimal peut couvrir toute la plage de `naturel64`. Les séparateurs
`_` sont permis. Le compilateur accepte donc directement
`18_446_744_073_709_551_615` et la borne basse
`-9_223_372_036_854_775_808` de `entier64`.

Les additions, soustractions et multiplications sont ramenées à la largeur du
type après chaque opération. Le comportement de débordement est ainsi défini
par complément à deux. La division, le reste et les comparaisons choisissent
explicitement leur forme signée ou non signée.

## Qualificateurs

`constante`/`const` et `volatile` précèdent le type :

```gspp
constante entier32 Limite = 42;
volatile naturel32 Registre = 0;
const entier32* lectureSeule;
```

Une valeur `constante` doit être initialisée et ne peut plus être affectée.
`volatile` impose à ce backend sans optimisation de conserver chaque lecture et
écriture exprimée par le programme. Les conversions implicites de pointeurs ne
retirent aucun qualificateur ; une conversion explicite est nécessaire.

## Tableaux fixes

Les dimensions suivent le nom du champ ou de la variable :

```gspp
entier16 valeurs[4];
naturel32 matrice[3][2];
```

Les éléments sont contigus, sans remplissage intermédiaire, et les dimensions
sont en ordre de lignes. L’indexation vérifie que l’indice est entier et met
l’adresse à l’échelle avec la taille réelle de l’élément. Les tableaux peuvent
être locaux, globaux ou membres d’une structure ou d’une union.

Les initialisations agrégées, la copie de tableaux, les paramètres tableaux par
valeur et les pointeurs vers un tableau complet ne font pas partie de 0.12.0.

## Énumérations et unions

Une énumération est typée et portée par son nom :

```gspp
énumération Etat
{
    Arrete = 3,
    Actif,
    Termine = 9,
};

Etat courant = Etat::Actif;
```

Son type sous-jacent est `entier32`. Deux énumérations différentes ne sont pas
compatibles et un entier n’est pas implicitement convertible en énumération.

Une `union` utilise la syntaxe des structures. Tous ses champs commencent au
décalage zéro ; sa taille est celle du plus grand champ, arrondie à
l’alignement maximal :

```gspp
union MotDouble
{
    naturel64 Complet;
    naturel32 Parties[2];
};
```

## Conversions explicites

Les deux formes suivantes sont équivalentes :

```gspp
entier32 code = convertir<entier32>(Etat::Actif);
int32 code = cast<int32>(State::Running);
```

Les conversions sont réservées aux scalaires. Une conversion entier–pointeur,
structure–scalaire ou tableau–scalaire est refusée. Pour une expression
constante, une conversion hors plage est diagnostiquée à la compilation. Pour
une valeur calculée à l’exécution, une conversion vers un entier plus étroit
conserve les bits de poids faible ; une conversion vers `booléen` produit
strictement zéro ou un. Une conversion explicite entre deux types pointeurs est
permise.

## ABI x86-64

Gs++ utilise l’ABI Microsoft x64 pour les appels générés, y compris dans le
noyau UEFI :

- les quatre premiers paramètres scalaires utilisent `RCX`, `RDX`, `R8` et
  `R9` ; seule la largeur déclarée est enregistrée dans la pile locale ;
- un entier signé de 8, 16 ou 32 bits est étendu avec son signe lors d’une
  lecture ; un entier non signé, un octet ou un booléen est étendu avec zéro ;
- les retours utilisent `RAX` et sont normalisés à la largeur et à la
  signedness déclarées avant `ret` ;
- les pointeurs et les entiers 64 bits occupent les 64 bits du registre ;
- une énumération suit les règles d’un `entier32` et un booléen est renvoyé sous
  la forme normalisée `0` ou `1` ;
- une structure ou union passée par valeur arrive sous forme d’adresse, puis
  est copiée dans le cadre local du destinataire ;
- une structure ou union retournée utilise une adresse de résultat cachée dans
  `RCX`, ce qui décale les paramètres explicites vers `RDX`, `R8` et `R9` ;
- les tableaux fixes ne sont pas encore transmis ni retournés par valeur.

Cette extension est identifiée par `GsAbi:x64-ms-v1` depuis la remise à zéro
canonique de Gs++ 0.17.1. Sa
description complète se trouve dans
[`VALEURS_STRUCTUREES_GS_PLUS_PLUS_0.15.md`](VALEURS_STRUCTUREES_GS_PLUS_PLUS_0.15.md).

Les tailles, alignements, extensions de registres, sérialisations globales et
cas signés/non signés sont couverts par les tests unitaires. Le programme
`GsPlusPlus/Tests/Integration/TypesSysteme.GsPP` exerce aussi réellement ces règles dans le chargeur
GsE et doit retourner le code `120`.
