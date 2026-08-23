# Validation du compilateur Gs++ 0.19.0

Date : 16 août 2026

## Résultat

L’héritage simple public de Gs++ 0.19.0 est implémenté et validé. Les
constructions propres Windows/MSVC et GNU/WSL réussissent, les scénarios
monolithique et séparé sont réellement exécutés, l’ABI rejette les hiérarchies
incompatibles et l’image Sanctuaire SE reconstruite démarre sous QEMU/OVMF.

Le Markdown est la documentation principale de ce jalon. Aucun `.docx` n’est
nécessaire pour rendre cette validation normative.

## Fonctionnalités exercées

Les tests couvrent :

- `classe Derivee : publique Base` et `class Derived : public Base` ;
- résolution des champs et méthodes dans toute la chaîne des bases ;
- accès aux membres protégés depuis une dérivée et refus depuis l’extérieur ;
- rejet de l’héritage protégé/privé, des cycles et des bases invalides ;
- conversions dérivée vers base pour `Base&` et `Base*` ;
- refus d’une conversion par valeur qui provoquerait un slicing ;
- `remplacer/override` exact et obligatoire sur un virtuel hérité ;
- rejet d’un remplacement sans cible virtuelle ou d’une redéfinition sans le
  mot-clé explicite ;
- sous-objet de base au décalage zéro ;
- réutilisation du pointeur de table d’une base polymorphe ;
- placement d’une première table après une base non polymorphe ;
- conservation des indices virtuels hérités et ajout de nouveaux emplacements ;
- construction des bases de la racine vers la dérivée ;
- destruction de la dérivée vers la racine ;
- dispatch dynamique au travers d’une référence et d’un pointeur de base ;
- identité du code et des données produits par les syntaxes française et
  anglaise ;
- contrôle de hiérarchie, disposition et fournisseur des virtuels dans les
  signatures d’ABI inter-unités.

## Scénarios exécutés

Le fichier `Tests/Integration/Heritage.GsPP` construit une base polymorphe et
une dérivée qui remplace la lecture et le destructeur, puis ajoute un troisième
virtuel. Les appels via `Base&`, `Base*` et la dérivée calculent
`22 + 22 + 44`, soit le code de retour `88`. À la sortie de portée, le compteur
de destruction doit être `11` : dix pour la dérivée et un pour la base.

La compilation séparée produit :

- `HeritageImplementation.GsObj` depuis l’implémentation ;
- `HeritagePrincipal.GsObj` depuis l’interface `.GsPPH` et le consommateur ;
- `HeritageSepare.GsE` après édition de liens.

Le GsE est vérifié et retourne `42`. Un test unitaire supplémentaire présente
au linker deux définitions qui divergent sur le fournisseur d’un emplacement
virtuel ; la liaison est refusée avec `incompatibilité ABI`.

## Contrats binaires audités

| Contrat | Valeur observée |
|---|---|
| Signature des neuf GsObj frais | `GSOBJ:0` puis `00` |
| Version GsObj | 1.0 |
| Champ ABI GsObj | 1 |
| Signature des deux GsA frais | `GSA\0` |
| Version GsA | 1.0 |
| Signature des trois GsE frais | `GSE\0` |
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
  `Construction/Validation-GSPP-0190-MSVC` ;
- CTest : **4/4 réussis**.

Les suites sont `gspp_tests`, `gspp_autohebergement_tests`, la vérification du
noyau et son exécution hébergée.

## GNU/WSL

- GNU C++ 11.4.0 ;
- configuration Release ;
- construction fraîche sous
  `Construction/Validation-GSPP-0190-GNU` ;
- CTest : **5/5 réussis**.

Les cinq suites ajoutent l’intégration complète Gs++, la vérification/exécution
du noyau et les contrôles UEFI/FAT32. Le classificateur auto-hébergé concorde
avec le lexeur C++ sur **81 classifications**, y compris
`remplacer/override`.

## Non-régression Sanctuaire SE

La reconstruction complète produit exactement les empreintes déjà validées :

```text
fb62587c9efe9dbc5ae04d9222c9f45d03337bbdb6c3471e804e7fc9dec752bc  BOOTX64.EFI
3b2ca9f532627f6fca4fe75a08c340325c1621b4dddff1eed36d8742a95972bb  Noyau.GsE
eb442509638b8fbf2c327720726cf02a7dc26233bcacd173825a1e8d1ac28cca  Sanctuaire-ESP.img
```

Les tailles sont respectivement 26 112, 19 463 et 67 108 864 octets. Le noyau
n’utilise pas encore l’héritage ; l’identité binaire confirme donc que la
nouvelle génération d’instructions ne modifie pas les programmes hors de son
périmètre.

## Démarrage QEMU/OVMF

Le test officiel `Tester-SanctuaireUEFI` 2.0.3 a démarré l’ESP fraîche avec
QEMU 11.0.50 et OVMF. Il a confirmé :

- le bandeau GS ;
- la mémoire verte en RGB `64,255,64` ;
- l’horloge cyan animée aux positions 9 et 24 ;
- l’injection QMP de la touche `a` ;
- le témoin clavier orange à la position 30.

Le rapport
`Construction/Validation-GSPP-0190-QEMU/Rapport-UEFI.json` indique
`Réussi / Passed`, code de sortie 0, en 5,592 secondes.

## Conclusion et limites

Le périmètre 0.19 est validé pour une seule base publique dont le constructeur
peut être appelé sans argument explicite. L’héritage multiple/virtuel, les
visibilités d’héritage protégée/privée, les listes d’initialisation de bases,
les conversions descendantes, la RTTI, les virtuels purs, les exceptions et la
synthèse récursive des champs objets restent hors de ce jalon.

Le contrat normatif complet est
[`../HERITAGE_GS_PLUS_PLUS_0.19.md`](../HERITAGE_GS_PLUS_PLUS_0.19.md).
