# Validation du compilateur Gs++ 0.15.0

> Document historique : l’ABI 2 et GsE 2.0 étaient des numéros locaux à ce
> jalon. Gs++ 0.17.1 conserve leurs fonctionnalités sous ABI 1 et GsE 1.0.

Date : 22 juillet 2026.

## Objet du jalon

Gs++ 0.15.0 ajoute les copies, les initialisations agrégées et le passage ou
retour des structures et unions par valeur. La convention est volontairement
incompatible avec les objets précédents : les signatures deviennent
`GsAbi:x64-ms-v2`, le champ ABI de GsObj vaut `2` et les imports GsE produits
portent l’ABI `2`.

Le format conteneur reste GsObj 1.0, les bibliothèques restent GsA 1.0 et le
format exécutable reste GsE 2.0. Les `.GsObj` ABI 1 sont refusés et doivent être
recompilés.

## Couverture fonctionnelle

Les tests de la 0.15 couvrent :

- agrégats vides, partiels, imbriqués et avec virgule finale ;
- structures, unions, tableaux fixes et scalaires contextualisés ;
- mise à zéro des éléments omis et des octets de bourrage ;
- globales agrégées et pointeurs de fonction dans un champ imbriqué ;
- copies lors d’une déclaration et d’une affectation ;
- évaluation complète de la source avant écriture de la cible, notamment
  `point = {point.Y, point.X}` ;
- structures de 8, 16 et 40 octets ;
- paramètres structurés copiés dans le cadre local ;
- retours structurés directs et appels indirects par callback ;
- accès à un champ d’un résultat temporaire ;
- liste structurée fournie directement comme argument ;
- liaison de deux objets GsObj échangeant une structure par valeur ;
- refus des agrégats trop longs, unions multiéléments, copies de types
  différents et retours structurés dépassant trois paramètres explicites ;
- refus des objets GsObj ABI 1 et des imports GsE ABI 1.

`GsPlusPlus/Tests/Integration/ValeursStructures.GsPP` est compilé, vérifié, chargé et réellement
exécuté ; son retour attendu et obtenu est `45`. Le scénario séparé
`GsPlusPlus/Tests/Integration/Separation/Valeurs*` produit deux `.GsObj`, les lie et exécute le GsE
résultant avec le retour `46`.

## Validation complète

La commande `make test` réussit et comprend :

- tests unitaires du lexeur, du parseur, du typage, du backend et des formats ;
- exécution des tests système (`120`), callbacks (`44`) et structures (`45`) ;
- compilation séparée, bibliothèques GsA, projets, cartes de liens et
  diagnostics d’incompatibilité ABI ;
- trajet structuré inter-unités avec le retour `46` ;
- reconstruction et exécution hébergée de Sanctuaire SE 0.10.2, retour `5` ;
- vérification PE32+, chargeur UEFI sans import de DLL et relocalisations ;
- image FAT32 de 64 Mio et comparaison bit à bit de sa reproduction ;
- reproduction des objets, bibliothèques, GsE et cartes de liens.

Une construction dédiée avec AddressSanitizer et UndefinedBehaviorSanitizer
réussit également. Les tests unitaires instrumentés passent, puis le compilateur
instrumenté produit `ValeursStructures.GsE`, que le vérificateur et le chargeur
instrumentés exécutent avec le retour `45`. La détection de fuites de
LeakSanitizer est désactivée dans cet environnement, qui exécute les processus
sous traçage ; les contrôles d’accès mémoire et de comportement indéfini restent
actifs avec arrêt à la première erreur.

## Portée UEFI

Les contrôles automatisés UEFI/FAT32 de la 0.15 passent. Sanctuaire SE 0.10.2
reste la référence dont le démarrage réel sous QEMU + OVMF a été validé par
l’utilisateur. L’image reconstruite avec le compilateur 0.15 n’a pas reçu un
nouveau test de démarrage réel dans cet environnement ; aucun QEMU n’y est
installé.
