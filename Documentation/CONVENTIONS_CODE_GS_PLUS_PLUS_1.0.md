# Conventions de code Gs++ 1.0

**CONVENTION CANONIQUE — applicable aux sources et interfaces livrées avec
Gs++.**

Ce document fixe la présentation du code maintenu par ⋞Galactic-Shrine⋟. Il
ne modifie ni la grammaire ni l'ABI du langage. Un projet utilisateur peut
adopter sa propre convention tant que son code reste syntaxiquement valide.

## Espaces de noms

Les API publiques appartenant au produit Gs++ utilisent le préfixe complet :

```cpp
GalacticShrine::GsPP::
```

Les domaines fonctionnels sont placés sous ce préfixe, par exemple :

```cpp
GalacticShrine::GsPP::Systeme
GalacticShrine::GsPP::Hebergee
GalacticShrine::GsPP::Autohebergement
```

Cette forme longue est conservée dans les interfaces, les symboles exportés,
les diagnostics, les métadonnées et tout contrat susceptible de participer à
l'ABI. Elle identifie clairement le propriétaire et le produit, et évite une
collision avec un espace `GsPP` défini par un autre logiciel.

`GsPP::` n'est pas un espace public de remplacement. Un raccourci local peut
être créé par le code consommateur lorsque le langage et le contexte le
permettent, mais il ne doit jamais changer le nom canonique d'un symbole
fourni par Gs++.

Les applications écrites en Gs++ ne sont pas obligées d'utiliser ce préfixe.
Elles choisissent leur propre identité, par exemple `MonStudio::MonProjet`.

## Accolades et indentation

- l'accolade ouvrante reste sur la ligne de la déclaration ou de
  l'instruction ;
- l'accolade fermante occupe sa propre ligne ;
- l'indentation utilise quatre espaces, sans tabulation ;
- une ligne vide sépare les étapes logiques d'une fonction ;
- une ligne vide est placée après l'ouverture d'une fonction documentée afin
  de détacher son corps de sa signature.

```cpp
espace GalacticShrine::GsPP::Exemple {

    publique entier32 Calculer(entier32 valeur) {

        si (valeur < 0) {
            retourner 0;
        }

        retourner valeur + 1;
    }
}
```

## Commentaires de documentation

Une API publique est documentée immédiatement avant sa déclaration ou sa
définition. Les commentaires multilignes utilisent exclusivement cette forme :

```cpp
/**
 * <résumé>Description concise.</résumé>
 * <remarque>Complément éventuel sur le contrat ou les limites.</remarque>
 **/
```

`<résumé>...</résumé>` est obligatoire pour une API publique. Les autres
sections XML comme `<remarque>...</remarque>`, `<contrat>...</contrat>` ou
`<exemple>...</exemple>` sont ajoutées seulement lorsqu'elles apportent une
information utile. Une section courte reste sur une ligne ; une section plus
longue peut placer ses balises d'ouverture et de fermeture sur des lignes
séparées.

Les balises canoniques sont écrites en français, conservent les accents et
incluent les types exposés par la signature :

| Balise | Rôle |
| --- | --- |
| `@Paramètre(type: nom)` | décrit le type et le nom d'un paramètre |
| `@Retourner(type)` | décrit le type et la valeur retournée |
| `@Erreur(code)` | décrit une erreur ou un échec observable |
| `@Remarque` | ajoute une précision d'utilisation |
| `@Depuis` | indique la version d'introduction du contrat |

Une fonction qui retourne `vide` n'a pas besoin de `@Retourner`. Une balise
ne doit pas être ajoutée si elle ne transmet aucune information utile.

```cpp
espace GalacticShrine::GsPP::Hebergee {

    /**
     * <résumé>Construit une vue non propriétaire sur une zone de caractères.</résumé>
     * @Paramètre(constante caractère*: donnees) Adresse du premier caractère.
     * @Paramètre(naturel64: taille) Nombre de caractères accessibles.
     * @Retourner(VueTexte) Vue formée à partir de l'adresse et de la taille.
     **/
    publique VueTexte CreerVueTexte(
        constante caractère* donnees, naturel64 taille) {

        retourner {donnees, taille};
    }
}
```

Les commentaires courts portant sur une seule décision interne peuvent
continuer à utiliser `//`. Plusieurs lignes de prose consécutives doivent être
regroupées dans un bloc `/** ... **/`.

## Nommage

- les symboles canoniques utilisent le vocabulaire français du langage ;
- les noms de types et de fonctions commencent par une majuscule ;
- les paramètres et variables locales commencent par une minuscule ;
- les alias anglais officiels désignent le même symbole et ne créent pas une
  seconde implémentation ;
- les noms publics évitent les abréviations ambiguës lorsque leur forme
  complète reste raisonnable.

## Portée de la convention

Cette convention s'applique aux nouveaux fichiers dès leur création. Le code
historique est migré lorsqu'il est touché ou lorsqu'une passe de formatage
dédiée est validée. Une modification purement visuelle ne doit jamais être
présentée comme une nouvelle capacité du langage.
