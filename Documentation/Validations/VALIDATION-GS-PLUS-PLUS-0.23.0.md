# Validation du compilateur Gs++ 0.23.0

**VALIDÉ — 16 août 2026.**

## Décision couverte

Cette version prend en charge le cycle de vie des tableaux fixes d’objets
classes :

- champs tableaux et tableaux multidimensionnels ;
- champs omis ou explicitement listés sous la forme `Champ()` ;
- tableaux locaux ;
- construction par défaut dans l’ordre des indices ;
- destruction dans l’ordre inverse ;
- diagnostics bilingues lorsque des arguments ou un agrégat par élément sont
  demandés.

Les scénarios monolithique et séparé construisent les valeurs `1,2,3,4`,
vérifient la trace de destruction `4321` et renvoient `10`.

## Invariants binaires vérifiés

| Format | Signature | En-tête | Version | ABI |
| --- | --- | ---: | ---: | ---: |
| GsObj | `GSOBJ:0` + un zéro | 112 octets | 1.0 | 1 |
| GsA | `GSA:0` + trois zéros | 32 octets | 1.0 | 1 |
| GsE | `GSE:0` + trois zéros | 112 octets | 1.0 | 1 |

La signature de liaison reste `GsAbi:x64-ms-v1`. Aucun champ d’en-tête ni
aucune règle de disposition mémoire n’a changé.

## Validation Windows/MSVC

Configuration et construction propres avec MSVC 19.44.35228.0 :

```powershell
cmake --preset release
cmake --build --preset release --target espace_travail --parallel
ctest --preset release --output-on-failure
```

Les artefacts validés sont conservés dans
`Construction/VisualStudio/Release`.

Résultat : **4/4 tests réussis** :

- tests unitaires Gs++ 0.23.0 ;
- auto-hébergement partiel ;
- vérification du noyau Sanctuaire ;
- exécution hébergée du noyau.

## Validation GNU/WSL

Configuration et construction propres avec GNU C++ 11.4.0 :

```bash
cmake --preset release-ninja
cmake --build --preset release-ninja --target espace_travail --parallel
ctest --preset release-ninja --output-on-failure
```

Les artefacts validés sont conservés dans `Construction/Ninja/Release`.

Résultat : **5/5 tests réussis** :

- tests unitaires Gs++ ;
- auto-hébergement partiel ;
- intégration Gs++ monolithique et séparée, incluant le retour `10` du jalon
  0.23 ;
- intégration du noyau Sanctuaire ;
- construction du chargeur UEFI et de l’image FAT32 de 64 Mio.

La compilation GNU finale ne produit aucun avertissement lié au nouveau plan
de construction.

## Démarrage UEFI réel virtualisé

L’image GNU reconstruite a été lancée avec `Tester-SanctuaireUEFI` 2.0.3,
QEMU 11.0.50 et OVMF. Résultat : **Réussi / Passed**.

- bandeau GS visible ;
- mémoire verte RGB `64,255,64` ;
- horloge cyan animée, positions `3` puis `19` ;
- touche `a` injectée par QMP et témoin clavier orange à la position `30` ;
- durée du test : `6,528 s`.

Empreintes de la chaîne validée :

```text
120a9f1524ef65e1a349178788f74f130292dc8c57608f4726f59db8dbed156d  BOOTX64.EFI
8677b16aeca88ab01c8209a4112521a60ad3a206631f1bc3094b50577ee79fe2  Noyau.GsE
1f699c99992bdd75bf603df53264e374d11ef3377242541d5dcb0de83b334aea  Sanctuaire-ESP.img
```

Le rapport machine se trouve dans
`Construction/Rapports/QEMU/GsPlusPlus-0.23.0/Rapport-UEFI.txt`.

## Versions d’outils vérifiées

```text
Gs++ Compiler 0.23.0
Chargeur GsE 0.23.0
```

## Conclusion

Gs++ 0.23.0 est validé sous les deux chaînes de compilation et sur le démarrage
UEFI réel virtualisé. Le jalon étend la sémantique objet sans changer
`GSOBJ:0`, `GSA:0`, `GSE:0`, les formats 1.0 ni l’ABI 1.
