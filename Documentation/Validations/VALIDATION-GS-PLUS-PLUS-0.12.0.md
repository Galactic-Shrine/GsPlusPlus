# Validation du compilateur Gs++ 0.12.0

> Document historique : les numéros de formats indiqués décrivent l’état local
> de ce jalon. Gs++ 0.17.1 fixe la base canonique GsObj/GsA/GsE 1.0 et ABI 1.

## Résultat

Le jalon 0.12.0 est validé côté hôte. Il ajoute les types système et leur ABI
x86-64 sans modifier le format GsE 2.0 ni ajouter de fonctionnalité à
Sanctuaire SE.

## Contrôles réussis

- reconstruction complète GNU Make, sans avertissement ;
- tests unitaires du lexeur, du parseur, de l’analyse sémantique et du backend ;
- tailles et alignements des scalaires 8, 16, 32 et 64 bits ;
- extensions signées et non signées lors des lectures, paramètres et retours ;
- divisions, restes et comparaisons signés ou non signés ;
- littéraux `naturel64` et `entier64` aux deux bornes de 64 bits ;
- sérialisation de globales de 1, 2, 4 et 8 octets ;
- booléens, octets, caractères, `const` et `volatile` ;
- tableaux fixes multidimensionnels, dispositions de structures et unions ;
- énumérations portées et conversions explicites ;
- refus des affectations constantes, tailles nulles et conversions constantes hors plage ;
- syntaxe française et alias anglais ;
- programme GsE `TypesSysteme.GsPP` exécuté réellement, code de retour `120` ;
- noyau Sanctuaire SE 0.10.2 reconstruit et exécuté réellement, code de retour `5` ;
- vérification et chargement GsE 2.0 ;
- contrôles statiques PE32+, UEFI et FAT32 ;
- AddressSanitizer et UndefinedBehaviorSanitizer, avec LeakSanitizer désactivé ;
- seconde reconstruction et comparaison binaire exacte des artefacts de référence.

## Correction Windows reprise

La base fournie ajoutait `NOMINMAX` et `WIN32_LEAN_AND_MEAN` avant
`windows.h`, ainsi que les conversions explicites des tailles de tableaux vers
`uint32_t`. Le journal Visual Studio 2026 joint indiquait que les tests 0.11
passaient sous Windows.

La reconstruction Linux a révélé qu’une conversion avait été appliquée au
mauvais membre du contexte : `CapacitePlagesMemoire` était écrit deux fois et
`NombrePlagesMemoireLibres` restait nul. La version 0.12 conserve la correction
de portabilité et initialise de nouveau les deux champs distincts. L’exécution
hébergée complète du noyau confirme la réparation.

## Mesures de la construction de référence

| Mesure | Gs++ 0.11.0 | Gs++ 0.12.0 |
|---|---:|---:|
| Code du noyau | 12 305 octets | 13 517 octets |
| Relocalisations du noyau | 239 | 239 |
| Taille de `Noyau.GsE` | 18 263 octets | 19 463 octets |
| Code du test de types | — | 2 811 octets |
| Données du test de types | — | 40 octets |

L’augmentation du code du noyau vient principalement de la normalisation
explicite des résultats `entier32` après les opérations. Elle garantit le
comportement à 32 bits au lieu de conserver accidentellement un résultat 64
bits dans `RAX`.

## Empreintes reproductibles

```text
31fc2bcff397f78e2f0efd798b7441ba76503fa9dcd902740cb71a19a0d619cb  Alias.obj
18903749ae08ea9cad76fa8106fcf3632d53a588a337d514a4a91435a8787d03  Application.GsE
7b2ae813f314163b1209e69fdd2477b1c3737b594b6c5ea5a4790c5124cff18c  TypesSysteme.GsE
8c1262f97736e35c54a5d4fd7383244c0a9792e746ea7c5a33437205e161d6e7  Noyau.GsE
2e5c8a64f8e3e431188463fb68002de75328f8864ce01526eded84790ef9179c  BOOTX64.EFI
2945be6c7090d84863b7d5fc8d2abf9d2fdb2ea07a293c1a2872c371e2e4c981  Sanctuaire-ESP.img
```

## Portée UEFI

L’image officielle Sanctuaire SE 0.10.2 d’empreinte
`9081371cc4b407d115e4ececc3d93d7e0eed43bf036e787b59bae12f4b3a451d`
reste la référence réellement démarrée sous QEMU et OVMF. L’image reconstruite
par Gs++ 0.12.0 passe les contrôles UEFI statiques et l’exécution hébergée, mais
son démarrage UEFI réel n’est pas revendiqué tant qu’un nouveau test QEMU/OVMF
n’a pas été effectué.
