# Modèle objet système de Gs++ 0.18

> Ce document reste la référence historique du socle objet 0.18. L’extension
> d’héritage est décrite dans
> [`HERITAGE_GS_PLUS_PLUS_0.19.md`](HERITAGE_GS_PLUS_PLUS_0.19.md), puis son
> extension courante dans
> [`REFERENCE-GS-PLUS-PLUS-0.22.md`](REFERENCE-GS-PLUS-PLUS-0.22.md). La
> durée de vie récursive des champs objets classes est détaillée dans
> [`CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md`](CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md).

## Statut

Ce document décrit le modèle objet effectivement implémenté par Gs++ 0.18.0.
Il complète la référence 0.17 sans modifier les formats natifs : `.GsObj`
conserve la signature `GSOBJ:0`, GsObj/GsA/GsE restent en version 1.0 et les
signatures de liaison restent préfixées par `GsAbi:x64-ms-v1`.

Le modèle est volontairement *freestanding*. Une classe locale, son
constructeur, son destructeur, ses méthodes et sa table virtuelle ne demandent
ni tas, ni exception, ni initialiseur global, ni bibliothèque d’exécution.

## Mots-clés bilingues

| Français canonique | Alias anglais | Rôle |
|---|---|---|
| `classe` | `class` | déclaration d’une classe |
| `publique` | `public` | section publique |
| `protégée` | `protected` | section protégée |
| `privée` | `private` | section privée |
| `constructeur` | `constructor` | constructeur d’instance |
| `destructeur` | `destructor` | destructeur d’instance |
| `virtuel` | `virtual` | méthode appelée par table virtuelle |
| `opérateur` | `operator` | surcharge d’un opérateur |
| `soi` | `this` | référence implicite vers l’instance |

Les accents restent facultatifs pour `protégée`/`protegee` et
`opérateur`/`operateur`. Le code source reste encodé en UTF-8.

## Déclaration d’une classe

```gspp
classe Ressource
{
    privée:
        entier32 Valeur;

    publique:
        constructeur(entier32 valeur)
        {
            soi.Valeur = valeur;
        }

        destructeur()
        {
            soi.Valeur = 0;
        }

        entier32 Lire()
        {
            retourner soi.Valeur;
        }
};
```

Une classe utilise la visibilité privée par défaut. Une `structure` et une
`union` conservent la visibilité publique par défaut et leur comportement
historique. Une section de visibilité se termine par `:`.

Les champs et méthodes privées ou protégées ne sont accessibles que depuis une
méthode de la classe propriétaire. En 0.18.0, l’héritage n’est pas encore
défini ; `protégée` est donc distinct dans l’AST et les diagnostics, mais donne
le même ensemble d’accès effectif que `privée`.

## Paramètre implicite `soi`

Chaque méthode est abaissée vers une fonction ordinaire dont le premier
paramètre est une référence cachée vers la classe :

```text
Ressource::Lire(Ressource& soi) -> entier32
```

Le corps emploie `soi.Champ`. L’appel `objet.Lire()` lie automatiquement
`objet` à ce premier paramètre. L’appel `pointeur->Lire()` lie la valeur gauche
`*pointeur`. Les appels restent soumis à la limite ABI Microsoft x64 de quatre
registres, paramètre `soi` compris ; un retour de structure réserve le premier
registre et limite donc l’ensemble à trois paramètres ABI.

## Constructeurs

Une variable locale de classe peut appeler un constructeur :

```gspp
Ressource ressource(42);
```

Avant l’appel, le stockage complet de l’objet est mis à zéro. Pour une classe
polymorphe, le compilateur installe ensuite le pointeur de table virtuelle. Le
constructeur reçoit l’adresse de l’objet par la référence cachée `soi`.

Plusieurs constructeurs peuvent être surchargés. Si aucun constructeur n’est
déclaré, `Classe objet;` produit un objet mis à zéro. Si au moins un
constructeur existe, une déclaration sans arguments sélectionne le
constructeur compatible sans argument explicite. L’initialisation agrégée avec
`= { ... }` reste réservée aux structures et unions ; elle ne contourne pas la
visibilité d’une classe.

Les constructeurs globaux ne sont pas introduits en 0.18.0 : cette décision
évite toute séquence de démarrage ou tout runtime implicite.

## Destructeurs et RAII

Un destructeur ne reçoit aucun paramètre explicite. Pour chaque variable locale
dont la classe déclare un destructeur, le compilateur émet automatiquement un
appel :

- à la sortie normale du bloc ;
- avant chaque `retourner` qui quitte la portée ;
- à la fin d’un corps conditionnel ou d’une itération ;
- en ordre inverse des déclarations.

La valeur de retour est évaluée avant les destructions, puis préservée pendant
leurs appels. Il n’existe pas d’exception dans le sous-ensemble système actuel ;
aucune table de déroulement ni personnalité d’exception n’est donc requise.

