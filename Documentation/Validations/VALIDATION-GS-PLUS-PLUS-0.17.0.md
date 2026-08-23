# Validation du compilateur Gs++ 0.17.0

> Document historique remplacé par la validation 0.17.1 pour les formats : la
> base canonique est `GSOBJ:0`, GsObj/GsA/GsE 1.0 et ABI 1.

Date : 23 juillet 2026

## Portée

La validation couvre les littéraux chaîne UTF-8, la logique à court-circuit,
la bibliothèque hébergée, le premier composant du compilateur écrit en Gs++,
les formats GsObj/GsA/GsE et toutes les non-régressions de Sanctuaire SE
0.10.2.

## Tests du langage

- chaînes terminées par zéro dans `.data` ;
- partage déterministe des littéraux identiques ;
- relocalisation `REL32` du code vers le symbole local de chaîne ;
- échappements valides et refus des chaînes ou échappements incomplets ;
- refus d’une écriture directe dans une chaîne constante ;
- priorités de `&&` et `||` ;
- court-circuit d’exécution et d’évaluation constante.

## Bibliothèque hébergée

`GsHebergee.GsA` est reconstruite à partir de trois objets :

- `VuesEtFlux.GsObj` ;
- `Conteneurs.GsObj` ;
- `Hote.GsObj`.

L’image de test exerce réellement :

- un fichier de six octets lu puis réécrit ;
- les lectures et écritures partielles des flux bornés ;
- un vecteur à stockage externe ;
- insertion, remplacement potentiel et recherche dans une table de symboles ;
- un diagnostic UTF-8 structuré ;
- deux expressions à court-circuit dont la branche droite divise par zéro.

Le point d’entrée retourne `170`.

## Premier composant auto-hébergé

`ClassificateurMotsCles.GsPP` est compilé puis lié avec la bibliothèque
hébergée. Le banc C++ charge son export
`Gs::Autohebergement::ClassifierMotCle` et compare son résultat au
classificateur du lexeur C++.

Les 63 comparaisons couvrent les mots-clés français, les alias anglais, les
accents et trois non-mots. Toutes concordent.

## Chaîne complète

Commandes de référence :

```bash
make test

make clean
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
make CXXFLAGS="-std=c++20 -O1 -g -Wall -Wextra -Wpedantic \
  -fsanitize=address,undefined -fno-omit-frame-pointer" test
```

La détection de fuites est désactivée uniquement parce que LeakSanitizer ne
peut pas parcourir `/proc` dans l’environnement supervisé de validation.
Toutes les autres vérifications AddressSanitizer et UndefinedBehaviorSanitizer
restent actives.

La validation comprend :

- tests unitaires C++ ;
- exécution des programmes GsE scalaires, callbacks et structures ;
- exécution de `GsSysteme.GsA` et `GsHebergee.GsA` ;
- comparaison C++/Gs++ du classificateur ;
- vérification structurelle GsE ;
- compilation séparée, cartes de liens et reproductibilité GsObj/GsA/GsE ;
- chargeur hôte ;
- reconstruction de `Noyau.GsE`, `BOOTX64.EFI` et de l’image FAT32 ;
- tests AddressSanitizer et UndefinedBehaviorSanitizer.

## UEFI

Sanctuaire SE 0.10.2 reste la référence déjà validée en démarrage UEFI réel
dans une machine virtuelle. Gs++ 0.17 reconstruit la même base noyau et la
soumet aux contrôles PE32+, GsE et FAT32. Aucun nouveau démarrage UEFI réel
n’est revendiqué pour l’image reconstruite pendant cette validation locale.

## Conclusion

Gs++ 0.17.0 atteint le jalon de préparation mesurable à l’auto-hébergement :
un composant du compilateur écrit en Gs++ est compilé, chargé, exécuté et
comparé automatiquement à l’implémentation de référence.
