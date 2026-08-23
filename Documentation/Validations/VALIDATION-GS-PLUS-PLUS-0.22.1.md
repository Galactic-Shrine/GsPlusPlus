# Validation du compilateur Gs++ 0.22.1

**VALIDÉ — 16 août 2026.**

## Décision couverte

Cette révision locale fixe l’inventaire des extensions avant publication :

- sources Gs++ : `.Gs++`, `.GsPP`, `.GsPlusPlus` ;
- interfaces Gs++ : `.HGs++`, `.HGsPP`, `.HeaderGsPlusPlus` ;
- sources Gs# réservées : `.Gs#`, `.GsS`, `.GsSharp` ;
- aucun fichier d’en-tête Gs#, conformément au modèle C# ;
- projets/solutions : `.GsPj`, `.GsProject`, `.GsPs` ;
- production native : `.GsObj`, `.GsA`, `.GsE` ;
- extensions obsolètes refusées : `.GsPH`, `.GsO`, `.GsPPH`,
  `.GsPlusPlusHeader`.

Les signatures internes sont `GSOBJ:0`, `GSA:0` et `GSE:0`. Les trois
formats restent en version 1.0 et tous les champs ABI valent 1.

## Dispositions binaires vérifiées

| Format | Champ de signature | En-tête | Format | ABI |
| --- | --- | ---: | ---: | ---: |
| GsObj | `GSOBJ:0` + un zéro | 112 octets | 1.0 | 1 |
| GsA | `GSA:0` + trois zéros | 32 octets | 1.0 | 1 |
| GsE | `GSE:0` + trois zéros | 112 octets | 1.0 | 1 |

La migration GsA réordonne la version, le nombre de membres, l’ABI et la
taille totale dans les 32 octets existants. La migration GsE décale les champs
après la nouvelle signature mais conserve les tables et l’en-tête de 112
octets. Les lecteurs, le vérificateur, le chargeur hébergé, le chargeur UEFI et
l’outil d’image ESP utilisent tous la nouvelle disposition.

## Validation Windows/MSVC

Construction propre :

```powershell
cmake -S . -B Construction/Validation-GSPP-0221-MSVC `
  -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build Construction/Validation-GSPP-0221-MSVC `
  --config Release --target preparer_tests --parallel
ctest --test-dir Construction/Validation-GSPP-0221-MSVC `
  -C Release --output-on-failure
```

Résultat : **4/4 tests réussis** :

- tests unitaires Gs++ ;
- auto-hébergement partiel ;
- vérification du noyau Sanctuaire ;
- exécution hébergée du noyau, code de retour `5`.

## Validation GNU/WSL

Construction propre avec GNU C++ 11.4.0 :

```bash
cmake -S . -B Construction/Validation-GSPP-0221-GNU \
  -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build Construction/Validation-GSPP-0221-GNU --parallel
cmake --build Construction/Validation-GSPP-0221-GNU \
  --target preparer_tests --parallel
ctest --test-dir Construction/Validation-GSPP-0221-GNU --output-on-failure
```

Résultat : **5/5 tests réussis**, y compris :

- les trois variantes de source et d’interface Gs++ ;
- le rejet des extensions obsolètes ;
- le routage des sources Gs# hors de `gsppc` ;
- les signatures, versions et ABI GsObj/GsA/GsE ;
- la compilation séparée, les bibliothèques et l’auto-hébergement partiel ;
- le noyau, `BOOTX64.EFI` et l’image FAT32 de 64 Mio.

## Démarrage UEFI réel virtualisé

L’image GNU reconstruite a été lancée avec `Tester-SanctuaireUEFI` 2.0.3,
QEMU 11.0.50 et OVMF. Résultat : **Réussi / Passed**.

- bandeau GS visible ;
- mémoire verte RGB `64,255,64` ;
- horloge cyan animée, positions `11` puis `25` ;
- touche `a` injectée par QMP et témoin clavier orange à la position `30` ;
- durée du test : 5,537 s.

Empreintes de la chaîne validée :

```text
120a9f1524ef65e1a349178788f74f130292dc8c57608f4726f59db8dbed156d  BOOTX64.EFI
8677b16aeca88ab01c8209a4112521a60ad3a206631f1bc3094b50577ee79fe2  Noyau.GsE
1f699c99992bdd75bf603df53264e374d11ef3377242541d5dcb0de83b334aea  Sanctuaire-ESP.img
```

Le rapport machine se trouve dans
`Construction/Validation-GSPP-0221-QEMU-FINAL/Rapport-UEFI.txt`.

## Conclusion

La migration est complète et cohérente dans le compilateur, les formats, les
bibliothèques, les projets, les tests et Sanctuaire SE. Aucune compatibilité
avec les extensions ou signatures locales antérieures n’est nécessaire : les
artefacts doivent être reconstruits.
