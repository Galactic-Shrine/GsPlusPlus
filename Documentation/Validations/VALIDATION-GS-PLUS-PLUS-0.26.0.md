# Validation du compilateur Gs++ 0.26.0

**VALIDÉ — 23 août 2026.**

## Résultat

Gs++ 0.26.0 livre la bibliothèque hébergée propriétaire nécessaire au futur
frontend auto-hébergé. Le jalon réussit les constructions et tests MSVC et
GNU/WSL, la conformité portable 18/18, les benchmarks smoke et le démarrage
réel virtualisé QEMU/OVMF de Sanctuaire SE.

Les formats natifs restent volontairement inchangés : GsObj, GsA et GsE sont
en version 1.0, leurs champs ABI valent 1 et les signatures inter-unités
commencent par `GsAbi:x64-ms-v1`.

## Périmètre livré

- chaîne UTF-8 propriétaire terminée par zéro, validation stricte, réserve,
  affectation et ajout transactionnels ;
- vecteurs dynamiques d’octets et de naturels ;
- table de symboles dynamique à sondage linéaire et croissance déterministe ;
- arène par blocs dont les adresses restent stables jusqu’à destruction ;
- jointure de chemins UTF-8, vues de nom et d’extension ;
- chargement de fichier alloué en deux requêtes et écriture à résultat
  explicite ;
- initialisation et destruction explicites de chaque propriétaire ;
- codes d’erreur sans exception ;
- conservation des qualificatifs lors d’une conversion explicite de pointeur.

Le contrat complet est
[`../BIBLIOTHEQUE_HEBERGEE_GS_PLUS_PLUS_0.26.md`](../BIBLIOTHEQUE_HEBERGEE_GS_PLUS_PLUS_0.26.md).

## Frontière des profils

`GsHebergee.GsA` dépend exactement de cinq imports :

```text
Gs::Hote::AllouerMemoire
Gs::Hote::LibererMemoire
Gs::Hote::LireFichier
Gs::Hote::EcrireFichier
Gs::Hote::EmettreDiagnostic
```

`GsSysteme.GsA` ne contient aucun symbole `Gs::Hote`. Cette séparation est
vérifiée automatiquement par `CONF-HOST-001` et `CONF-HOST-002`.

L’archive hébergée reconstruite sous MSVC et GNU possède la même empreinte :

```text
61ba47a416f3c3f7b2a5d26dd638813035f057f747695ac50ff0281f702d5084
```

## Exécution hébergée

Le GsE de test retourne `260`. Il couvre :

- UTF-8 valide accentué et séquence surlongue refusée ;
- terminaison nulle et auto-ajout d’une chaîne ;
- échec forcé d’une allocation de 4097 octets sans perte du contenu antérieur ;
- croissance et lecture des deux vecteurs ;
- réallocation, mise à jour et recherche de neuf symboles ;
- stabilité d’un pointeur après création d’un second bloc d’arène ;
- jointure `racine/source.GsPP`, nom `source.GsPP` et extension `.GsPP` ;
- lecture allouée en deux appels, écriture et libération ;
- compte équilibré des allocations/libérations et absence de libération
  invalide.

## Matrice MSVC

Construction : `Construction/VisualStudio/Release`.

```text
5/5 tests réussis
gspp_tests
gspp_autohebergement_tests
gspp_conformite
sanctuaire_noyau_verification
sanctuaire_noyau_execution
```

Le rapport de conformité est :

```text
Construction/VisualStudio/Release/Tests/GsPlusPlus/Conformite/rapport.json
```

Résultat : **18/18 réussis, 0 échec**.

## Matrice GNU/WSL

Construction : `Construction/Ninja/Release`, GNU C++ 11.4.0.

```text
6/6 tests réussis
gspp_tests
gspp_autohebergement_tests
gspp_conformite
gspp_integration
sanctuaire_noyau_integration
sanctuaire_uefi_integration
```

Le rapport de conformité est :

```text
Construction/Ninja/Release/Tests/GsPlusPlus/Conformite/rapport.json
```

Résultat : **18/18 réussis, 0 échec**.

Le script d’intégration a été durci : les recherches dans les binaires utilisent
`grep -a` directement. L’ancien pipeline `strings | grep -q` pouvait produire
un faux échec 141 sous `pipefail` quand le GsE hébergé devenait assez grand.

## Benchmarks smoke

Les quatre scénarios froids réussissent sur chaque chaîne :

| Chaîne | Session | Échantillons validés |
| --- | --- | ---: |
| MSVC | `20260823T090835.111749Z-b11abaf8` | 4/4 |
| GNU/WSL | `20260823T090905.501260Z-2ec70931` | 4/4 |

Les deux `status.json` annoncent `passed`.

## Reconstruction Sanctuaire SE et QEMU/OVMF

La dépendance de construction de l’image ESP référence désormais directement
`Noyau.GsE`. Une modification du noyau force donc réellement la régénération
de l’ESP, en plus de l’ordre imposé par la cible CMake.

Empreintes de la preuve GNU fraîche :

| Artefact | SHA-256 |
| --- | --- |
| `Noyau.GsE` | `8677b16aeca88ab01c8209a4112521a60ad3a206631f1bc3094b50577ee79fe2` |
| `BOOTX64.EFI` | `120a9f1524ef65e1a349178788f74f130292dc8c57608f4726f59db8dbed156d` |
| `Sanctuaire-ESP.img` | `1f699c99992bdd75bf603df53264e374d11ef3377242541d5dcb0de83b334aea` |

`Tester-SanctuaireUEFI` 2.0.3 a exécuté l’image avec QEMU 11.0.50 et OVMF.
Le rapport annonce **Réussi / Passed** en 5,324 secondes et confirme :

- bandeau GS ;
- mémoire verte RGB `64,255,64` ;
- horloge cyan animée aux positions 11 et 23 ;
- touche `a` injectée et clavier orange à la position 30.

Le rapport machine est :

```text
Construction/Rapports/QEMU/GsPlusPlus-0.26.0/Rapport-UEFI.json
```

Le test direct signale l’absence de `SHA256SUMS.txt` dans un paquet, car il a
reçu l’image ESP fraîche plutôt qu’une archive de distribution. L’empreinte de
l’image est néanmoins calculée, enregistrée dans le rapport et comparée aux
artefacts de cette validation.

## Versions publiées par les outils

```text
Gs++ Compiler 0.26.0
Chargeur GsE 0.26.0
```

## Conclusion

Gs++ 0.26.0 est validé sous MSVC, GNU/WSL et QEMU/OVMF. La bibliothèque
hébergée est suffisante pour commencer le jalon 0.27 consacré à la migration
progressive du frontend, sans développement fonctionnel actif de Sanctuaire SE
ou de Gs#.
