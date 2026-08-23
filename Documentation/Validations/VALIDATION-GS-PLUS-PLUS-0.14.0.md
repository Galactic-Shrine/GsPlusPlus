# Validation du compilateur Gs++ 0.14.0

> Document historique : les numéros de formats indiqués décrivent l’état local
> de ce jalon. Gs++ 0.17.1 fixe la base canonique GsObj/GsA/GsE 1.0 et ABI 1.

Date : 22 juillet 2026.

## Objet du jalon

La version 0.14.0 ajoute les pointeurs de fonction typés sans modifier les
formats GsObj 1.0, GsA 1.0 ni GsE 2.0. Elle étend leurs contenus ABI et leurs
relocalisations de façon compatible avec les fichiers qui n’utilisent pas de
callback.

La syntaxe canonique est
`pointeur_fonction<Retour(Paramètres)>`; son alias anglais est
`function_pointer<Return(Parameters)>`.

## Couverture fonctionnelle

Les tests couvrent :

- prise d’adresse avec `&Fonction` et conversion directe d’un nom de fonction ;
- affectation, comparaison et déréférencement d’un callback ;
- appels indirects depuis une locale, un paramètre, une globale, un champ et un tableau ;
- fonction retournant un pointeur de fonction ;
- vérification des arguments et du type de retour ;
- refus d’appeler une valeur entière ;
- refus d’affecter une signature incompatible ;
- propagation récursive de la signature dans l’ABI GsObj ;
- refus à la liaison de deux signatures de callback incompatibles ;
- génération de `call r11` selon l’ABI Microsoft x64 ;
- initialisation d’une callback globale et relocalisation GsE `BASE64` ;
- refus d’un indice réservé `BASE64` falsifié ;
- application de la relocalisation par les chargeurs hébergé et UEFI.

Le programme `GsPlusPlus/Tests/Integration/PointeursFonction.GsPP` combine ces formes et retourne `44`
après chargement réel de son image GsE.

## Non-régressions attendues

La validation complète comprend également :

- tests unitaires du lexeur, du parseur, du typage et du backend ;
- compilation séparée `.GsPPH`/`.GsPlusPlusHeader` vers `.GsObj` ;
- bibliothèques GsA, extraction à la demande et cartes de liens ;
- reconstruction et exécution hébergée de Sanctuaire SE 0.10.2, retour `5` ;
- vérification PE32+, construction de `BOOTX64.EFI` et image FAT32 ;
- reproductibilité des objets, bibliothèques, exécutables et archives ;
- AddressSanitizer et UndefinedBehaviorSanitizer ;
- reconstruction autonome depuis l’archive livrée.

La validation UEFI réelle acquise reste celle de Sanctuaire SE 0.10.2. Le
chargeur UEFI 0.14 compile et sait appliquer `BASE64`; l’image de référence du
noyau ne contient toutefois pas encore de callback globale nécessitant cette
nouvelle relocalisation.
