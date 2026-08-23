# Pointeurs de fonction de Gs++ 0.14

Gs++ 0.14 ajoute un type de callback explicite, vérifié statiquement et inclus
dans l’ABI des unités compilées séparément.

## Syntaxe bilingue

```gspp
pointeur_fonction<entier32(entier32, booléen)>
function_pointer<int32(int32, bool)>
```

La partie précédant les parenthèses est le type de retour. Les types placés
entre parenthèses sont les paramètres, dans l’ordre. Les deux formes sont
strictement équivalentes.

Une signature accepte actuellement jusqu’à quatre paramètres, comme les appels
Gs++ ordinaires. Les paramètres et le retour doivent être des types scalaires,
des pointeurs ou d’autres pointeurs de fonction. Les structures, unions et
tableaux ne sont pas encore transmis par valeur.

## Création et stockage

L’adresse d’une fonction peut être obtenue explicitement avec `&` :

```gspp
publique entier32 Doubler(entier32 valeur)
{
    retourner valeur * 2;
}

pointeur_fonction<entier32(entier32)> operation = &Doubler;
```

Un nom de fonction se convertit également vers sa signature sans `&` :

```gspp
operation = Doubler;
```

Un callback occupe huit octets sur x86-64. Il peut être stocké dans une
variable locale ou globale, un paramètre, un champ de structure ou un tableau.
Il peut aussi être retourné par une fonction.

## Appels indirects

Toute expression dont le type est un pointeur de fonction peut être appelée :

```gspp
operation(21);
(*operation)(21);
objet.Executer(21);
table[index](21);
Choisir(vrai)(21);
```

Le compilateur vérifie le nombre d’arguments et chacun de leurs types avant la
génération. Une valeur entière ou un pointeur de données ne peut pas être
appelé. Deux signatures de callbacks différentes ne sont pas affectables sans
conversion explicite valide.

## ABI x86-64

Les appels indirects suivent la même ABI Microsoft x64 que les appels directs :

- quatre premiers arguments dans `RCX`, `RDX`, `R8` et `R9` ;
- espace d’accueil de 32 octets réservé par l’appelant ;
- pile alignée avant l’appel ;
- valeur de retour dans `RAX` ;
- cible indirecte conservée dans `R11`, puis appelée avec `call r11`.

La signature ABI GsObj encode récursivement le retour et les paramètres du
callback. L’éditeur de liens refuse donc une déclaration et une définition qui
diffèrent uniquement par la signature d’un pointeur de fonction.

## Globales et format GsE

Une globale callback peut être initialisée par une fonction définie :

```gspp
publique pointeur_fonction<entier32(entier32)> Operation = &Doubler;
```

L’objet natif contient une relocalisation absolue `Adresse64`. Lors de la
production du GsE, elle devient une relocalisation interne `BASE64` composée de
la base réelle de chargement et du RVA de la fonction. `gsechargeur` et
`BOOTX64.EFI` appliquent cette relocalisation avant tout appel au programme.

Les initialiseurs calculés, les callbacks externes non résolus et les listes
d’initialisation agrégées ne sont pas encore autorisés pour une globale.

## Exemple complet

```gspp
publique entier32 AjouterUn(entier32 valeur)
{
    retourner valeur + 1;
}

publique entier32 Appliquer(
    pointeur_fonction<entier32(entier32)> rappel,
    entier32 valeur)
{
    retourner rappel(valeur);
}

publique entier32 Principal()
{
    pointeur_fonction<entier32(entier32)> rappel = AjouterUn;
    retourner Appliquer(rappel, 41);
}
```

Ce programme retourne `42`.
