# Valeurs structurées de Gs++ 0.15

Gs++ 0.15 rend les structures et unions utilisables comme de véritables
valeurs. Elles peuvent être initialisées, copiées, affectées, transmises à une
fonction et retournées. Ces opérations restent entièrement statiques : elles
n’exigent ni tas, ni constructeur caché, ni bibliothèque d’exécution.

## Initialisation agrégée

Une liste entre accolades suit l’ordre de déclaration des champs ou des
éléments :

```gspp
structure Point
{
    entier32 X;
    entier32 Y;
};

structure Rectangle
{
    Point Debut;
    Point Fin;
    entier32 Couleurs[3];
};

Rectangle zone = {{10, 20}, {30, 40}, {255, 128}};
```

Les éléments absents sont mis à zéro. Le tableau `Couleurs` ci-dessus vaut
donc `{255, 128, 0}`. Les octets de bourrage d’une structure sont également
initialisés à zéro, ce qui rend la sortie déterministe. Une virgule finale et
la liste vide `{}` sont acceptées.

Une union accepte zéro ou un élément ; cet élément initialise son premier
champ. Les initialisateurs désignés par nom ne font pas encore partie du
langage.

Les mêmes règles s’appliquent aux globales. Le compilateur aplatit les
constantes dans `.donnees` et émet une relocalisation `Adresse64` lorsqu’un
champ contient l’adresse d’une fonction.

## Copies et affectations

Une copie reproduit tous les octets de la valeur dans un stockage distinct :

```gspp
Point original = {20, 22};
Point copie = original;
original.X = 1;
copie = {10, 32};
```

Modifier `original` ne modifie pas `copie`. Le type de la source et celui de la
destination doivent être identiques. Les tableaux fixes acceptent une
initialisation agrégée, mais leur affectation ou copie directe reste refusée.

## Paramètres par valeur

À l’appel, une structure est représentée par l’adresse de sa valeur source.
Le prologue de la fonction la copie immédiatement dans son propre cadre de
pile. Le paramètre possède donc la sémantique d’une valeur indépendante :

```gspp
entier32 LireApresModification(Point point)
{
    point.X = 99;
    retourner point.X;
}
```

La fonction ne peut pas modifier le `Point` de l’appelant par ce paramètre.
Un pointeur explicite reste nécessaire pour demander un partage du stockage.

## Retours de structures

L’appelant réserve une zone de résultat et transmet son adresse dans `RCX`.
Les paramètres explicites sont alors décalés vers `RDX`, `R8` et `R9`. Le
destinataire écrit le résultat dans cette zone et renvoie son adresse dans
`RAX` :

```gspp
Point Construire(entier32 x, entier32 y)
{
    retourner {x, y};
}
```

Cette convention est identique pour un appel direct, un appel par pointeur de
fonction et un appel entre deux objets natifs. Une fonction retournant une
structure accepte au maximum trois paramètres explicites ; les autres
fonctions conservent quatre emplacements.

Le backend réserve dans le cadre de pile des zones temporaires de taille et
d’alignement exacts pour les résultats structurés et les listes utilisées
comme arguments. Il n’effectue aucune allocation dynamique.

## Version ABI

Le contrat de symbole devient :

```text
GsAbi:x64-ms-v1:
```

Le champ ABI canonique de GsObj vaut `1`, tout comme celui des imports GsE. Le
conteneur reste `GsObj 1.0` et le format exécutable est `GsE 1.0`. Gs++ 0.15
avait d’abord numéroté ce contrat localement en ABI 2 ; Gs++ 0.17.1 le
renumérote en ABI 1 sans retirer les règles de passage structuré. Les objets
portant l’ancienne numérotation locale 2 sont refusés et doivent être
recompilés depuis leurs sources ou interfaces `.HGsPP`/`.HeaderGsPlusPlus`.

## Couverture de validation

`GsPlusPlus/Tests/Integration/ValeursStructures.GsPP` vérifie les structures petites et grandes, les
agrégats imbriqués, la mise à zéro, les copies de globales, les paramètres et
retours par valeur, les callbacks structurés et les accès à un champ d’un
résultat temporaire. Son code de retour attendu est `45`.

Le scénario `GsPlusPlus/Tests/Integration/Separation/Valeurs*` compile une définition et un
consommateur en deux `.GsObj`, les lie, puis exécute le GsE obtenu. Le retour
attendu est `46` et confirme que la copie du paramètre ne modifie pas la valeur
du consommateur.
