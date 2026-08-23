# Validation du compilateur Gs++ 0.17.1

Date : 15 août 2026

## Décision canonique

Gs++ 0.17.1 remet à zéro la numérotation de ses formats encore locaux avant
toute publication. Cette opération ne retire aucune fonctionnalité : elle fixe
la première base canonique commune à toute la chaîne.

| Contrat | Valeur canonique |
|---|---|
| Signature GsObj | `GSOBJ:0` puis un octet réservé nul |
| Format GsObj | 1.0 |
| Format GsA | 1.0 |
| Format GsE | 1.0 |
| ABI GsObj | 1 |
| ABI des imports GsE | 1 |
| Préfixe ABI textuel | `GsAbi:x64-ms-v1:` |

Le protocole de démarrage `ContexteDemarrage.Version = 3` reste distinct : il
versionne l’échange entre le chargeur UEFI et le noyau, pas les formats de
fichier ni l’ABI de liaison Gs++.

## Rupture locale assumée

Les anciens artefacts ne sont pas convertis et aucune lecture compatible n’est
conservée. Le lecteur GsObj refuse `GSOBJ\0` et la cible intermédiaire `GSO:0`.
Le vérificateur GsE refuse l’ancienne version locale 2.0 et les imports portant
le champ ABI 2. Les objets, bibliothèques, exécutables et images doivent être
reconstruits avec Gs++ 0.17.1.

## Contrôles binaires

- les huit premiers octets GsObj valent
  `47 53 4F 42 4A 3A 30 00` (`GSOBJ:0\0`) ;
- la version GsObj aux octets 8 à 11 vaut 1.0 ;
- l’architecture aux octets 12 et 13 reste AMD64 (`0x8664`) ;
- le champ ABI GsObj aux octets 14 et 15 vaut 1 ;
- l’en-tête GsObj conserve sa taille de 112 octets ;
- l’en-tête GsA annonce 1.0 ;
- l’en-tête GsE annonce 1.0 et ses imports annoncent l’ABI 1.

## Non-régressions

La validation doit reconstruire et exercer :

- `GsSysteme.GsA` et ses quatre objets ;
- `GsHebergee.GsA` et ses trois objets ;
- le classificateur de mots-clés auto-hébergé et ses 63 comparaisons ;
- le test de bibliothèque hébergée ;
- `Noyau.GsE`, son vérificateur et son exécution hébergée ;
- les tests de valeurs structurées, callbacks, compilation séparée,
  reproductibilité, archives et diagnostics ABI.

La validation propre a réussi sous les deux environnements : 4/4 tests CTest
sous Windows/MSVC et 5/5 sous GNU/WSL, y compris le script d’intégration, la
vérification/exécution du noyau et l’intégration UEFI structurelle.

Le test UEFI réel `Tester-SanctuaireUEFI` 2.0.3 a ensuite démarré la nouvelle
image le 15 août 2026 avec QEMU 11.0.50 et OVMF. Le bandeau GS, la mémoire
verte, l’horloge cyan animée et le clavier orange ont tous été validés.

```text
fb62587c9efe9dbc5ae04d9222c9f45d03337bbdb6c3471e804e7fc9dec752bc  BOOTX64.EFI
3b2ca9f532627f6fca4fe75a08c340325c1621b4dddff1eed36d8742a95972bb  Noyau.GsE
eb442509638b8fbf2c327720726cf02a7dc26233bcacd173825a1e8d1ac28cca  Sanctuaire-ESP.img
```