Les destructeurs de champs imbriqués ne sont pas synthétisés automatiquement
en 0.18.0. Une classe propriétaire qui contient un autre objet à durée de vie
non triviale doit appeler explicitement la politique de libération appropriée
dans son propre destructeur.

## Références

La syntaxe est `T&` :

```gspp
entier32 Incrémenter(entier32& valeur)
{
    valeur = valeur + 1;
    retourner valeur;
}

entier32 valeur = 41;
entier32& alias = valeur;
```

Une référence :

- doit être initialisée ;
- doit se lier à une valeur gauche de type compatible ;
- occupe huit octets sur x86-64 ;
- est passée comme une adresse dans l’ABI ;
- se comporte comme le type référencé dans les expressions ;
- ne possède pas de valeur nulle définie par le langage.

Les références sont prises en charge pour les paramètres et variables locales.
Les champs références, références globales, références de tableaux et retours
par référence sont refusés avec un diagnostic explicite en 0.18.0. Un pointeur
reste la solution pour ces contrats.

Dans une signature ABI textuelle, le marqueur `R` distingue une référence d’un
pointeur et d’une valeur. Ce marqueur fait partie du contrôle de compatibilité
entre objets GsObj sous l’ABI canonique 1.

## Surcharge de fonctions

Plusieurs fonctions peuvent partager le même nom si leurs listes de paramètres
diffèrent. La résolution applique :

1. le nombre exact d’arguments ;
2. les correspondances de types exactes ;
3. à défaut, une adaptation de constante entière vérifiée ;
4. le rejet si aucune meilleure candidate unique n’existe.

Une surcharge reçoit un symbole de liaison déterministe dérivé de sa signature.
Une fonction non surchargée conserve exactement son ancien nom, afin de ne pas
modifier les symboles historiques. Prendre l’adresse d’un groupe surchargé sans
contexte de type ou créer un alias vers un groupe ambigu est refusé.

Le même nom manglé est calculé pour une définition et une déclaration externe,
ce qui permet la compilation séparée GsObj/GsA.

## Surcharge d’opérateurs

Une méthode peut surcharger les opérateurs unaires `!` et `~`, ainsi que les
opérateurs binaires suivants :

```text
+  -  *  /  %  ==  !=  <  <=  >  >=  &  |  ^  <<  >>
```

Exemple :

```gspp
entier32 opérateur +(entier32 delta)
{
    retourner soi.Valeur + delta;
}
```

L’opérande gauche d’un opérateur membre est la référence cachée `soi`. Un
opérateur binaire libre peut également être déclaré avec deux paramètres. Les
opérateurs d’affectation, d’appel, d’indexation, `&&` et `||` ne sont pas
surchargeables en 0.18.0.

## Fonctions virtuelles optionnelles

Une classe sans méthode `virtuel` conserve la même disposition de champs qu’une
structure non polymorphe. Dès qu’au moins une méthode est virtuelle :

- un pointeur de table virtuelle de huit octets occupe le décalage 0 ;
- l’alignement de la classe est au moins 8 ;
- les champs commencent après ce pointeur et gardent leur alignement naturel ;
- la table `@GsVTable::<nom-complet>` contient une adresse 64 bits par méthode ;
- les entrées suivent l’ordre de déclaration des méthodes virtuelles ;
- l’appel charge l’entrée depuis l’objet, puis effectue un appel indirect.

Les tables utilisent des relocalisations `Adresse64` dans `.data`. Leur symbole
est local et leur contenu participe aux contrôles de liaison. La signature ABI
de la classe encode explicitement classe/structure/union, le caractère
polymorphe, la taille, l’alignement, les champs et leurs décalages.

Gs++ 0.18.0 ne définit pas encore l’héritage ni le remplacement d’une méthode
de base. Le mécanisme virtuel valide donc la disposition et l’appel indirect
optionnels, et prépare un futur contrat d’héritage sans en figer prématurément
les règles.

## Validation de référence

Le scénario `Tests/Integration/ModeleObjet.GsPP` compile une classe avec champ
privé, constructeur, destructeur RAII, méthode ordinaire, méthode virtuelle,
opérateur, référence et fonction surchargée. L’image GsE 1.0 est vérifiée puis
exécutée par `gsechargeur`; le résultat attendu est `25` et un compteur confirme
que le destructeur s’est exécuté exactement une fois à la sortie du bloc.

Le scénario de compilation séparée produit deux GsObj à partir de l’interface,
de l’implémentation et du consommateur, puis lie et exécute le GsE final avec le
retour `42`. Les tests unitaires couvrent aussi la syntaxe
française/anglaise, la disposition de classe, le mangling, la table virtuelle,
les relocalisations, l’ABI 1, le refus d’un accès privé externe et le rejet de
deux ordres de tables virtuelles incompatibles entre unités.

La validation finale du 16 août 2026 a réussi 4/4 tests sous Windows/MSVC,
5/5 sous GNU/WSL et le démarrage réel QEMU/OVMF de Sanctuaire SE. Le rapport
canonique est
[`Validations/VALIDATION-GS-PLUS-PLUS-0.18.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.18.0.md).
