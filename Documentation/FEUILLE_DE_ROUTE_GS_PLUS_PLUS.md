# Feuille de route prioritaire de Gs++

## Principe de versionnement

Le compilateur Gs++ possède désormais un cycle de versions indépendant de
Sanctuaire SE. La version 0.10.2 de Sanctuaire SE reste la référence UEFI
validée et sert de test d’intégration réel. Aucun nouvel ordonnanceur, processus
ou pilote n’est ajouté au système tant que le cœur du langage nécessaire à ces
composants n’est pas stabilisé.

## Jalons

### Gs++ 0.11 — alias applicatifs — terminé

- véritables alias de fonctions et de globales ;
- alias de structures et de champs ;
- chaînes d’alias et noms qualifiés ;
- exports COFF/GsE partageant une seule adresse ;
- suppression des fonctions-ponts du noyau de référence ;
- diagnostics de cycles, conflits et cibles absentes.

### Gs++ 0.12 — types système — terminé

- entiers signés et non signés de 8, 16, 32 et 64 bits ;
- booléen distinct ;
- caractères et octets ;
- constantes, `const` et `volatile` ;
- tableaux de taille fixe ;
- énumérations et unions ;
- conversions explicites vérifiées ;
- règles ABI x86-64 documentées pour chaque type.

### Gs++ 0.13 — compilation séparée — terminé

- interfaces historiques `.GsPPH` et `.GsPlusPlusHeader`, remplacées en
  0.22.1 par `.HGs++`, `.HGsPP` et `.HeaderGsPlusPlus` ;
- format objet et édition de liens multi-unités stabilisés ;
- projets `.GsPj`/`.GsProject` et solutions `.GsPs` ;
- bibliothèques statiques natives ;
- symboles de diagnostic et informations de source ;
- vérification de compatibilité d’ABI entre unités.

### Gs++ 0.14 — pointeurs de fonction — terminé

- signatures de callbacks typées et bilingues ;
- prise d’adresse, stockage, passage et retour de fonctions ;
- appels indirects selon l’ABI Microsoft x64 ;
- signatures ABI récursives dans GsObj et contrôle inter-unités ;
- callbacks globaux relocalisés dans les chargeurs hébergé et UEFI.

### Gs++ 0.15 — valeurs structurées — terminé

- copies et affectations de structures et d’unions ;
- initialisations agrégées imbriquées et mise à zéro des éléments absents ;
- agrégats globaux avec relocalisations de fonctions ;
- structures passées et retournées par valeur ;
- callbacks acceptant et retournant des structures ;
- ABI canonique `x64-ms-v1` vérifiée entre objets GsObj ; elle conserve toutes
  les règles de passage structuré auparavant testées sous la numérotation locale v2.

### Gs++ 0.16 — bibliothèque système — terminé

- mémoire, texte et vues non propriétaires ;
- opérateurs et fonctions de bits 32/64 bits ;
- atomiques x86-64 et primitives de synchronisation ;
- bibliothèque noyau freestanding séparée de la future bibliothèque hébergée ;
- aucune allocation, exception, import ou initialisation cachée en mode système.

### Gs++ 0.17 — préparation à l’auto-hébergement — terminé

- littéraux chaîne UTF-8 terminés par zéro et protégés en écriture ;
- opérateurs logiques `&&` et `||` avec court-circuit réel ;
- bibliothèque `GsHebergee.GsA` pour flux, fichiers, diagnostics, vecteurs et
  tables de symboles à stockage explicite ;
- classificateur des mots-clés réécrit en Gs++ ;
- comparaison automatique de 79 entrées avec le lexeur C++ de référence,
  nouveaux mots-clés objet français et anglais compris.

### Gs++ 0.18 — modèle objet système — terminé

- classes et visibilité complète ;
- constructeurs et destructeurs ;
- références ;
- surcharge de fonctions et d’opérateurs ;
- RAII utilisable sans runtime obligatoire ;
- fonctions virtuelles optionnelles et disposition documentée.

La 0.18.0 livre ce périmètre freestanding avec les limites volontaires
documentées : pas encore d’héritage, de retours/champs références, de
constructeurs globaux ni de déroulement d’exceptions. Le contrat détaillé est
dans [`MODELE_OBJET_GS_PLUS_PLUS_0.18.md`](MODELE_OBJET_GS_PLUS_PLUS_0.18.md).

### Gs++ 0.19 — héritage simple et remplacement — terminé

- héritage simple public avec sous-objet de base au décalage zéro ;
- accès protégé depuis les méthodes des classes dérivées ;
- conversions implicites dérivée vers base par référence ou pointeur ;
- refus du slicing implicite par valeur ;
- `remplacer/override` obligatoire pour redéfinir un virtuel hérité ;
- conservation des emplacements virtuels hérités et extension déterministe de
  la table ;
