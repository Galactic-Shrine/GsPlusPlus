# Champs objets classes et durée de vie récursive — Gs++ 0.22

## État

**VALIDÉ — Gs++ 0.22.0 — 16 août 2026.**

Gs++ 0.22 complète les listes d’initialisation de Gs++ 0.21 avec les champs
dont le type est une classe possédée par valeur. La construction, les tables
virtuelles et la destruction de ces sous-objets sont planifiées à leur adresse
réelle, y compris lorsqu’une classe intermédiaire ne déclare pas de
constructeur.

Ce jalon ne modifie aucun conteneur binaire : la signature objet reste
`GSOBJ:0`, GsObj/GsA/GsE restent en version 1.0, tous les champs ABI restent à
1 et les signatures de liaison restent préfixées par `GsAbi:x64-ms-v1`.

## Syntaxe

Un champ classe peut recevoir des arguments dans la liste du constructeur :

```gspp
classe Moteur
{
    publique:
        constructeur(entier32 puissance) {}
};

classe Vehicule
{
    Moteur Principal;

    publique:
        constructeur(entier32 puissance)
            : Principal(puissance)
        {}
};
```

La forme anglaise est identique avec `class`, `public`, `constructor` et
`super`. Les règles de Gs++ 0.21 restent applicables :

1. `parent(...)` ou `super(...)`, s’il est présent, doit être le premier
   initialiseur ;
2. seuls les champs déclarés directement par la classe peuvent être listés ;
3. un champ ne peut apparaître qu’une fois ;
4. les champs explicitement listés suivent leur ordre de déclaration ;
5. un alias de champ est normalisé vers son champ de stockage canonique.

Contrairement aux champs non classes, qui exigent exactement une expression,
un champ classe accepte zéro à trois arguments utilisateur. Le receveur caché
occupe le premier des quatre registres d’arguments de l’ABI Microsoft x64.

## Construction explicite

Pour `Champ(arguments)`, l’analyseur :

- cherche les surcharges de `Champ::$constructeur` ;
- inclut le receveur caché `Champ&` dans la résolution ;
- vérifie les conversions valeur et les liaisons par référence ;
- refuse les appels ambigus ou sans surcharge compatible ;
- contrôle la visibilité publique, protégée ou privée depuis la classe
  propriétaire ;
- enregistre le symbole exact et les indicateurs ABI de passage par référence.

Si le type du champ ne déclare aucun constructeur, seule la forme sans
argument est autorisée. Fournir un argument dans ce cas produit un diagnostic,
car Gs++ n’invente pas un constructeur de conversion implicite.

## Construction implicite des champs omis

Tout champ classe absent de la liste est construit automatiquement à sa place
dans l’ordre de déclaration.

- Si sa classe déclare des constructeurs, une surcharge sans argument doit être
  accessible et non ambiguë.
- Si sa classe ne déclare aucun constructeur, Gs++ construit récursivement sa
  base, installe sa table virtuelle éventuelle, puis construit ses propres
  champs classes.
- Une classe intermédiaire sans constructeur n’interrompt donc plus la durée
  de vie de ses sous-objets.

Le même plan implicite s’applique à une variable locale dont la classe ne
déclare aucun constructeur. Ainsi, les champs classes ne dépendent pas de la
présence d’un constructeur utilisateur dans chaque niveau d’imbrication.

## Ordre de construction

Pour une classe possédant une base et trois champs classes, l’ordre observable
est :

1. construction de la base directe, récursivement depuis la racine ;
2. installation de la table virtuelle de la classe courante ;
3. premier champ classe déclaré ;
4. deuxième champ classe déclaré ;
5. troisième champ classe déclaré ;
6. corps du constructeur courant.

Chaque étape porte un décalage relatif à l’objet racine. Le backend calcule
l’adresse `objet + décalage`, puis appelle le constructeur ou écrit le pointeur
de table virtuelle au décalage prévu par la disposition de cette sous-classe.

