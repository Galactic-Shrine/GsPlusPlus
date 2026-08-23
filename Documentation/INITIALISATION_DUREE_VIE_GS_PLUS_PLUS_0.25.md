# Initialisation et durée de vie en Gs++ 0.25

**VALIDÉ — Gs++ 0.25.0 — 16 août 2026.**

## Objet du jalon

Gs++ 0.25.0 finalise le contrat d’initialisation déterministe nécessaire à la
future version 1.0 sans introduire de runtime caché. Le jalon ajoute les valeurs
par défaut des champs de classes, la délégation entre constructeurs et un
argument uniforme pour les tableaux d’objets classes. Il précise aussi que les
objets de classe globaux restent exclus des profils courants.

Ces règles complètent les constructions de bases, de champs et de tableaux
déjà définies entre les versions 0.20 et 0.23. Elles ne changent ni la
disposition mémoire des objets, ni les formats natifs, ni l’ABI.

## Valeurs par défaut des champs

Un champ direct de classe non objet peut recevoir une expression par défaut au
point de déclaration :

```gspp
classe Configuration
{
    publique:
        entier32 Taille = 7;
        entier32 Codes[3] = {1, 2};

        constructeur() {}
};
```

L’initialisation d’un objet applique les règles suivantes :

1. la base directe est construite ;
2. les champs sont initialisés dans leur ordre de déclaration ;
3. un champ cité dans la liste du constructeur utilise l’expression explicite ;
4. sinon, un champ possédant une valeur par défaut utilise cette valeur ;
5. sinon, le contrat historique de mise à zéro ou de construction par défaut
   s’applique ;
6. le corps du constructeur est exécuté après toutes ces étapes.

Une entrée explicite de liste remplace donc la valeur déclarée :

```gspp
classe Choix
{
    publique:
        entier32 Valeur = 5;
        constructeur() {}
        constructeur(entier32 valeur) : Valeur(valeur) {}
};
```

Les éléments absents d’un agrégat de tableau sont mis à zéro. L’expression par
défaut est évaluée à chaque construction de l’objet ; elle n’est pas partagée
entre les instances.

Ce contrat est réservé aux champs de classes. Les champs de structures et
d’unions restent des agrégats et refusent cette syntaxe. Un champ objet de type
classe ne peut pas utiliser `= expression` : son constructeur doit être choisi
implicitement ou dans la liste d’initialisation. Une classe qui déclare une
valeur par défaut doit aussi déclarer au moins un constructeur afin que le
point d’exécution soit explicite.

## Constructeurs délégués

Un constructeur peut déléguer à une autre surcharge de la même classe avec
`soi(arguments)` en français ou `this(arguments)` en anglais :

```gspp
classe Configuration
{
    publique:
        entier32 Valeur = 7;

        constructeur() {}
        constructeur(entier32 valeur) : soi()
        {
            soi.Valeur = valeur;
        }
};
```

La délégation est l’unique entrée autorisée dans la liste d’initialisation du
constructeur. Elle ne peut pas être combinée avec `parent`/`super` ni avec un
champ. Le constructeur cible réalise toute l’initialisation de la base, de la
table virtuelle et des champs. Le corps du constructeur délégant s’exécute
ensuite sur l’objet entièrement construit.

La résolution de surcharge et le contrôle d’accès sont identiques à ceux d’un
constructeur ordinaire. La délégation directe vers soi-même et tout cycle
indirect entre plusieurs constructeurs sont des erreurs statiques. Une
déclaration `externe` de constructeur ne peut pas porter de délégation.

## Tableaux d’objets avec argument uniforme

Les tableaux locaux et les tableaux de champs objets classes peuvent fournir
une même liste d’arguments à chacun de leurs éléments :

```gspp
Element locaux[3](4);
```

```gspp
classe Conteneur
{
    Element Elements[3];
    publique: constructeur() : Elements(2) {}
};
```

Le compilateur résout une seule surcharge compatible pour le type d’élément.
Pour chaque élément, dans l’ordre croissant des indices, il réévalue les
expressions d’arguments puis appelle cette surcharge à l’adresse exacte de
l’élément. La destruction conserve l’ordre strictement inverse. Les références
restent des références lors de chaque appel et ne sont pas converties en copies
cachées.

Cette forme fournit des arguments uniformes. Une liste différente par élément,
une copie implicite de tableau ou un agrégat de classes tel que
`{{...}, {...}}` restent exclus du contrat 0.25.

## Objets globaux et profil freestanding

Une globale scalaire, un pointeur, une structure, une union ou un tableau non
classe peut conserver une initialisation sérialisable dans les données GsObj.
En revanche, une globale dont le type final est une classe, y compris à travers
un tableau, est refusée.

Gs++ 0.25 n’émet donc aucune table cachée de constructeurs, aucun point d’entrée
avant `Principal` et aucun registre de destructeurs de fin de processus. Cette
exclusion est normative pour le contrat 1.0 courant et préserve l’identité du
profil freestanding. Un futur profil hébergé pourrait définir un protocole
explicite distinct sans modifier silencieusement ce contrat.

## Erreurs et déroulement

Le langage ne fournit pas d’exceptions dans ce jalon. Une construction locale
qui commence s’achève normalement ou le programme suit son mécanisme d’erreur
explicite. Aucun déroulement de pile partiel après une exception n’est promis.
Le RAII existant continue de couvrir les sorties normales de blocs, branches,
boucles et retours anticipés.

## Disposition mémoire, formats et ABI

Aucun champ caché n’est ajouté. Les valeurs par défaut appartiennent au code du
constructeur et les arguments uniformes ne changent pas la représentation du
tableau.

| Contrat | Valeur conservée |
| --- | --- |
| Signature GsObj | `GSOBJ:0` suivie d’un octet nul |
| Signature GsA | `GSA:0` suivie de trois octets nuls |
| Signature GsE | `GSE:0` suivie de trois octets nuls |
| Formats GsObj, GsA et GsE | 1.0 |
| Champs ABI | 1 |
| Signature de liaison | `GsAbi:x64-ms-v1` |

Les productions locales antérieures doivent être reconstruites avec le
compilateur 0.25, mais aucune migration de format n’est nécessaire.

## Couverture validée

- valeur scalaire par défaut et agrégat de tableau partiellement initialisé ;
- remplacement d’une valeur par défaut dans la liste du constructeur ;
- délégation française `soi` et anglaise `this` ;
- refus des délégations directes, cycliques ou mélangées ;
- argument uniforme sur tableau local et tableau de champ ;
- construction croissante et destruction inverse des éléments ;
- refus des objets de classe globaux ;
- refus des valeurs par défaut dans les agrégats et sur un champ classe ;
- génération française et anglaise identique ;
- exécution GsE monolithique et séparée avec retour `25` ;
- absence de régression MSVC, GNU, benchmark et QEMU/OVMF.

La preuve complète est publiée dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.25.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.25.0.md).