- construction automatique des bases sans argument et destruction complète de
  la dérivée vers la racine ;
- contrôle de la hiérarchie et de la table virtuelle dans l’ABI inter-unités.

La 0.19.0 limite volontairement ce jalon à une seule base publique et à son
constructeur accessible sans argument explicite. L’héritage multiple, virtuel,
protégé/privé, les listes d’initialisation de bases et les conversions vers une
classe dérivée restent prévus. Le contrat détaillé se trouve dans
[`HERITAGE_GS_PLUS_PLUS_0.19.md`](HERITAGE_GS_PLUS_PLUS_0.19.md).

### Gs++ 0.20 — initialisation et appels parent — terminé

- `: parent(arguments)` en français et `: super(arguments)` en anglais pour
  initialiser la base directe depuis un constructeur dérivé ;
- résolution surchargée et contrôle d’accès du constructeur de base ;
- prologue de construction propre à chaque constructeur, sans double appel
  dans les chaînes comportant des classes sans constructeur déclaré ;
- `parent.Methode()`/`super.Method()` pour appeler directement une
  implémentation héritée, sans dispatch virtuel ;
- conservation du dispatch dynamique pour les appels ordinaires via une
  référence ou un pointeur de base ;
- scénarios monolithique et séparé exécutés avec retour `82` et trace de durée
  de vie `1,2,3,4` ;
- contrôle bilingue sur 83 classifications dans l’auto-hébergement partiel.

La 0.20.0 reste limitée à une seule base publique et à une seule entrée
d’initialisation de base. Les initialisateurs de champs, l’héritage
multiple/virtuel, les conversions descendantes, la RTTI et les virtuels purs
restent prévus. Le contrat détaillé se trouve dans
[`INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md`](INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md).

### Gs++ 0.21 — initialisateurs de champs — terminé

- liste `: parent(...), Champ(expression)` et forme anglaise `super` ;
- base obligatoirement première, puis champs directs uniques dans leur ordre de
  déclaration ;
- normalisation des alias de champs ;
- champs constants, scalaires, pointeurs, structures non classes, tableaux et
  agrégats ;
- mise à zéro conservée pour les champs omis ;
- génération avant le corps du constructeur ;
- scénarios monolithique et séparé exécutés avec trace `12345` puis `1234567`
  et retour `75`.

La 0.21.0 ne construit pas encore les champs objets de type classe, car leur
destruction récursive sur toutes les sorties doit être définie conjointement.
Les valeurs par défaut au point de déclaration, la délégation entre
constructeurs et l’héritage multiple/virtuel restent prévus. Le contrat
détaillé se trouve dans
[`INITIALISEURS_CHAMPS_GS_PLUS_PLUS_0.21.md`](INITIALISEURS_CHAMPS_GS_PLUS_PLUS_0.21.md).

### Gs++ 0.22 — champs objets classes — terminé

- résolution de `Champ(arguments)` pour les champs possédés par valeur ;
- construction automatique sans argument des champs classes omis ;
- traversée récursive des classes intermédiaires sans constructeur ;
- installation des tables virtuelles à l’adresse exacte des sous-objets ;
- destruction dans l’ordre destructeur courant, champs inversés, puis base ;
- réutilisation du RAII sur fins de blocs, branches, boucles et retours ;
- scénarios monolithique et séparé avec trace `1234`, puis `123495678`, et
  retour `91`.

La 0.22.0 laisse volontairement hors périmètre les tableaux de classes, les
constructeurs délégués, les valeurs par défaut au point de déclaration et les
exceptions. Le contrat détaillé se trouve dans
[`CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md`](CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md).

### Gs++ 0.23 — tableaux d’objets classes — terminé

- tableaux fixes de champs objets classes, y compris multidimensionnels ;
- tableaux locaux d’objets classes ;
- construction par défaut des éléments dans l’ordre des indices ;
- destruction des éléments dans l’ordre strictement inverse ;
- prise en charge d’un champ omis ou explicitement listé avec `Champ()` ;
- conservation des plans récursifs de base, champs et tables virtuelles pour
  chaque élément ;
- scénarios monolithique et séparé avec trace de destruction `4321` et retour
  `10`.

La 0.23.0 n’accepte pas encore des arguments ou agrégats distincts par
élément. Les constructeurs délégués, valeurs par défaut au point de
déclaration, constructeurs globaux, exceptions et formes d’héritage avancées
restent prévus. Le contrat détaillé se trouve dans
[`TABLEAUX_OBJETS_CLASSES_GS_PLUS_PLUS_0.23.md`](TABLEAUX_OBJETS_CLASSES_GS_PLUS_PLUS_0.23.md).