Le scénario de validation construit successivement :

```text
1  base
2  champ explicite
3  champ omis avec constructeur sans argument
4  feuille d’un champ polymorphe sans constructeur
```

La trace après construction vaut donc `1234`.

## Destruction récursive

Une variable locale classe reçoit un plan de destruction aplati. Pour chaque
sous-objet, une action contient :

- le symbole du destructeur à appeler ;
- le décalage relatif du receveur ;
- la classe du receveur.

L’ordre est défini récursivement comme suit :

1. corps du destructeur de la classe la plus dérivée, s’il est déclaré ;
2. champs classes directs en ordre inverse de déclaration ;
3. pour chaque champ, ses propres champs puis sa base selon la même règle ;
4. destructeur de la base directe, puis remontée jusqu’à la racine.

Un champ ou une classe sans destructeur déclaré n’ajoute pas d’appel, mais ses
sous-objets et sa base continuent d’être parcourus. Il n’est donc pas nécessaire
de déclarer un destructeur vide pour obtenir la destruction d’un champ possédé.

Dans le scénario validé, la fin de vie ajoute :

```text
9  destructeur du conteneur
5  feuille du dernier champ imbriqué
6  deuxième champ
7  premier champ
8  base
```

La trace complète vaut `123495678`.

## RAII et sorties de contrôle

Le plan de destruction est enregistré dans le mécanisme RAII local existant.
Il est donc exécuté :

- à la fin normale d’un bloc ;
- à la sortie d’une branche ;
- à la fin d’une itération ;
- avant un `retourner/return` anticipé.

Le scénario monolithique retourne depuis une fonction alors que le conteneur
est encore actif. La fonction appelante observe ensuite la trace complète, ce
qui prouve que les sous-objets imbriqués sont détruits avant la transmission de
la valeur de retour.

Gs++ 0.22 ne fournit pas encore les exceptions ; aucun déroulement de pile sur
exception n’est revendiqué.

## Polymorphisme imbriqué

Une classe sans constructeur peut néanmoins déclarer des méthodes virtuelles.
Son étape implicite installe sa table à l’adresse de son sous-objet avant de
construire ses champs. Le scénario 0.22 appelle une méthode virtuelle sur un
champ imbriqué de ce type et obtient la valeur attendue `40`.

Cette prise en charge ne change ni la disposition documentée en 0.18/0.19, ni
la signature ABI inter-unités. Une définition de classe incompatible reste
refusée par l’éditeur de liens.

## Compilation séparée

Le scénario séparé compile :

- les définitions et corps des classes dans un premier `.GsObj` ;
- l’interface `.HGsPP` et le consommateur dans un second `.GsObj` ;
- les deux objets vers un GsE 1.0.

Le consommateur calcule localement le plan de destruction à partir de
l’interface, tandis que les symboles des destructeurs sont résolus à l’édition
de liens. L’exécutable retourne `91`, comme le scénario monolithique.

## Diagnostics validés

Les tests refusent notamment :

- un champ omis dont la classe ne possède aucun constructeur sans argument
  compatible ;
- un constructeur de champ privé ou autrement inaccessible ;
- un destructeur de champ inaccessible ;
- des arguments vers une classe qui ne déclare aucun constructeur ;
- un tableau dont le type élémentaire est une classe ;
- les doublons, l’ordre incorrect, les champs hérités ou inconnus déjà refusés
  en 0.21.

## Limites explicites

Gs++ 0.22.0 ne revendique pas encore :

- les tableaux de classes et leur construction/destruction élément par élément ;
- les constructeurs délégués ;
- les initialisateurs au point de déclaration du champ ;
- les constructeurs globaux ;
- l’héritage multiple ou virtuel ;
- la RTTI, les virtuels purs et les exceptions.

Ces limites n’imposent aucune renumérotation de format. Les projets étant
locaux, les artefacts existants peuvent être reconstruits directement.

## Preuve

La validation complète est consignée dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.22.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.22.0.md).
