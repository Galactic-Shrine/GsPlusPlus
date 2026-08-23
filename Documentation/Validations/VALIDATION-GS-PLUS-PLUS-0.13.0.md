# Validation du compilateur Gs++ 0.13.0

> Document historique : les signatures et numéros de formats indiqués décrivent
> ce jalon local. Gs++ 0.17.1 fixe `GSOBJ:0`, GsObj/GsA/GsE 1.0 et ABI 1.

Date : 22 juillet 2026.

## Résultat

Le jalon 0.13.0 est validé côté hôte. La compilation séparée ne fusionne plus
les AST de toutes les unités : les sources sont transformées en objets `GsO`,
les bibliothèques `GsA` sont lues séparément et l’éditeur de liens produit le
`CodeMachine` final après contrôle de l’ABI.

La référence fonctionnelle de Sanctuaire SE reste la version 0.10.2 déjà
validée sous QEMU/OVMF. Elle est reconstruite sans nouvelle fonctionnalité
système afin de détecter toute régression du compilateur ou du format GsE.

## Chaîne validée

```text
Calculs.GsPH + Calculs.GsPP
    → Calculs.GsO
    → Calculs.GsA

Calculs.GsPlusPlusHeader + Principal.GsPP
    → Principal.GsO

Principal.GsO + Calculs.GsA
    → vérification ABI
    → Application.GsE
    → chargement et exécution native
    → code de retour 44
```

Le même ensemble est aussi construit à partir d’une solution `.GsPs` composée
d’un projet bibliothèque `.GsPj` et d’un projet application `.GsProject`.

## Contrôles automatisés

La commande `make test` valide :

- lexeur, analyseur syntaxique, analyseur sémantique et backend existants ;
- types système 0.12 et alias applicatifs 0.11 ;
- sérialisation puis relecture d’un objet `GsO 1.0` ;
- rejet d’une version GsO inconnue et d’une référence de chaîne falsifiée ;
- sérialisation puis relecture d’une bibliothèque `GsA 1.0` ;
- rejet d’une taille totale GsA falsifiée ;
- extraction du membre requis d’une bibliothèque ;
- absence d’extraction d’un membre inutilisé possédant un import externe ;
- résolution d’une fonction et d’une globale entre deux unités ;
- isolation de deux symboles privés homonymes ;
- rejet d’un symbole public défini plusieurs fois par l’éditeur ;
- rejet d’une incompatibilité de type de paramètre entre interface et
  implémentation ;
- rejet d’une disposition de structure différente derrière un pointeur ;
- positions des deux déclarations présentes dans le diagnostic ABI ;
- production d’une carte avec source et signature ;
- extensions `.GsPH`, `.GsPlusPlusHeader`, `.GsPj`, `.GsProject` et `.GsPs` ;
- reproductibilité bit à bit des GsO, GsA, GsE et cartes de liens ;
- exécution hébergée du programme séparé avec code de retour `44` ;
- exécution du programme de types système avec code de retour `120` ;
- exécution hébergée du noyau Sanctuaire avec code de retour `5` ;
- vérification structurelle des GsE, du PE32+ UEFI et de l’image FAT32 ;
- reproductibilité du chargeur UEFI et de l’image ESP de 64 Mio.

## Analyseurs mémoire

Le compilateur, le lecteur GsO, le lecteur GsA, l’éditeur de liens, le
constructeur de solutions et le chargeur hébergé ont été reconstruits avec :

```text
-fsanitize=address,undefined -fno-omit-frame-pointer
```

Les tests unitaires instrumentés, la construction de la solution et
l’exécution du GsE lié passent sans erreur AddressSanitizer ni
UndefinedBehaviorSanitizer. LeakSanitizer a été désactivé pour cette exécution,
car l’environnement lance les processus sous traçage et LeakSanitizer s’y
arrête avant les tests.

## Mesures du scénario séparé

| Élément | Résultat |
|---|---:|
| `Calculs.GsO` | 112 octets de code, 4 octets de données, 2 symboles, 2 relocalisations |
| `Principal.GsO` | 68 octets de code, 3 symboles, 2 relocalisations |
| `Calculs.GsA` de test | 2 membres, dont 1 volontairement inutilisé |
| GsE lié | 3 symboles, 4 relocalisations, 0 import restant |
| Exécution séparée | code de retour `44` |
| Incompatibilité volontaire | refusée avec les deux positions source |

## Régression Sanctuaire SE

La construction conserve :

- `Noyau.GsE` au format GsE 2.0 ;
- 13 517 octets de code, 88 octets de données et 104 octets de zone zéro ;
- 111 symboles et 239 relocalisations avant résolution interne ;
- le chargeur PE32+ sans import de DLL ;
- GDT, IDT, TSS/IST, pagination W^X, APIC, PIT et clavier dans le contrôle
  statique ;
- l’image ESP FAT32 déterministe de 67 108 864 octets.

QEMU, OVMF et CMake ne sont pas installés dans l’environnement de construction
utilisé pour ce rapport. Aucun nouveau démarrage UEFI réel n’est donc revendiqué
pour le paquet compilateur 0.13.0. La validation UEFI réelle déjà acquise reste
celle de Sanctuaire SE 0.10.2 ; le noyau et son format exécutable sont gelés.

## Limite restante avant Sanctuaire SE 0.11

La compilation séparée et son ABI sont désormais stabilisées par les tests.
Le dernier verrou de langage inscrit dans la feuille de route avant la reprise
de l’ordonnanceur est la prise en charge des pointeurs de fonction.
