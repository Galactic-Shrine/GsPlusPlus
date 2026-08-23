# Validation du compilateur Gs++ 0.11.0

> Document historique : les numéros de formats indiqués décrivent l’état local
> de ce jalon. Gs++ 0.17.1 fixe la base canonique GsObj/GsA/GsE 1.0 et ABI 1.

## Résultat

Le jalon 0.11.0 est validé côté hôte. Il ajoute les alias applicatifs sans
modifier le format GsE 2.0 ni introduire de nouvelle fonctionnalité dans
Sanctuaire SE.

## Contrôles réussis

- reconstruction complète avec GNU Make, sans avertissement ;
- tests unitaires du lexeur, du parseur, de l’analyse sémantique et du backend ;
- alias de fonctions, globales, structures et champs ;
- résolution anticipée entre fichiers et chaînes d’alias ;
- refus des cycles, conflits et cibles absentes ;
- égalité d’adresse des symboles COFF canoniques et aliasés ;
- égalité d’adresse des exports GsE canoniques et aliasés ;
- point d’entrée GsE sélectionné par un alias public ;
- normalisation des relocalisations vers le symbole canonique ;
- un seul import pour une fonction externe appelée par son alias ;
- vérification et chargement GsE 2.0 ;
- exécution hébergée réelle du noyau de référence, code de retour `5` ;
- contrôles statiques PE32+, UEFI et FAT32 ;
- AddressSanitizer et UndefinedBehaviorSanitizer sur le compilateur, le
  vérificateur, le chargeur et l’exécution hébergée du noyau ;
- seconde reconstruction et comparaison binaire exacte.

LeakSanitizer a été désactivé pendant les essais instrumentés parce que
l’environnement de validation interdit son inspection de `/proc` sous traçage.
AddressSanitizer et UndefinedBehaviorSanitizer sont restés actifs.

## Effet de la migration du noyau témoin

Les fonctions-ponts anglaises ont été remplacées par des alias partageant les
implémentations françaises :

| Mesure | Gs++ 0.10.2 | Gs++ 0.11.0 |
|---|---:|---:|
| Code du noyau | 13 612 octets | 12 305 octets |
| Relocalisations | 271 | 239 |
| Taille de `Noyau.GsE` | 19 367 octets | 18 263 octets |

Les API anglaises restent présentes dans les exports. Les alias de fonctions
et de globales partagent l’adresse de leur déclaration française canonique.

## Empreintes reproductibles de la construction de référence

```text
595196edb0bce4a810f51e671c5a05dd5678ba5a354780c486fb7f3596da7528  Alias.obj
2b809eaeb30cfecd9c762d12c025b132951999c8becfe3ab101710080e216a84  Application.GsE
4b83b535d868fcf96fdb4390db3983d0a905412c4251b8c3338b868ad74d5a55  Noyau.GsE
2e5c8a64f8e3e431188463fb68002de75328f8864ce01526eded84790ef9179c  BOOTX64.EFI
727924cba4ba7a8d0b4adbadef062887491d87984c2225208983bbe95d10dad2  Sanctuaire-ESP.img
```

## Portée UEFI

L’image officielle de Sanctuaire SE 0.10.2, SHA-256
`9081371cc4b407d115e4ececc3d93d7e0eed43bf036e787b59bae12f4b3a451d`,
reste la référence réellement validée sous QEMU et OVMF. L’image reconstruite
par Gs++ 0.11.0 diffère parce que les fonctions-ponts ont disparu. Elle passe
les contrôles UEFI statiques, mais elle ne constitue pas une nouvelle version
de Sanctuaire SE et n’est pas annoncée comme réellement démarrée.