### Gs++ 0.24 — contrat et conformité — terminé

- périmètre candidat du langage Gs++ 1.0 consolidé ;
- contrats GsObj, GsA et GsE 1.0 et ABI 1 documentés ;
- profils freestanding et hébergé séparés explicitement ;
- manifeste de conformité portable avec treize exigences ;
- extensions, formats, ABI, bilinguisme, reproductibilité et refus essentiels
  vérifiés sous MSVC et GNU ;
- résultats 5/5 sous MSVC, 6/6 sous GNU/WSL, benchmarks smoke et démarrage
  QEMU/OVMF réussis.

La preuve se trouve dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.24.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.24.0.md).

### Gs++ 0.25 — initialisation et durée de vie — terminé

- valeurs par défaut des champs de classes, avec priorité à la liste explicite ;
- délégation `soi(arguments)`/`this(arguments)` exclusive et sans cycle ;
- arguments uniformes des tableaux locaux et champs objets, réévalués pour
  chaque élément ;
- construction `123` et destruction inverse `321` validées en monolithique et
  en compilation séparée ;
- objets de classe globaux exclus du contrat freestanding sans runtime caché ;
- conformité portée à seize exigences ;
- résultats 5/5 sous MSVC, 6/6 sous GNU/WSL, benchmarks smoke et démarrage
  QEMU/OVMF réussis.

Le contrat et la preuve se trouvent dans
[`INITIALISATION_DUREE_VIE_GS_PLUS_PLUS_0.25.md`](INITIALISATION_DUREE_VIE_GS_PLUS_PLUS_0.25.md)
et
[`Validations/VALIDATION-GS-PLUS-PLUS-0.25.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.25.0.md).

### Gs++ 0.26 — bibliothèque hébergée — terminé

- chaînes UTF-8 propriétaires avec validation stricte ;
- vecteurs dynamiques, table de symboles et arène à adresses stables ;
- chemins et fichiers alloués avec erreurs explicites sans exception ;
- exactement cinq imports dans `GsHebergee.GsA` et aucun import d’hôte dans
  `GsSysteme.GsA` ;
- conformité portée à dix-huit exigences ;
- résultats 5/5 sous MSVC, 6/6 sous GNU/WSL, benchmarks smoke et démarrage
  QEMU/OVMF réussis.

Le contrat et la preuve se trouvent dans
[`BIBLIOTHEQUE_HEBERGEE_GS_PLUS_PLUS_0.26.md`](BIBLIOTHEQUE_HEBERGEE_GS_PLUS_PLUS_0.26.md)
et
[`Validations/VALIDATION-GS-PLUS-PLUS-0.26.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.26.0.md).

## Convergence vers le produit Gs++ 1.0

Le socle nécessaire à Sanctuaire SE est techniquement atteint depuis Gs++
0.23.0. La décision produit du 16 août 2026 impose néanmoins de terminer Gs++
comme produit réellement exploitable avant de développer activement
Sanctuaire SE 0.11, Gs# ou les autres couches.

Les prochains jalons sont donc réservés à Gs++ :

1. 0.24 — contrat du langage et suite de conformité — terminé ;
2. 0.25 — initialisation et durée de vie finalisées — terminé ;
3. 0.26 — bibliothèque hébergée suffisante pour le compilateur — terminé ;
4. 0.27 — frontend auto-hébergé — actif, lexeur validé sous MSVC/GNU,
   analyseur syntaxique et AST à migrer ;
5. 0.28 — backend, formats et linker auto-hébergés ;
6. 0.29 — durcissement, reproductibilité et distribution ;
7. 1.0.0 — sortie produit après satisfaction de tous les critères.

Le périmètre détaillé, les invariants gelés et les critères de sortie se
trouvent dans
[`PLAN_PRODUIT_GS_PLUS_PLUS_1.0.md`](PLAN_PRODUIT_GS_PLUS_PLUS_1.0.md).

La preuve intermédiaire du lexeur 0.27 se trouve dans
[`FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md`](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md).

Sanctuaire SE reste sur la référence 0.10.2 et continue d’être reconstruit
comme preuve d’intégration réelle. Toute modification Gs++ susceptible
d’affecter le code natif doit encore produire `Noyau.GsE`, `BOOTX64.EFI` et
l’image ESP, puis réussir le test QEMU/OVMF. Aucun développement fonctionnel de
l’ordonnanceur, des processus ou du mode utilisateur n’est engagé avant Gs++
1.0.
