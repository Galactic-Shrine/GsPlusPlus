# Validation du compilateur Gs++ 0.18.0

Date : 16 août 2026

## Résultat

Le modèle objet système de Gs++ 0.18.0 est implémenté et validé. Les
constructions propres Windows/MSVC et GNU/WSL réussissent, les scénarios objets
sont réellement exécutés, les formats natifs restent sur la base canonique 1.0
et l’image Sanctuaire SE reconstruite démarre sous QEMU/OVMF.

Le Markdown est la documentation principale de ce jalon. Les éventuels
documents `.docx` homonymes sont uniquement des copies secondaires de
consultation.

## Contrats binaires conservés

| Contrat | Valeur validée |
|---|---|
| Signature GsObj | `GSOBJ:0` puis un octet réservé nul |
| Format GsObj | 1.0 |
| Format GsA | 1.0 |
| Format GsE | 1.0 |
| ABI GsObj | 1 |
| ABI des imports GsE | 1 |
| Préfixe ABI textuel | `GsAbi:x64-ms-v1:` |
| Architecture | AMD64, convention Microsoft x64 |

Un audit des artefacts frais a contrôlé, dans chacune des constructions GNU et
MSVC :

- neuf fichiers GsObj portant `GSOBJ:0\0`, la version 1.0 et l’ABI 1 ;
- deux bibliothèques GsA portant `GSA\0` et la version 1.0 ;
- trois exécutables GsE portant `GSE\0` et la version 1.0 ;
- les imports GsE et les signatures textuelles sous l’ABI 1.

Les tests de rejet des anciennes bases locales restent actifs : `GSOBJ\0`, la
cible intermédiaire `GSO:0`, le format GsE 2.0 et les champs ABI 2 ne sont pas
acceptés. Aucun convertisseur de compatibilité n’est ajouté, car tous les
projets concernés sont encore locaux et leurs artefacts peuvent être
reconstruits.

## Modèle objet exercé

La validation unitaire et d’intégration couvre :

- `classe/class`, visibilité publique, protégée et privée ;
- méthodes et paramètre caché `Classe& soi` ;
- constructeurs surchargés et initialisation préalable à zéro ;
- destructeurs locaux et RAII en ordre inverse des déclarations ;
- préservation de la valeur de retour pendant les destructions ;
- références locales et de paramètres avec marqueur ABI `R` ;
- surcharge de fonctions, constructeurs et opérateurs ;
- mangling déterministe seulement pour les groupes surchargés ;
- classes polymorphes avec pointeur de table au décalage 0 ;
- tables virtuelles locales en `.data` et relocalisations `Adresse64` ;
- contrôle de l’ordre des emplacements virtuels entre unités GsObj ;
- syntaxe française et anglaise produisant le même code machine ;
- refus des accès privés externes et des surcharges ambiguës.

Le scénario monolithique
`Tests/Integration/ModeleObjet.GsPP` combine constructeur, destructeur,
référence, surcharge, opérateur, méthode ordinaire et méthode virtuelle. Son
GsE est vérifié puis chargé et exécuté avec le code de retour `25`. Ce résultat
inclut la confirmation que le destructeur s’exécute exactement une fois à la
sortie de la portée.

Le scénario séparé compile l’interface, l’implémentation et le consommateur en
deux GsObj, les lie, vérifie le GsE final et l’exécute avec le code de retour
`42`. Un second test présente au linker deux ordres de tables virtuelles
incompatibles ; la liaison est correctement refusée comme incompatibilité ABI.

## Constructions propres

### Windows/MSVC

- Visual Studio 2022, MSVC 19.44.35228.0 ;
- SDK Windows 10.0.26100.0 ;
- configuration `Release` x64 ;
- reconstruction des compilateurs, bibliothèques, images auto-hébergées et du
  noyau ;
- CTest : **4/4 réussis**.

Les quatre suites sont les tests unitaires Gs++, l’auto-hébergement, la
vérification du noyau et son exécution hébergée.

### GNU/WSL

- construction propre de la cible `espace_travail` ;
- CTest : **5/5 réussis** ;
- intégration Gs++, compilation séparée, reproductibilité et diagnostics ;
- vérification et exécution hébergée du noyau ;
- contrôles PE32+, UEFI et FAT32 ;
- reproductibilité binaire du chargeur et de l’image ESP.

Le classificateur auto-hébergé concorde avec le lexeur C++ sur **79
classifications**, dont les nouveaux mots-clés français et anglais de 0.18.

## Non-régression Sanctuaire SE

L’optimisation RAII ne sauvegarde la valeur de retour que lorsqu’une portée
contient effectivement un objet à détruire. Grâce à cette règle, le noyau
Sanctuaire SE, qui n’utilise pas encore le modèle objet, reste identique octet
pour octet à la reconstruction validée avec Gs++ 0.17.1.

```text
fb62587c9efe9dbc5ae04d9222c9f45d03337bbdb6c3471e804e7fc9dec752bc  BOOTX64.EFI
3b2ca9f532627f6fca4fe75a08c340325c1621b4dddff1eed36d8742a95972bb  Noyau.GsE
eb442509638b8fbf2c327720726cf02a7dc26233bcacd173825a1e8d1ac28cca  Sanctuaire-ESP.img
```

Les tailles associées sont respectivement 26 112 octets, 19 463 octets et
67 108 864 octets.

Le test réel `Tester-SanctuaireUEFI` 2.0.3 a démarré cette image le 16 août
2026 avec QEMU 11.0.50 et OVMF. Il a confirmé :

- le bandeau GS ;
- le témoin mémoire vert, RGB `64,255,64` ;
- l’horloge cyan animée, passée des positions 14 à 30 ;
- l’injection QMP de la touche `a` ;
- le témoin clavier orange à la position 30.

Le rapport machine se trouve dans
`Construction/Validation-GSPP-0180-QEMU/Rapport-UEFI.json` et indique
`Réussi / Passed` en 5,891 secondes. Le lanceur officiel `.cmd` préfère
désormais PowerShell 7 lorsqu’il est installé et conserve un repli vers Windows
PowerShell.

## Limites assumées

Le jalon ne revendique pas encore l’héritage, le remplacement d’une méthode de
base, les références de champ/globale/tableau/retour, les constructeurs
globaux, la synthèse récursive des destructeurs de champs ni les exceptions.
Ces limites sont documentées dans
[`../MODELE_OBJET_GS_PLUS_PLUS_0.18.md`](../MODELE_OBJET_GS_PLUS_PLUS_0.18.md)
et ne remettent pas en cause le périmètre freestanding validé ici.
