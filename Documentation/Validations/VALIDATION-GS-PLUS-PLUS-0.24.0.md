# Validation du compilateur Gs++ 0.24.0

**VALIDÉ — 16 août 2026.**

## Décision couverte

Gs++ 0.24.0 transforme le socle fonctionnel 0.23 en premier jalon de
convergence vers un produit Gs++ 1.0 réellement exploitable. Le jalon couvre :

- la spécification consolidée du langage candidat 1.0 ;
- les profils freestanding et hébergé ;
- la spécification complète du format GsObj 1.0 ;
- la consolidation des contrats GsA 1.0, GsE 1.0 et ABI 1 ;
- un manifeste de conformité portable ;
- treize preuves automatiques exécutées sous MSVC et GNU ;
- l’alignement du banc de mesure sur la version 0.24.0 ;
- le maintien de Sanctuaire SE 0.10.2 comme barrière d’intégration UEFI.

Cette version ne prétend pas terminer Gs++ 1.0. Les jalons 0.25 à 0.29 restent
nécessaires pour la durée de vie finale, la bibliothèque hébergée,
l’auto-hébergement complet, le durcissement et la distribution.

## Contrats binaires vérifiés

| Format | Signature | En-tête | Version | ABI |
| --- | --- | ---: | ---: | ---: |
| GsObj | `GSOBJ:0` + un zéro | 112 octets | 1.0 | 1 |
| GsA | `GSA:0` + trois zéros | 32 octets | 1.0 | 1 |
| GsE | `GSE:0` + trois zéros | 112 octets | 1.0 | 1 |

La signature de liaison reste `GsAbi:x64-ms-v1`. Aucun champ d’en-tête, numéro
de format, numéro d’ABI ou règle de disposition mémoire n’a changé.

## Suite de conformité

La suite `gspp_conformite` a produit un rapport JSON dans chacune des deux
constructions permanentes :

```text
Construction/VisualStudio/Release/Tests/GsPlusPlus/Conformite/rapport.json
Construction/Ninja/Release/Tests/GsPlusPlus/Conformite/rapport.json
```

Résultat dans les deux cas : **13/13 réussis, 0 échec**.

- trois extensions source acceptées ;
- trois extensions d’interface acceptées ;
- en-têtes GsObj, GsA et GsE 1.0 vérifiés octet par octet ;
- préfixe ABI vérifié ;
- programmes français et anglais exécutés avec le retour `24` ;
- égalité binaire locale de deux productions GsObj, GsA et GsE ;
- refus de `.GsO`, `.GsPPH`, `.Gs#` et d’un GsE tronqué.

Le GsE canonique minimal possède la même empreinte SHA-256 sous MSVC et GNU :

```text
cf042ec871ca46c4648871e36e46b830530661465248ce5845eb49eb223ffa90
```

Les GsObj incluent volontairement le chemin source explicite ; leurs empreintes
diffèrent donc entre les chemins Windows et WSL, tout en restant
reproductibles à deux exécutions identiques sur chaque hôte.

## Validation Windows/MSVC

Construction dans `Construction/VisualStudio/Release` avec MSVC
19.44.35228.0 :

```powershell
cmake --preset release
cmake --build --preset release --target tests_gspp_preparation --parallel
ctest --preset release --output-on-failure
```

Résultat : **5/5 tests réussis**.

- tests unitaires Gs++ 0.24.0 ;
- auto-hébergement partiel ;
- conformité 13/13 ;
- vérification du noyau Sanctuaire ;
- exécution hébergée du noyau.

## Validation GNU/WSL

Construction dans `Construction/Ninja/Release` avec GNU C++ 11.4.0 :

```bash
cmake --preset release-ninja
cmake --build --preset release-ninja --target tests_gspp_preparation --parallel
ctest --preset release-ninja --output-on-failure
```

Résultat : **6/6 tests réussis**.

- tests unitaires Gs++ 0.24.0 ;
- auto-hébergement partiel ;
- conformité 13/13 ;
- intégration Gs++ monolithique et séparée ;
- intégration du noyau Sanctuaire ;
- construction du chargeur UEFI et de l’image FAT32 de 64 Mio.

## Banc de mesure

Le mode `smoke` 0.24.0 a validé les quatre scénarios sous les deux chaînes :

```text
Construction/Benchmarks/GsPlusPlus/20260816T151424.654721Z-419be9f9
Construction/Benchmarks/GsPlusPlus/20260816T151423.940040Z-4a5132c3
```

Chaque session annonce `status: passed` et quatre échantillons fonctionnels.
Ces essais prouvent le fonctionnement du banc ; ils ne constituent pas une
revendication de performance comparative.

## Démarrage UEFI réel virtualisé

L’image GNU fraîche a été lancée avec `Tester-SanctuaireUEFI` 2.0.3,
QEMU 11.0.50 et OVMF. Résultat : **Réussi / Passed**.

- bandeau GS visible ;
- mémoire verte RGB `64,255,64` ;
- horloge cyan animée, positions `10` puis `24` ;
- touche `a` injectée par QMP et témoin clavier orange à la position `30` ;
- durée du test : `5,613 s`.

Empreintes de la chaîne validée :

```text
120a9f1524ef65e1a349178788f74f130292dc8c57608f4726f59db8dbed156d  BOOTX64.EFI
8677b16aeca88ab01c8209a4112521a60ad3a206631f1bc3094b50577ee79fe2  Noyau.GsE
1f699c99992bdd75bf603df53264e374d11ef3377242541d5dcb0de83b334aea  Sanctuaire-ESP.img
```

Le rapport machine se trouve dans :

```text
Construction/Rapports/QEMU/GsPlusPlus-0.24.0/Rapport-UEFI.json
```

L’empreinte ESP est identique à celle de Gs++ 0.23.0. Cette stabilité est
attendue : le jalon 0.24 formalise et contrôle les contrats existants sans
modifier la génération native du noyau ou du chargeur.

## Versions d’outils vérifiées

```text
Gs++ Compiler 0.24.0
Chargeur GsE 0.24.0
```

## Conclusion

Gs++ 0.24.0 est validé sous MSVC, GNU/WSL et QEMU/OVMF. Le contrat candidat
1.0, les formats natifs 1.0 et l’ABI 1 disposent désormais d’une preuve
portable et d’un rapport machine par construction. Le prochain jalon actif est
Gs++ 0.25 ; Sanctuaire SE et Gs# restent gelés fonctionnellement.
