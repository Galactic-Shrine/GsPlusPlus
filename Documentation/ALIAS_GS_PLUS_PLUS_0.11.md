# Alias applicatifs Gs++ 0.11

## Statut

Cette spécification décrit les alias applicatifs implantés par le compilateur
Gs++ 0.11.0. Le français reste la forme canonique d’une API ; les noms anglais
officiels peuvent désigner la même déclaration sans produire de fonction-pont,
de second stockage ou de champ supplémentaire.

## Grammaire

Au niveau d’un programme ou d’un espace de noms :

```text
déclaration-alias := "alias" nom-qualifié "=" nom-qualifié ";"
```

Dans une structure :

```text
alias-champ := "alias" identifiant "=" identifiant ";"
```

Exemple :

```gspp
espace Sanctuaire::Noyau
{
    structure ContexteDemarrage
    {
        entier32 TailleStructure;
        alias StructureSize = TailleStructure;
    };

    publique entier32 Demarrer(ContexteDemarrage* contexte)
    {
        retourner contexte->StructureSize;
    }

    alias BootContext = ContexteDemarrage;
    alias Start = Demarrer;
}
```

La déclaration peut précéder sa cible et peut se trouver dans un autre fichier
source de la même compilation. Une cible ou un nom d’alias entièrement qualifié
est accepté :

```gspp
alias Shrine::Kernel::Start = Sanctuaire::Noyau::Demarrer;
```

## Sémantique

Le compilateur résout chaque alias vers une déclaration canonique unique avant
l’analyse des expressions et la génération du code. Les chaînes sont permises :

```gspp
alias Start = Demarrer;
alias Begin = Start;
```

`Begin`, `Start` et `Demarrer` désignent alors la même fonction. Un cycle, une
cible absente, une redéclaration ou un conflit avec une déclaration réelle est
une erreur de compilation.

Les catégories prises en charge sont :

- fonctions ;
- variables globales ;
- structures ;
- champs de structures.

Un alias doit conserver la catégorie de sa cible. Il hérite également de sa
visibilité. Il n’est pas possible de rendre publique une cible privée en lui
ajoutant simplement un alias.

## Génération native

Pour une fonction définie, tous ses alias partagent le même décalage, la même
taille et le même corps machine. Pour une variable globale, ils partagent la
même section et le même décalage. Un alias de champ réutilise le type et le
décalage du champ canonique ; il ne modifie donc jamais la taille ni l’ABI de la
structure.

Dans un objet COFF, les noms applicatifs publics sont écrits comme plusieurs
symboles pointant vers la même adresse. Dans un fichier GsE 1.0, ils deviennent
plusieurs exports partageant la même adresse relogée. Un alias public peut être
choisi comme point d’entrée.

Les appels, accès aux globales et accès aux champs sont toujours normalisés vers
le nom canonique avant la génération des relocalisations. Un alias d’une
fonction `externe` ne crée donc pas un deuxième import : le fichier produit ne
demande que le symbole externe canonique.

## Limites du jalon 0.11

La version 0.11.0 ne fournit pas encore :

- l’alias global d’un espace de noms complet ;
- les alias de variables locales ou de paramètres ;
- la sélection entre plusieurs surcharges, puisque la surcharge de fonctions
  n’est pas encore implantée ;
- le renommage automatique d’une API entière.

Un alias de symbole public écrit dans GsE reste soumis à la limite défensive de
1 024 octets UTF-8 par nom.
