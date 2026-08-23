# Impact des sous-systèmes sur Gs++

## Objectif

Les sous-systèmes Linux et Windows ainsi que le futur port natif d’Unreal
Engine imposent des besoins au langage, au compilateur et au SDK Gs++. Cette
documentation part du modèle objet Gs++ 0.23.0 désormais implémenté et précise
les capacités suivantes à stabiliser avant la plateforme applicative.

## Besoins communs

- ABI native versionnée ;
- compilation séparée fiable ;
- imports et exports dynamiques ;
- bibliothèques partagées natives ;
- structures, unions et dispositions binaires contrôlées ;
- pointeurs et callbacks ;
- atomiques et mémoire ordonnée ;
- exceptions matérielles et erreurs système distinctes ;
- interfaces C ou ABI neutres ;
- génération de symboles et informations de débogage ;
- compilation freestanding et hosted clairement séparée ;
- prise en charge des architectures futures sans figer x86-64 partout.

## Pour le sous-système Linux

- ABI System V AMD64 en plus de l’ABI native actuelle ;
- structures compatibles avec les interfaces POSIX/Linux ;
- appels indirects et transitions d’ABI ;
- chargeur et lecteur ELF ;
- relocalisations ELF nécessaires ;
- gestion des symboles dynamiques et versions de symboles ;
- outils de génération d’interfaces à partir de définitions externes.

L’ajout de System V ne doit pas remplacer l’ABI native Sanctuaire SE. Le
compilateur doit pouvoir sélectionner l’ABI au niveau d’une cible ou d’une
interface de compatibilité.

## Pour le sous-système Windows

- lecture PE/COFF utilisateur ;
- conventions d’appel Windows requises ;
- interopérabilité avec les composants de la couche Wine ;
- génération de ponts ABI ;
- isolation des bibliothèques et préfixes ;
- diagnostics des symboles et dépendances manquantes.

La présence actuelle d’une ABI Microsoft x64 dans Gs++ ne constitue pas une
compatibilité Win32. Les API Windows restent fournies par le sous-système.

## Pour Unreal Engine Sanctuaire SE

- compilateur et éditeur de liens utilisables comme chaîne externe ;
- réponse stable aux outils de construction ;
- bibliothèques statiques et partagées ;
- génération incrémentale ;
- fichiers de dépendances ;
- profils Debug, Development, Test et Shipping ;
- symboles de débogage ;
- optimisation et génération de code à grande échelle ;
- prise en charge de très grands projets ;
- interface C++/Gs++ clairement documentée ;
- SDK redistribuable destiné aux développeurs Sanctuaire SE ;
- capacité à construire l’éditeur Unreal et ses outils natifs en `.GsE`, pas
  uniquement les exécutables des jeux.

La chaîne Unreal doit produire directement des exécutables `.GsE`. Elle ne doit
pas dépendre du format `.GsP`, qui reste une option de distribution extérieure
à la compilation et à l’édition de liens.

Gs++ intervient dans la chaîne native, l’ABI, les bibliothèques et l’exécutable.
Il ne doit pas imposer de remplacement aux formats de projets, plugins, assets,
cartes, contenus cuisinés, shaders, archives ou configurations d’Unreal Engine.
Ces éléments restent produits et interprétés par Unreal Engine selon ses propres
mécanismes.

## Séquence proposée

```text
Produit Gs++ 1.0 : langage, bibliothèques et toolchain auto-hébergée stabilisés
    ↓
ABI et bibliothèques dynamiques natives
    ↓
SDK applicatif Sanctuaire SE
    ↓
ABI System V et outils ELF
    ↓
Interopérabilité requise par Wine
    ↓
Chaîne de plateforme Unreal Engine Sanctuaire SE
```
