# Initialisation et appels parent Gs++ 0.20

> Ce document reste la référence historique de `parent/super` en 0.20. La liste
> étendue aux champs directs est décrite dans
> [`REFERENCE-GS-PLUS-PLUS-0.22.md`](REFERENCE-GS-PLUS-PLUS-0.22.md). Les
> champs non classes sont détaillés en 0.21 et les champs objets classes en
> [`CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md`](CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md).

## Statut

Ce document décrit les fonctions effectivement implémentées et validées dans
Gs++ 0.20.0. Il complète le contrat d’héritage simple public de Gs++ 0.19 sans
modifier la signature `GSOBJ:0`, les formats GsObj/GsA/GsE 1.0, l’ABI native
numéro 1 ni le préfixe `GsAbi:x64-ms-v1:`.

Le Markdown est la source principale. Toute copie `.docx` éventuelle reste une
version secondaire de consultation.

## Syntaxe canonique

Un constructeur dérivé peut initialiser sa base directe :

```gspp
classe Base
{
    protégée: entier32 Valeur;

    publique:
        constructeur(entier32 valeur)
        {
            soi.Valeur = valeur;
        }

        virtuel entier32 Lire()
        {
            retourner soi.Valeur;
        }
};

classe Derivee : publique Base
{
    privée: entier32 Bonus;

    publique:
        constructeur(entier32 valeur, entier32 bonus)
            : parent(valeur)
        {
            soi.Bonus = bonus;
        }

        remplacer entier32 Lire()
        {
            retourner parent.Lire() + soi.Bonus;
        }
};
```

La forme anglaise officielle est :

```gspp
constructor(int32 value, int32 bonus) : super(value) { ... }
return super.Read();
```

`parent` est le mot-clé français canonique et `super` son alias anglais. Le mot
`base` n’est volontairement pas réservé : les programmes existants peuvent
continuer à l’utiliser comme nom de variable.

## Initialiseur du constructeur de base

La grammaire prise en charge est limitée à un seul initialiseur, après la liste
des paramètres du constructeur dérivé et avant son corps :

```text
constructeur(paramètres) : parent(arguments) bloc
constructor(parameters) : super(arguments) block
```

Les règles sont les suivantes :

- l’initialiseur est réservé à un constructeur membre d’une classe dérivée ;
- il désigne toujours la base directe déclarée par la classe ;
- la résolution de surcharge emploie les types des arguments et le récepteur
  caché de la base ;
- un constructeur de base public ou protégé est accessible depuis la dérivée ;
- un constructeur privé est refusé ;
- une base directe sans constructeur déclaré accepte `parent()` mais pas
  `parent(arguments)` ;
- sans initialiseur explicite, le constructeur accessible invocable sans
  argument reste sélectionné automatiquement ;
- une déclaration externe de constructeur dans une interface `.HGsPP` ne porte
  pas l’initialiseur : celui-ci appartient à la définition `.GsPP`.

Une chaîne peut contenir des classes intermédiaires sans constructeur déclaré.
Le compilateur recherche alors le premier constructeur déclaré vers la racine,
l’appelle une seule fois et installe les tables virtuelles des étapes
intermédiaires dans l’ordre de construction.

## Prologue de construction

Pour chaque constructeur déclaré, le générateur x86-64 émet un prologue qui :

1. rend disponibles le récepteur caché et les paramètres explicites ;
2. évalue les arguments de `parent(...)` dans le contexte du constructeur ;
3. appelle le constructeur de base sélectionné ;
4. installe les tables virtuelles des bases intermédiaires sans constructeur ;
5. installe la table virtuelle de la classe courante ;
6. exécute le corps du constructeur dérivé.

L’appel d’un constructeur dérivé depuis une variable locale ne répète pas ce
travail. Chaque constructeur déclaré et chaque étape sans constructeur sont
donc traités exactement une fois.

La destruction conserve le contrat 0.19 : corps du destructeur dérivé, base
directe, puis racine.

## Appel direct de l’implémentation héritée

Dans une méthode de classe dérivée, `parent`/`super` est une pseudo-expression
dont le type statique est celui de la base directe et dont l’adresse est le
récepteur `soi/this`. Par exemple :

```gspp
remplacer entier32 Lire()
{
    retourner parent.Lire() + soi.Bonus;
}
```

L’appel est lié directement à l’implémentation héritée résolue et produit un
appel `REL32`. Il ne consulte pas la table virtuelle. Cette règle permet à un
remplacement d’enrichir le comportement de sa base sans provoquer de récursion
virtuelle vers lui-même.

À l’inverse, les formes ordinaires restent dynamiques :

```gspp
Base& vue = derivee;
entier32 resultat = vue.Lire();
```

Ici, `vue.Lire()` emploie toujours l’emplacement virtuel hérité et atteint le
remplacement de la classe dynamique.

`parent`/`super` est refusé hors d’une méthode de classe et dans une classe sans
base. Ce jalon ne permet pas de choisir un ancêtre plus éloigné par son nom : la
recherche part de la base directe puis suit les règles de membres hérités.

## Compilation séparée et ABI

L’initialiseur de base est une propriété de la définition du constructeur et
n’ajoute aucun nouveau conteneur binaire. Les types, hiérarchies, dispositions,
tables virtuelles et signatures de fonctions continuent d’être contrôlés par
les signatures récursives `GsAbi:x64-ms-v1:`.

Un appel direct `parent.Methode()` produit une relocalisation normale vers le
symbole de la méthode de base. Il fonctionne dans un GsE monolithique comme
entre objets GsObj liés séparément.

Tous les objets doivent être reconstruits avec le compilateur courant. Aucune
couche de compatibilité avec les anciennes bases locales n’est introduite.

## Périmètre validé

Les tests couvrent :

- `parent` français et `super` anglais avec identité du code et des données ;
- arguments valeur et indicateurs ABI de référence du constructeur de base ;
- résolution et appel `REL32` de l’implémentation parent ;
- chaîne avec classe intermédiaire sans constructeur, sans double appel ;
- rejet d’un initialiseur dans une racine ou une déclaration externe ;
- rejet des arguments vers une base sans constructeur déclaré ;
- rejet d’un constructeur de base inaccessible ou non invocable ;
- rejet de `parent` hors d’une méthode ;
- exécution réelle des scénarios monolithique et séparé avec retour `82` et
  trace de durée de vie `1,2,3,4`.

La preuve complète est consignée dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.20.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.20.0.md).

## Limites explicites

Gs++ 0.20.0 ne revendique pas encore :

- plusieurs bases, l’héritage virtuel, protégé ou privé ;
- plusieurs entrées dans une liste d’initialisation ;
- l’initialisation explicite des champs avant le corps du constructeur ;
- la désignation explicite d’un ancêtre au-delà de la base directe ;
- les conversions descendantes, la RTTI ou les virtuels purs ;
- la covariance des retours ;
- les références comme champs, globales, éléments de tableau ou retours ;
- la synthèse récursive des destructeurs de champs objets ;
- les constructeurs globaux et les exceptions.

Ces limites produisent un diagnostic lorsqu’une syntaxe reconnue les rencontre ;
elles ne modifient pas silencieusement le contrat binaire.
