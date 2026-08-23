# Tableaux d’objets classes en Gs++ 0.23

**VALIDÉ — Gs++ 0.23.0 — 16 août 2026.**

## Objet du jalon

Gs++ 0.23.0 étend le cycle de vie déterministe du modèle objet aux tableaux
fixes dont l’élément final est une classe possédée par valeur. Cette règle
s’applique aux champs de classes et aux variables locales, y compris lorsque
le tableau possède plusieurs dimensions.

Ce jalon ne modifie ni la représentation d’un tableau, ni la disposition des
classes, ni les formats natifs. Il complète uniquement les plans sémantiques de
construction et de destruction déjà introduits en 0.22.

## Syntaxe prise en charge

Un tableau de champ objet classe peut être omis de la liste d’initialisation :

```gspp
classe Element
{
    publique:
        constructeur() {}
        destructeur() {}
};

classe Conteneur
{
    Element Valeurs[2][3];
    publique: constructeur() {}
};
```

Il peut aussi être cité explicitement sans argument :

```gspp
classe Conteneur
{
    Element Valeurs[2][3];
    publique: constructeur() : Valeurs() {}
};
```

Les deux formes construisent par défaut les six éléments. La seconde rend le
choix visible dans la liste, sans changer l’ordre réel.

Une variable locale suit le même contrat :

```gspp
entier32 Utiliser()
{
    Element locaux[4];
    retourner 0;
}
```

## Construction normative

Les éléments sont parcourus dans l’ordre croissant des indices, selon la
disposition contiguë déjà utilisée par Gs++. Pour `Element valeurs[2][3]`,
l’ordre est :

1. `valeurs[0][0]` ;
2. `valeurs[0][1]` ;
3. `valeurs[0][2]` ;
4. `valeurs[1][0]` ;
5. `valeurs[1][1]` ;
6. `valeurs[1][2]`.

Pour chaque élément, le compilateur :

1. calcule son décalage à partir de la taille du sous-tableau ou de l’élément ;
2. résout le constructeur accessible sans argument lorsqu’il est déclaré ;
3. sinon, parcourt récursivement la base, la table virtuelle et les champs de
   la classe sans constructeur déclaré ;
4. transmet au générateur l’adresse exacte et le type réel du sous-objet.

Un constructeur déclaré mais non compatible avec zéro argument produit le
diagnostic habituel d’absence de surcharge compatible. Un constructeur sans
argument inaccessible est également refusé.

## Destruction normative

Les éléments sont détruits dans l’ordre strictement inverse de leur
construction. Pour l’exemple `[2][3]`, l’ordre commence à `[1][2]` et se
termine à `[0][0]`.

Chaque élément applique récursivement le contrat objet général :

1. corps de son destructeur ;
2. champs directs en ordre inverse ;
3. base directe, puis remontée vers la racine.

Pour un tableau de champ, ce plan s’insère à sa place dans la destruction du
conteneur. Les champs du conteneur restent eux-mêmes détruits en ordre inverse.
Pour un tableau local, le plan est enregistré dans le RAII existant et
s’exécute sur fin de bloc, branche, boucle ou retour anticipé.

## Initialisation explicitement différée

Gs++ 0.23.0 n’introduit pas de syntaxe d’arguments distincts pour chaque
élément. Les formes suivantes restent refusées :

```gspp
classe C
{
    Element Valeurs[2];
    publique: constructeur() : Valeurs(1) {}
};
```

```gspp
entier32 Exemple()
{
    Element valeurs[2] = {{}, {}};
    retourner 0;
}
```

Le premier cas exige des arguments par élément qui ne sont pas encore
spécifiés. Le second supposerait une sémantique de copie ou de construction
agrégée d’objets classes. Le compilateur émet dans les deux cas un diagnostic
bilingue dédié au lieu de générer un cycle de vie incomplet.

## Disposition mémoire et ABI

La taille reste le produit des dimensions par la taille de l’élément final.
L’alignement reste celui de cet élément. Aucun compteur, fanion de construction
ou pointeur caché n’est ajouté dans l’objet.

| Contrat | Valeur conservée |
| --- | --- |
| Signature GsObj | `GSOBJ:0` suivie d’un octet nul |
| Signature GsA | `GSA:0` suivie de trois octets nuls |
| Signature GsE | `GSE:0` suivie de trois octets nuls |
| Formats GsObj, GsA et GsE | 1.0 |
| Champs ABI | 1 |
| Signature de liaison | `GsAbi:x64-ms-v1` |

Les objets, archives et exécutables locaux peuvent être reconstruits, mais
aucune migration de format n’est provoquée par le jalon 0.23.

## Couverture validée

- tableau de champ multidimensionnel `[2][3]` ;
- six décalages de construction croissants ;
- six décalages de destruction décroissants ;
- tableau local d’objets classes ;
- construction explicite de tout le tableau par `Champ()` ;
- construction implicite lorsque le champ est omis ;
- diagnostics pour arguments de champ et agrégat local non pris en charge ;
- génération bilingue français/anglais identique ;
- exécution GsE monolithique et séparée d’un tableau `[2][2]` ;
- valeurs construites `1,2,3,4`, trace de destruction `4321` et retour `10`.

La preuve de validation complète se trouve dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.23.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.23.0.md).

## Limites restantes

Restent hors du périmètre : arguments ou initialiseurs distincts par élément,
copie implicite de tableaux d’objets classes, constructeurs globaux,
constructeurs délégués, valeurs par défaut au point de déclaration, héritage
multiple ou virtuel, RTTI, virtuels purs et exceptions.
