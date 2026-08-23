# Validation du compilateur Gs++ 0.25.0

**VALIDÉ — 16 août 2026.**

## Décision couverte

Gs++ 0.25.0 finalise le contrat d’initialisation et de durée de vie retenu pour
le périmètre candidat 1.0. Le jalon couvre :

- les valeurs par défaut des champs de classes ;
- le remplacement d’une valeur par défaut par la liste du constructeur ;
- la délégation `soi(arguments)`/`this(arguments)` sans cycle ;
- les arguments uniformes des tableaux locaux et des tableaux de champs objets
  classes ;
- la réévaluation des arguments dans l’ordre des indices et la destruction
  inverse ;
- l’exclusion normative des objets de classe globaux sans runtime caché ;
- l’extension de la conformité portable de 13 à 16 exigences ;
- le maintien de Sanctuaire SE 0.10.2 comme barrière d’intégration UEFI.

Le contrat détaillé se trouve dans
[`../INITIALISATION_DUREE_VIE_GS_PLUS_PLUS_0.25.md`](../INITIALISATION_DUREE_VIE_GS_PLUS_PLUS_0.25.md).

## Contrats binaires vérifiés

| Format | Signature | En-tête | Version | ABI |
| --- | --- | ---: | ---: | ---: |
| GsObj | `GSOBJ:0` + un zéro | 112 octets | 1.0 | 1 |
| GsA | `GSA:0` + trois zéros | 32 octets | 1.0 | 1 |
| GsE | `GSE:0` + trois zéros | 112 octets | 1.0 | 1 |

La signature de liaison reste `GsAbi:x64-ms-v1`. Aucun champ d’en-tête, numéro
de format, numéro d’ABI ou règle de disposition mémoire n’a changé.

## Preuves du langage 0.25

Le scénario `InitialisationDureeVie.GsPP` et son équivalent en compilation
séparée vérifient à l’exécution :

- `Base = 7` et `Codes[3] = {1, 2}` avec mise à zéro du dernier élément ;
- délégation de `constructeur(entier32)` vers `constructeur()` puis affectation
  du champ dans le corps délégant ;
- remplacement de `Valeur = 5` par `Valeur(valeur)` ;
- appel de `ProchaineValeur()` pour chacun des trois éléments d’un champ puis
  d’un tableau local ;
- trace de construction `123`, trace de destruction `321` et retour final
  `25` dans les deux modes de compilation.

Les tests négatifs couvrent notamment les cycles de délégation, la délégation
directe, les listes mélangées, les valeurs par défaut hors classe, les champs
objets classes initialisés avec `=`, l’absence de constructeur explicite et les
objets de classe globaux.

## Suite de conformité

La suite `gspp_conformite` a produit un rapport JSON dans chacune des deux
constructions permanentes :

```text
Construction/VisualStudio/Release/Tests/GsPlusPlus/Conformite/rapport.json
Construction/Ninja/Release/Tests/GsPlusPlus/Conformite/rapport.json
```

Résultat dans les deux cas : **16/16 réussis, 0 échec**.

Les trois nouvelles exigences sont :

- `CONF-LIFE-001` : exécution du scénario de durée de vie avec retour `25` ;
- `CONF-NEG-005` : refus d’un objet de classe global ;
- `CONF-NEG-006` : refus d’un cycle de délégation entre constructeurs.

Le GsE canonique minimal possède la même empreinte SHA-256 sous MSVC et GNU :

```text
82671ee387f2441504f2e09bd9cfa01cf1a41e5c2f2e556a605cec33bf74f141
```

Le scénario de durée de vie possède lui aussi la même empreinte GsE sur les
deux chaînes :

```text
7136a00beffa473da16d1bc6cef1670d4377231124edffe25fa01fc41c9ca5a6
```

Les GsObj incluent volontairement le chemin source explicite ; leurs
empreintes peuvent différer entre les chemins Windows et WSL tout en restant
reproductibles à deux exécutions identiques sur chaque hôte.

## Validation Windows/MSVC

Construction permanente dans `Construction/VisualStudio/Release` avec MSVC
19.44.35228.0 :

```powershell
cmake --build --preset release --target tests_gspp_preparation --parallel
ctest --preset release --output-on-failure
```

Résultat : **5/5 tests réussis**.

- tests unitaires Gs++ 0.25.0 ;
- auto-hébergement partiel ;
- conformité 16/16 ;
- vérification du noyau Sanctuaire ;
- exécution hébergée du noyau.

## Validation GNU/WSL

Construction permanente dans `Construction/Ninja/Release` avec GNU C++
11.4.0 :

```bash
cmake --build --preset release-ninja --target tests_gspp_preparation --parallel
ctest --preset release-ninja --output-on-failure
```

Résultat : **6/6 tests réussis**.

- tests unitaires Gs++ 0.25.0 ;
- auto-hébergement partiel ;
- conformité 16/16 ;
- intégration Gs++ monolithique et séparée ;
- intégration du noyau Sanctuaire ;
- construction du chargeur UEFI et de l’image FAT32 de 64 Mio.

## Banc de mesure

Le mode `smoke` 0.25.0 a validé les quatre scénarios sous les deux chaînes :

```text
Construction/Benchmarks/GsPlusPlus/20260816T154219.377364Z-fd40e9fd
Construction/Benchmarks/GsPlusPlus/20260816T154219.121204Z-f6d396c2
```

Chaque session annonce `status: passed` et quatre échantillons fonctionnels.
Ces essais prouvent le fonctionnement du banc ; ils ne constituent pas une
revendication de performance comparative.

## Démarrage UEFI réel virtualisé

L’image GNU fraîche a été lancée avec `Tester-SanctuaireUEFI` 2.0.3,
QEMU 11.0.50 et OVMF. Résultat : **Réussi / Passed**.

- bandeau GS visible ;
- mémoire verte RGB `64,255,64` ;
- horloge cyan animée, positions `15` puis `30` ;
- touche `a` injectée par QMP et témoin clavier orange à la position `30` ;
- durée du test : `5,699 s`.

Empreintes de la chaîne validée :

```text
120a9f1524ef65e1a349178788f74f130292dc8c57608f4726f59db8dbed156d  BOOTX64.EFI
8677b16aeca88ab01c8209a4112521a60ad3a206631f1bc3094b50577ee79fe2  Noyau.GsE
1f699c99992bdd75bf603df53264e374d11ef3377242541d5dcb0de83b334aea  Sanctuaire-ESP.img
```

Le rapport machine se trouve dans :

```text
Construction/Rapports/QEMU/GsPlusPlus-0.25.0/Rapport-UEFI.json
```

Ces empreintes sont identiques à celles de Gs++ 0.24.0. Cette stabilité est
attendue : le jalon 0.25 complète l’initialisation du langage sans modifier les
formats natifs, le chargeur UEFI ou les sources du noyau de référence.

## Versions d’outils vérifiées

```text
Gs++ Compiler 0.25.0
Chargeur GsE 0.25.0
```

## Conclusion

Gs++ 0.25.0 est validé sous MSVC, GNU/WSL et QEMU/OVMF. Le contrat
d’initialisation et de durée de vie retenu pour 1.0 est désormais implémenté,
documenté et couvert en compilation monolithique, séparée et négative. Le
prochain jalon actif est Gs++ 0.26, consacré à la bibliothèque hébergée ;
Sanctuaire SE et Gs# restent gelés fonctionnellement.
