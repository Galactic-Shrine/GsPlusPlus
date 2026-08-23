# Validation du compilateur Gs++ 0.16.0

> Document historique : l’ABI 2 et GsE 2.0 étaient des numéros locaux à ce
> jalon. Gs++ 0.17.1 conserve leurs fonctionnalités sous ABI 1 et GsE 1.0.

Date : 22 juillet 2026.

## Objet du jalon

Gs++ 0.16.0 ajoute les opérations binaires nécessaires au code système, des
intrinsèques atomiques x86-64 et la première bibliothèque native
`GsSysteme.GsA`.

Cette évolution ne modifie pas les conteneurs : GsObj reste en version 1.0 avec
l’ABI 2, GsA reste en version 1.0 et GsE reste en version 2.0. La convention
d’appel structurée `GsAbi:x64-ms-v2` demeure inchangée.

## Couverture du langage et du backend

Les tests de la 0.16 couvrent :

- lexage, priorité, typage et génération de `~`, `&`, `^`, `|`, `<<` et `>>` ;
- décalages constants et dynamiques, logiques ou arithmétiques ;
- fermeture `>>` de types `pointeur_fonction` imbriqués sans ambiguïté avec un
  opérateur de décalage ;
- encodage de `xchg`, `lock xadd`, `lock cmpxchg`, `mfence` et `pause` ;
- charges et stockages atomiques 32 et 64 bits ;
- refus d’un prototype réservé d’intrinsèque incorrect ;
- absence de symbole ou relocalisation pour les intrinsèques intégrées ;
- suppression des prototypes d’interface inutilisés dans la table d’imports.

## Bibliothèque système

Le projet `GsPlusPlus/Bibliotheques/Systeme/GsSysteme.GsPj` produit quatre GsObj puis une
bibliothèque GsA déterministe :

- mémoire : remplissage, copie, comparaison et égalité ;
- vues : octets, texte, sous-vues, longueurs et recherches ;
- bits : rotations, comptage, inversion d’octets, puissance de deux et
  alignement ;
- atomiques : charge, stockage, échange, ajout, comparaison-échange, barrière,
  pause et verrou léger.

`GsPlusPlus/Tests/Integration/BibliothequeSysteme.GsPP` utilise l’interface anglaise `Gs::System`, lie
la bibliothèque, vérifie les chemins 32/64 bits puis retourne `64`. Le GsE
produit possède `0` import. Une seconde construction de `GsSysteme.GsA` est
identique bit à bit à la première.

## Validation complète

Une reconstruction propre avec `make -j2 test` réussit sans avertissement et
comprend :

- tous les tests unitaires du lexeur au chargeur ;
- exécution des tests système (`120`), callbacks (`44`), structures (`45`) et
  bibliothèque système (`64`) ;
- échange structuré entre GsObj avec le retour `46` ;
- bibliothèques GsA, projets, solutions, cartes de liens et diagnostics ABI ;
- reconstruction et exécution hébergée de Sanctuaire SE 0.10.2, retour `5` ;
- vérifications PE32+, chargeur UEFI sans DLL et relocalisations ;
- image FAT32 de 64 Mio et contrôles de reproductibilité.

## Analyseurs mémoire

Une construction distincte avec AddressSanitizer et
UndefinedBehaviorSanitizer réussit. Elle exécute :

- l’intégralité des tests unitaires 0.16 ;
- la compilation des quatre modules système ;
- la production de l’objet applicatif et de la bibliothèque GsA ;
- l’édition, la vérification et l’exécution du GsE final, retour `64`.

LeakSanitizer est désactivé parce que l’environnement lance les commandes sous
traçage. Les contrôles d’accès mémoire et de comportement indéfini restent
actifs avec arrêt à la première erreur.

## Construction CMake

La cible `bibliotheque_systeme` est fournie pour CMake et construit les quatre
GsObj dans le répertoire binaire avant de produire `GsSysteme.GsA`. CMake
n’est pas installé dans l’environnement de validation présent ; cette cible
n’a donc pas été exécutée ici. GNU Make constitue la chaîne effectivement
validée.

## Portée UEFI

Les contrôles automatisés UEFI et FAT32 passent. Sanctuaire SE 0.10.2 reste la
référence dont le démarrage sous QEMU + OVMF a été validé par l’utilisateur.
L’image reconstruite par Gs++ 0.16 n’a pas fait l’objet d’un nouveau démarrage
réel dans cet environnement.
