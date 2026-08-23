# Héritage du modèle objet Gs++ 0.19

> Ce document reste la référence historique du socle d’héritage 0.19.
> L’initialisation explicite et les appels directs de la base ajoutés en 0.20
> sont décrits dans
> [`INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md`](INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md).

## Statut

Ce document décrit l’héritage effectivement implémenté et validé dans Gs++
0.19.0. Il complète le modèle objet 0.18 sans modifier la signature
`GSOBJ:0`, les formats GsObj/GsA/GsE 1.0 ni l’ABI native numéro 1.

Le fichier Markdown est la source principale. Toute copie `.docx` éventuelle
est secondaire et ne change pas le présent contrat.

## Syntaxe

Une classe peut déclarer une seule base publique :

```gspp
classe Base
{
    protégée:
        entier32 Valeur;

    publique:
        constructeur() { soi.Valeur = 20; }
        virtuel entier32 Lire() { retourner soi.Valeur; }
        virtuel destructeur() {}
};

classe Derivee : publique Base
{
    privée:
        entier32 Bonus;

    publique:
        constructeur() { soi.Bonus = 2; }
        remplacer entier32 Lire()
        {
            retourner soi.Valeur + soi.Bonus;
        }
        remplacer destructeur() {}
};
```

Les alias anglais officiels sont `class`, `public`, `protected`, `private`,
`virtual`, `override`, `constructor`, `destructor`, `this` et `return`.

L’analyseur reconnaît aussi une visibilité d’héritage protégée ou privée afin
de produire un diagnostic précis, mais Gs++ 0.19 n’accepte sémantiquement que
l’héritage public.

## Résolution des membres et visibilité

La recherche d’un champ ou d’une méthode commence dans la classe statique de
l’expression, puis remonte la chaîne de bases jusqu’à la racine. Une
déclaration de la classe dérivée masque donc une déclaration ordinaire de même
nom dans une base.

Les règles d’accès sont :

| Visibilité du membre | Classe propriétaire | Classe dérivée | Code extérieur |
|---|---:|---:|---:|
| `publique/public` | oui | oui | oui |
| `protégée/protected` | oui | oui | non |
| `privée/private` | oui | non | non |

Le contrôle utilise la classe qui a réellement déclaré le membre. Un champ
protégé d’une base reste donc accessible dans une méthode de toute descendante,
mais pas depuis une fonction globale qui manipule la classe dérivée.

## Conversions de hiérarchie

Une valeur dérivée peut être liée implicitement à :

- une référence sur une base publique, par exemple `Base& base = derivee;` ;
- un pointeur sur une base publique, par exemple `Base* base = &derivee;` ;
- un paramètre `Base&` ou `Base*` dans la résolution de surcharge.

La sous-classe de base étant au décalage zéro, aucune correction de pointeur
n’est nécessaire dans ce jalon d’héritage simple.

Les opérations suivantes sont volontairement refusées :

- copie implicite d’une dérivée dans une base passée ou stockée par valeur ;
- conversion base vers dérivée ;
- conversion entre branches sans relation de descendance ;
- suppression de `const` pendant une conversion ;
- conversion de pointeurs de niveau supérieur à un.

Le refus des conversions par valeur empêche le slicing silencieux. Une future
fonctionnalité de copie de sous-objet devra être explicite et posséder son
propre contrat avant d’être acceptée.

## Disposition mémoire

Le sous-objet de base est toujours placé au décalage zéro. Les champs propres à
la dérivée commencent après la taille alignée de la base.

Pour une base déjà polymorphe :

```text
Derivee, décalage 0
├── pointeur de table virtuelle hérité
├── champs de Base
└── champs propres à Derivee
```

Le pointeur de table conserve le décalage défini par la base. Dans le cas
validé courant, une base polymorphe commence par ce pointeur au décalage zéro.

Si la base n’est pas polymorphe et que la dérivée introduit le premier virtuel,
le sous-objet de base reste au décalage zéro et le nouveau pointeur de table est
placé après la base, avec un alignement de huit octets. La conversion vers la
base ne change donc toujours pas l’adresse du sous-objet.

Les tests de disposition valident notamment :

