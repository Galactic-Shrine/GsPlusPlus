# Validation du compilateur Gs++ 0.21.0

Date : 16 août 2026

## Résultat

Les initialisateurs de champs directs de Gs++ 0.21.0 sont implémentés et
validés. Les constructions propres Windows/MSVC et GNU/WSL réussissent, les
scénarios monolithique et séparé sont réellement exécutés et l’image
Sanctuaire SE reconstruite démarre sous QEMU/OVMF.

Le Markdown est la documentation principale de ce jalon. Aucun `.docx` n’est
nécessaire pour rendre cette validation normative.

## Fonctionnalités exercées

Les tests couvrent :

- `: parent(valeur), Champ(expression)` et la forme anglaise avec `super` ;
- classe racine avec une liste composée uniquement de champs ;
- base obligatoirement placée avant les champs ;
- champs directs uniques et dans leur ordre de déclaration ;
- normalisation d’un alias de champ ;
- initialisation d’un champ `constante/const` ;
- scalaires, pointeurs, structures, tableaux et agrégats ;
- chaîne UTF-8 et valeur structurée temporaire dans une liste de champs ;
- mise à zéro des champs omis avant le corps ;
- appels avec effets de bord dans les expressions d’initialisation ;
- rejet des doublons, de l’ordre inverse et d’une arité différente de un ;
- rejet des champs inconnus ou hérités ;
- rejet d’un champ objet classe en attente de son contrat récursif ;
- identité du code et des données pour les syntaxes française et anglaise ;
- conservation du prologue `base → table virtuelle → champs → corps` ;
- conservation de la destruction `dérivée → base`.

## Scénarios exécutés

`Tests/Integration/InitialiseursChamps.GsPP` construit une base puis initialise
un champ constant, une structure, un tableau et un pointeur avant le corps. Les
effets de bord produisent la trace `12345` après construction. Les destructeurs
ajoutent `6` puis `7`, soit la trace finale `1234567`.

Le calcul additionne `40 + 2 + 3 + 4 + 5 + 6 + 7 + 8` et retourne donc `75`.

La compilation séparée produit :

- `InitialiseursChampsImplementation.GsObj` depuis l’implémentation ;
- `InitialiseursChampsPrincipal.GsObj` depuis l’interface `.GsPPH` et le
  consommateur ;
- `InitialiseursChampsSepares.GsE` après édition de liens.

Le GsE séparé est vérifié et retourne lui aussi `75`. Les scénarios 0.20 restent
exécutés avec leurs retours `82`, et les scénarios 0.19 avec `88` et `42`.

## Contrats binaires audités

| Contrat | Valeur observée |
|---|---|
| Signature GsObj | `GSOBJ:0` puis `00` |
| Version GsObj | 1.0 |
| Champ ABI GsObj | 1 |
| Signature GsA | `GSA\0` |
| Version GsA | 1.0 |
| Signature GsE | `GSE\0` |
| Version GsE | 1.0 |
| Champ ABI d’import GsE | 1 |
| Signatures de liaison | préfixe `GsAbi:x64-ms-v1:` |

Les anciennes bases locales restent refusées. Les projets étant locaux, leurs
artefacts sont reconstruits au lieu d’introduire une couche de compatibilité.

## Windows/MSVC

- Visual Studio 2022 ;
- MSVC 19.44.35228.0 ;
- SDK Windows 10.0.26100.0 ;
- configuration Release x64 ;
- construction fraîche sous
  `Construction/Validation-GSPP-0210-MSVC` ;
- CTest : **4/4 réussis**.

Les suites sont `gspp_tests`, `gspp_autohebergement_tests`, la vérification du
noyau et son exécution hébergée.

## GNU/WSL

- GNU C++ 11.4.0 ;
- configuration Release ;
- construction fraîche sous
  `Construction/Validation-GSPP-0210-GNU` ;
- CTest : **5/5 réussis**.

Les cinq suites ajoutent l’intégration complète Gs++, la vérification/exécution
du noyau et les contrôles UEFI/FAT32. Le classificateur auto-hébergé concorde
toujours avec le lexeur C++ sur **83 classifications** ; 0.21 n’ajoute aucun
mot-clé.

GNU Make a signalé un décalage maximal de 0,033 seconde entre les horloges
Windows et WSL. La construction, la liaison et les cinq tests se sont terminés
avec le code zéro.

## Non-régression Sanctuaire SE

La reconstruction complète conserve les empreintes validées :

```text
fb62587c9efe9dbc5ae04d9222c9f45d03337bbdb6c3471e804e7fc9dec752bc  BOOTX64.EFI
3b2ca9f532627f6fca4fe75a08c340325c1621b4dddff1eed36d8742a95972bb  Noyau.GsE
eb442509638b8fbf2c327720726cf02a7dc26233bcacd173825a1e8d1ac28cca  Sanctuaire-ESP.img
```

Les tailles restent 26 112, 19 463 et 67 108 864 octets. Le noyau n’emploie
pas encore les listes 0.21 ; son identité binaire confirme l’absence de
régression hors du nouveau chemin de génération.

## Démarrage QEMU/OVMF

Le test `Tester-SanctuaireUEFI` 2.0.3 a démarré l’ESP fraîche avec QEMU 11.0.50
et OVMF. Il a confirmé :

- le bandeau GS ;
- la mémoire verte en RGB `64,255,64` ;
- l’horloge cyan animée aux positions 6 et 23 ;
- l’injection QMP de la touche `a` ;
- le témoin clavier orange à la position 30.

Le rapport
`Construction/Validation-GSPP-0210-QEMU/Rapport-UEFI.json` indique
`Réussi / Passed`, code de sortie 0, en 6,414 secondes.

## Conclusion et limites

Le périmètre 0.21 est validé pour les champs directs ne possédant pas une durée
de vie de classe. La construction/destruction récursive de champs objets, les
valeurs par défaut au point de déclaration, la délégation entre constructeurs,
l’héritage multiple/virtuel, la RTTI, les virtuels purs et les exceptions
restent hors de ce jalon.

Le contrat normatif complet est
[`../INITIALISEURS_CHAMPS_GS_PLUS_PLUS_0.21.md`](../INITIALISEURS_CHAMPS_GS_PLUS_PLUS_0.21.md).
