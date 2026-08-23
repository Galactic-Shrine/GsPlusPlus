# Validation du compilateur Gs++ 0.20.0

Date : 16 août 2026

## Résultat

L’initialisation explicite de la base directe et l’appel direct de
l’implémentation parent sont implémentés et validés. Les constructions propres
Windows/MSVC et GNU/WSL réussissent, les scénarios monolithique et séparé sont
réellement exécutés et l’image Sanctuaire SE reconstruite démarre sous
QEMU/OVMF.

Le Markdown est la documentation principale de ce jalon. Aucun `.docx` n’est
nécessaire pour rendre cette validation normative.

## Fonctionnalités exercées

Les tests couvrent :

- `: parent(arguments)` et l’alias anglais `: super(arguments)` ;
- résolution surchargée et indicateurs ABI des paramètres du constructeur de
  base ;
- accès aux constructeurs publics ou protégés et refus des privés ;
- appel automatique du constructeur sans argument en l’absence d’initialiseur ;
- chaîne de classes intermédiaires sans constructeur, sans double construction ;
- installation des tables virtuelles à chaque étape de construction ;
- `parent.Lire()`/`super.Lire()` lié directement à l’implémentation héritée ;
- conservation du dispatch virtuel pour un appel ordinaire via `Base&` ;
- destruction de la dérivée vers la racine ;
- identité du code et des données produits par les syntaxes française et
  anglaise ;
- rejet d’un initialiseur sur une racine ou une déclaration externe ;
- rejet d’arguments vers une base sans constructeur déclaré ;
- rejet d’un constructeur de base inaccessible ou non invocable ;
- rejet de `parent/super` hors d’une méthode ou dans une classe sans base ;
- conservation des contrats GsObj/GsA/GsE et ABI inter-unités.

## Scénarios exécutés

`Tests/Integration/InitialisationParent.GsPP` construit une base avec la valeur
`40`, puis une dérivée avec un bonus `2`. La trace vaut `12` après construction
et `1234` après destruction. L’appel virtuel via `Base&` retourne `42` ; l’appel
direct `parent.Lire()` retourne `40`. Le code final est donc `82`.

La compilation séparée produit :

- `InitialisationParentImplementation.GsObj` depuis l’implémentation ;
- `InitialisationParentPrincipal.GsObj` depuis l’interface `.GsPPH` et le
  consommateur ;
- `InitialisationParentSepare.GsE` après édition de liens.

Le GsE séparé est vérifié et retourne lui aussi `82`. Les scénarios 0.19 restent
exécutés en non-régression avec leurs retours `88` et `42`.

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

Les objets issus de `GSO:0`, `GSOBJ\0`, GsE 2.0 ou ABI 2 restent refusés. Il
n’existe ni couche de compatibilité ni convertisseur, car les projets et leurs
artefacts sont encore locaux et entièrement reconstruisibles.

## Windows/MSVC

- Visual Studio 2022 ;
- MSVC 19.44.35228.0 ;
- SDK Windows 10.0.26100.0 ;
- configuration Release x64 ;
- construction fraîche sous
  `Construction/Validation-GSPP-0200-MSVC` ;
- CTest : **4/4 réussis**.

Les suites sont `gspp_tests`, `gspp_autohebergement_tests`, la vérification du
noyau et son exécution hébergée.

## GNU/WSL

- GNU C++ 11.4.0 ;
- configuration Release ;
- construction fraîche sous
  `Construction/Validation-GSPP-0200-GNU` ;
- CTest : **5/5 réussis**.

Les cinq suites ajoutent l’intégration complète Gs++, la vérification/exécution
du noyau et les contrôles UEFI/FAT32. Le classificateur auto-hébergé concorde
avec le lexeur C++ sur **83 classifications**, dont `parent/super` et
`remplacer/override`.

GNU Make a signalé un léger décalage d’horloge entre Windows et WSL sur la
bibliothèque fraîchement créée ; la construction, la liaison et les cinq tests
se sont néanmoins terminés avec le code zéro.

## Non-régression Sanctuaire SE

La reconstruction complète produit exactement les empreintes déjà validées :

```text
fb62587c9efe9dbc5ae04d9222c9f45d03337bbdb6c3471e804e7fc9dec752bc  BOOTX64.EFI
3b2ca9f532627f6fca4fe75a08c340325c1621b4dddff1eed36d8742a95972bb  Noyau.GsE
eb442509638b8fbf2c327720726cf02a7dc26233bcacd173825a1e8d1ac28cca  Sanctuaire-ESP.img
```

Les tailles sont respectivement 26 112, 19 463 et 67 108 864 octets. Le noyau
n’utilise pas encore les nouvelles formes 0.20 ; l’identité binaire confirme
donc que leur génération ne modifie pas les programmes hors de leur périmètre.

## Démarrage QEMU/OVMF

Le test officiel `Tester-SanctuaireUEFI` 2.0.3 a démarré l’ESP fraîche avec
QEMU 11.0.50 et OVMF. Il a confirmé :

- le bandeau GS ;
- la mémoire verte en RGB `64,255,64` ;
- l’horloge cyan animée aux positions 11 et 27 ;
- l’injection QMP de la touche `a` ;
- le témoin clavier orange à la position 30.

Le rapport
`Construction/Validation-GSPP-0200-QEMU/Rapport-UEFI.json` indique
`Réussi / Passed`, code de sortie 0, en 6,144 secondes.

## Conclusion et limites

Le périmètre 0.20 est validé pour l’unique base publique directe. Une seule
forme `parent(...)`/`super(...)` est admise et les champs ne possèdent pas
encore leur propre entrée d’initialisation. L’héritage multiple/virtuel, les
visibilités d’héritage protégée/privée, les conversions descendantes, la RTTI,
les virtuels purs, les exceptions et la synthèse récursive des champs objets
restent hors de ce jalon.

Le contrat normatif complet est
[`../INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md`](../INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md).