| Classe | Taille | Pointeur virtuel | Champs |
|---|---:|---:|---|
| base polymorphe avec un `entier32` | 16 | 0 | premier champ à 8 |
| dérivée avec un `entier32` propre | 24 | 0 | champ propre à 16 |
| base non polymorphe avec un `entier32` | 4 | aucun | premier champ à 0 |
| dérivée introduisant le premier virtuel | 16 | 8 | base conservée à 0 |

## Remplacement des méthodes virtuelles

`remplacer/override` affirme qu’une méthode correspond exactement à un
emplacement virtuel hérité. La clé d’emplacement comprend le nom, les types des
paramètres explicites et le type de retour ; le récepteur caché n’en fait pas
partie.

Les règles sont strictes :

- une redéfinition d’un virtuel hérité doit porter `remplacer/override` ;
- `remplacer` échoue si aucun virtuel hérité ne possède la signature exacte ;
- un constructeur ne peut être ni virtuel ni remplacé ;
- un destructeur virtuel peut être remplacé avec `remplacer destructeur()` ;
- un remplacement conserve l’indice de l’emplacement de la base ;
- une nouvelle méthode `virtuel` est ajoutée à la fin de la table héritée ;
- les appels via `Base&` ou `Base*` utilisent l’emplacement de la base et
  atteignent l’implémentation de la classe dynamique.

Exiger le mot-clé `remplacer` évite qu’une modification de signature transforme
silencieusement un remplacement attendu en nouvelle méthode indépendante.

## Construction et destruction

Pour une variable locale de classe dérivée, le générateur :

1. met à zéro le stockage complet de l’objet ;
2. construit les bases de la racine vers la base directe ;
3. installe avant chaque constructeur la table correspondant à l’étape en
   cours ;
4. installe la table de la classe la plus dérivée ;
5. exécute le constructeur de la dérivée.

À la fin de la portée, sur chaque chemin déjà couvert par le RAII 0.18, les
destructeurs sont appelés dans l’ordre inverse : dérivée, base directe, puis
racine. La valeur de retour reste préservée pendant cette séquence.

En 0.19, chaque constructeur de base doit être accessible et invocable sans
argument explicite. Une base sans constructeur utilisable de cette forme
produit un diagnostic. Les listes d’initialisation et le passage d’arguments à
un constructeur de base ne sont pas encore définis.

## Compilation séparée et ABI

Les signatures `GsAbi:x64-ms-v1:` restent sous l’ABI numéro 1, mais leur
description de type contient désormais :

- la signature complète de la base ;
- le décalage du pointeur de table virtuelle ;
- l’ordre exact des emplacements virtuels ;
- la classe qui fournit l’implémentation de chaque emplacement.

Deux GsObj qui emploient le même nom de classe avec une hiérarchie, une
disposition ou un remplacement différent sont donc incompatibles. L’éditeur de
liens refuse leur symbole commun avec le diagnostic `incompatibilité ABI`.

Cette extension ne change ni la version du conteneur ni le numéro d’ABI : elle
précise le contenu déjà prévu par la signature récursive `x64-ms-v1`. Tous les
artefacts étant locaux, ils sont reconstruits avec le compilateur courant.

## Périmètre validé

Les tests couvrent le frontend français et anglais, les diagnostics, la
disposition, la table virtuelle, l’ABI inter-unités et l’exécution réelle. Le
scénario monolithique retourne `88` après trois appels dynamiques et confirme
un compteur de destruction égal à `11`. Le scénario séparé retourne `42` et
confirme la même chaîne de destruction entre deux GsObj.

La preuve complète est consignée dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.19.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.19.0.md).

## Limites explicites

Gs++ 0.19.0 ne revendique pas encore :

- l’héritage multiple ou virtuel ;
- l’héritage protégé ou privé ;
- des arguments ou listes d’initialisation pour les bases ;
- `super/base` pour nommer explicitement une implémentation de base ;
- les conversions contrôlées base vers dérivée ou l’information de type
  dynamique ;
- les interfaces abstraites, méthodes virtuelles pures ou classes finales ;
- la covariance des types de retour ;
- les références comme champs, globales, éléments de tableau ou retours ;
- la synthèse récursive des destructeurs des champs objets ;
- les constructeurs globaux et les exceptions.

Ces limites ne sont pas simulées. Toute syntaxe reconnue mais non prise en
charge reçoit un diagnostic au lieu de générer un contrat binaire ambigu.
