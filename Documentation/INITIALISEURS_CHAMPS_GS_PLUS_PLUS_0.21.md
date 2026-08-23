# Initialisateurs de champs Gs++ 0.21

> Le contrat complémentaire des champs objets classes est validé dans
> [`CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md`](CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md).
> Le présent document reste normatif pour le périmètre 0.21.

## Statut

Ce document décrit les listes d’initialisation effectivement implémentées et
validées dans Gs++ 0.21.0. Il complète `parent/super` de Gs++ 0.20 sans
modifier la signature `GSOBJ:0`, les formats GsObj/GsA/GsE 1.0, l’ABI native
numéro 1 ni le préfixe `GsAbi:x64-ms-v1:`.

Le Markdown est la source principale. Toute copie `.docx` éventuelle reste une
version secondaire de consultation.

## Syntaxe

Un constructeur peut initialiser sa base directe puis ses champs :

```gspp
classe Derivee : publique Base
{
    privée:
        constante entier32 Bonus;
        Point Position;

    publique:
        constructeur(entier32 valeur, entier32 bonus)
            : parent(valeur),
              Bonus(bonus),
              Position({2, 3})
        {
        }
};
```

La forme anglaise emploie le même nom de champ et l’alias `super` :

```gspp
constructor(int32 value, int32 bonus)
    : super(value), Bonus(bonus), Position({2, 3})
{
}
```

Une classe racine peut utiliser une liste composée uniquement de champs :

```gspp
constructeur(entier32 valeur) : Valeur(valeur) {}
```

## Grammaire et ordre

Après les paramètres du constructeur, la liste accepte :

```text
':' [parent(arguments) ','] Champ(expression)
    [',' Champ(expression)]...
```

Les règles sont déterministes :

- `parent(...)` ou `super(...)`, lorsqu’il est présent, doit être la première
  entrée ;
- chaque nom de champ désigne un champ déclaré directement dans la classe du
  constructeur ;
- un champ hérité appartient au constructeur de sa propre classe et ne peut
  pas être réinitialisé depuis la liste dérivée ;
- chaque champ apparaît au maximum une fois ;
- les champs listés suivent leur ordre de déclaration, même si certains champs
  intermédiaires sont omis ;
- chaque initialiseur de champ contient exactement une expression ;
- un alias de champ direct est accepté puis normalisé vers le champ canonique ;
- une déclaration externe de constructeur dans un `.HGsPP` ne contient pas la
  liste ; celle-ci appartient à la définition `.GsPP`.

Exiger l’ordre de déclaration rend également l’ordre d’évaluation visible dans
le code source et évite qu’un réordonnancement silencieux du backend change les
effets de bord.

## Types pris en charge

L’expression est contrôlée par les mêmes règles d’initialisation que les
variables locales. Gs++ 0.21 accepte :

- les entiers, booléens, octets, caractères et énumérations ;
- les pointeurs ordinaires et pointeurs de fonction ;
- les champs `constante/const`, qui peuvent ainsi recevoir leur valeur avant le
  corps du constructeur ;
- les structures et unions non classes, par copie ou agrégat ;
- les tableaux fixes, avec un agrégat obligatoire ;
- les littéraux chaîne UTF-8 et les résultats structurés temporaires ;
- les expressions employant paramètres, globales, fonctions ou champs déjà
  disponibles.

Les champs non listés conservent les octets nuls établis lors de la création de
l’objet. Le corps peut ensuite modifier les champs qui ne sont pas constants.

Un champ objet dont le type est lui-même une classe reste refusé dans la liste.
Son constructeur et son destructeur exigent d’abord un contrat récursif complet
pour toutes les sorties de portée. Un pointeur vers une classe reste accepté,
car il ne possède pas la durée de vie de l’objet pointé.

## Ordre de génération

Le prologue x86-64 d’un constructeur déclaré exécute :

1. la sauvegarde du récepteur caché et des paramètres ;
2. l’évaluation des arguments et l’appel du constructeur de base ;
3. l’installation des tables virtuelles des étapes intermédiaires ;
4. l’installation de la table virtuelle de la classe courante ;
5. les initialisateurs de champs, dans l’ordre validé ;
6. le corps du constructeur.

La destruction conserve l’ordre inverse du modèle objet : destructeur de la
dérivée, puis base directe et racine. Les initialisateurs de champs non classes
n’ajoutent aucun appel de destruction caché.

Pour un champ, le backend calcule l’adresse `soi + décalage`, puis réutilise la
génération d’initialiseur typée. Les agrégats mettent d’abord le stockage du
champ à zéro avant d’écrire leurs éléments présents.

## Compilation séparée et ABI

La liste est une propriété de la définition du constructeur. Elle ne modifie ni
la disposition déclarée de la classe ni sa signature de type. Une interface et
une implémentation continuent donc d’échanger les signatures récursives
`GsAbi:x64-ms-v1:` existantes.

Le scénario séparé 0.21 compile l’implémentation et le consommateur en deux
GsObj portant `GSOBJ:0`, puis les lie en un GsE 1.0. La trace et la valeur
calculée sont identiques au scénario monolithique.

Tous les artefacts locaux doivent être reconstruits avec le compilateur
courant. Aucune couche de lecture d’une ancienne base locale n’est ajoutée.

## Périmètre validé

Les tests couvrent :

- combinaison `parent(...)` puis plusieurs champs ;
- classe racine avec liste de champs seulement ;
- champs constants, scalaires et pointeurs ;
- structures et tableaux initialisés par agrégats ;
- chaîne UTF-8 et structure temporaire retournée par une fonction ;
- appel de fonctions avec effets de bord dans les expressions ;
- normalisation d’un alias de champ ;
- champs omis puis affectés dans le corps ;
- identité de génération entre français et anglais ;
- rejet des doublons, de l’ordre inverse et d’une arité différente de un ;
- rejet des champs inconnus, hérités et objets de type classe ;
- exécution mono-fichier et séparée avec trace de construction `12345`, trace
  finale `1234567` et retour `75`.

La preuve complète est consignée dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.21.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.21.0.md).

## Limites explicites

Gs++ 0.21.0 ne revendique pas encore :

- la construction et la destruction récursives des champs objets classes ;
- les valeurs par défaut déclarées directement avec les champs ;
- la délégation d’un constructeur vers un autre constructeur de la même classe ;
- plusieurs bases, l’héritage virtuel, protégé ou privé ;
- la RTTI, les conversions descendantes et les virtuels purs ;
- les références comme champs, globales, tableaux ou retours ;
- les constructeurs globaux et les exceptions.

Ces limites restent explicites et diagnostiquées au lieu de produire une durée
de vie ou un contrat binaire ambigu.
