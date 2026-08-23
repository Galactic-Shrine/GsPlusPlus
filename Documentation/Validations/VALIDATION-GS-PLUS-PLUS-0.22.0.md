# Validation du compilateur Gs++ 0.22.0

## Résultat

**VALIDÉ — 16 août 2026.**

La construction et la destruction récursives des champs objets classes sont
implémentées, exécutées et couvertes en compilation monolithique et séparée.
La non-régression du compilateur, des bibliothèques Gs++, du noyau, du chargeur
UEFI et de l’image ESP est confirmée.

## Périmètre 0.22 contrôlé

Les tests couvrent :

- `Champ(arguments)` avec résolution surchargée et paramètres valeur/référence ;
- champs classes omis et constructeur sans argument automatique ;
- classe imbriquée sans constructeur utilisateur ;
- table virtuelle installée à l’adresse d’un champ imbriqué ;
- ordre de construction base, champs déclarés, corps ;
- ordre de destruction destructeur courant, champs inversés, base ;
- destruction récursive avant un retour anticipé ;
- mêmes plans à partir d’une interface `.GsPPH` en compilation séparée ;
- diagnostics d’invocabilité et d’accès des constructeurs/destructeurs ;
- rejet documenté des tableaux de classes ;
- identité du code produit par les formes françaises et anglaises ;
- conservation des tests Gs++ 0.11 à 0.21.

## Scénario monolithique

Le fichier
[`../../Tests/Integration/ChampsObjetsClasses.GsPP`](../../Tests/Integration/ChampsObjetsClasses.GsPP)
construit :

1. `BaseConteneur` ;
2. `ChampExplicite` avec l’argument `20` ;
3. `ChampDefaut`, omis de la liste ;
4. `FeuilleImbriquee`, possédée par une classe polymorphe sans constructeur.

La trace de construction vaut `1234`. La méthode virtuelle du champ imbriqué
retourne `40`. Un retour de fonction déclenche ensuite les destructeurs dans
l’ordre `9,5,6,7,8`, et la fonction appelante observe `123495678`.

La somme `1 + 20 + 30 + 40` produit le code de retour GsE **91**.

## Scénario séparé

Les fichiers `ChampsObjetsClasses.GsPPH`,
`ChampsObjetsClassesImplementation.GsPP` et
`ChampsObjetsClassesPrincipal.GsPP` produisent deux GsObj 1.0, ensuite liés
vers `ChampsObjetsClassesSepares.GsE`.

Le plan de destruction est calculé dans l’unité consommatrice à partir de
l’interface et les symboles sont fournis par l’unité d’implémentation. Le GsE
retourne également **91**.

## Invariants de formats et d’ABI

| Contrôle | Valeur observée |
|---|---:|
| Signature objet | `GSOBJ:0` + octet nul |
| GsObj | 1.0 |
| GsA | 1.0 |
| GsE | 1.0 |
| ABI GsObj | 1 |
| ABI GsA | 1 |
| ABI import GsE | 1 |
| Signature | `GsAbi:x64-ms-v1` |

Aucune compatibilité avec `GSO:0` ou `GSOBJ\0` n’est ajoutée. Les artefacts
locaux sont reconstruits.

## Windows/MSVC

- Visual Studio 2022 ;
- MSVC 19.44.35228.0 ;
- SDK Windows 10.0.26100.0 ;
- configuration Release x64 ;
- construction fraîche : `Construction/Validation-GSPP-0220-MSVC` ;
- CTest : **4/4 réussis**.

Suites réussies :

1. `gspp_tests` ;
2. `gspp_autohebergement_tests` ;
3. `sanctuaire_noyau_verification` ;
4. `sanctuaire_noyau_execution`.

## GNU/WSL

- Ubuntu sous WSL ;
- GNU C++ 11.4.0 ;
- générateur Ninja, configuration Release ;
- construction fraîche : `Construction/Validation-GSPP-0220-GNU` ;
- CTest : **5/5 réussis**.

Suites réussies :

1. `gspp_tests` ;
2. `gspp_autohebergement_tests` ;
3. `gspp_integration` ;
4. `sanctuaire_noyau_integration` ;
5. `sanctuaire_uefi_integration`.

Le classificateur auto-hébergé reste concordant sur **83 classifications** ;
Gs++ 0.22 n’ajoute aucun mot-clé.

## Non-régression Sanctuaire SE

Les artefacts reconstruits conservent exactement les empreintes précédentes :

```text
fb62587c9efe9dbc5ae04d9222c9f45d03337bbdb6c3471e804e7fc9dec752bc  BOOTX64.EFI
3b2ca9f532627f6fca4fe75a08c340325c1621b4dddff1eed36d8742a95972bb  Noyau.GsE
eb442509638b8fbf2c327720726cf02a7dc26233bcacd173825a1e8d1ac28cca  Sanctuaire-ESP.img
```

Tailles observées :

| Artefact | Taille |
|---|---:|
| `BOOTX64.EFI` | 26 112 octets |
| `Noyau.GsE` | 19 463 octets |
| `Sanctuaire-ESP.img` | 67 108 864 octets |

Le noyau n’utilise pas encore les champs objets 0.22. Son identité binaire
confirme que le nouveau chemin n’altère pas les productions existantes.

## Démarrage QEMU/OVMF

`Tester-SanctuaireUEFI` 2.0.3 a exécuté l’image fraîche avec QEMU 11.0.50 et
OVMF. Le rapport
`Construction/Validation-GSPP-0220-QEMU/Rapport-UEFI.json` indique :

- résultat `Réussi / Passed` ;
- code de sortie `0` ;
- durée `5,838` secondes ;
- bandeau GS présent ;
- mémoire verte RGB `64,255,64` ;
- horloge cyan animée aux positions `10` et `26` ;
- touche `a` injectée par QMP `send-key` ;
- témoin clavier orange à la position `30`.

## Conclusion et limites

Le jalon 0.22 est validé pour les champs classes possédés individuellement par
valeur. Les tableaux de classes, constructeurs délégués, initialisateurs au
point de déclaration, constructeurs globaux, exceptions, héritage
multiple/virtuel, RTTI et virtuels purs restent hors périmètre.

Le contrat normatif complet est
[`../CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md`](../CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md).
