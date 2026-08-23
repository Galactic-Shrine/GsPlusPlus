# Compilateur Gs++ 0.17.1 — formats natifs canoniques et auto-hébergement

> Référence historique du jalon 0.17. La référence courante est
> [`REFERENCE-GS-PLUS-PLUS-0.22.md`](REFERENCE-GS-PLUS-PLUS-0.22.md) ; elle
> conserve `GSOBJ:0`, les formats 1.0 et l’ABI 1, puis ajoute le modèle objet
> et son héritage simple public avec initialisation explicite de la base.

Gs++ est le langage système natif de **Sanctuaire SE** (*Shrine OS*). Ce dépôt contient son compilateur expérimental x86-64, écrit en C++ jusqu’à ce que Gs++ puisse devenir auto-hébergé. À partir de cette version, le compilateur possède son propre cycle de versions : Sanctuaire SE 0.10.2 reste la référence UEFI stable utilisée comme test d’intégration.

Licence : **Mozilla Public License 2.0 (MPL-2.0)**.

Le français est la forme canonique du langage. Chaque mot-clé possède un alias anglais normalisé vers la même représentation interne : les deux écritures produisent donc le même code machine.

Le compilateur ne traduit pas le programme en C++ et ne dépend pas de .NET :

```text
source Gs++
  → lexeur UTF-8
  → jetons bilingues normalisés
  → analyse syntaxique et sémantique
  → instructions x86-64
  → objet natif GsObj ou objet COFF AMD64
  → bibliothèque statique GsA et édition de liens multi-unités
  → exécutable Sanctuaire GsE
```

## Fonctionnalités de la version 0.17.1

La version 0.17 compile et exécute le premier composant du compilateur écrit en
Gs++ : le classificateur de mots-clés. Son image GsE est comparée
automatiquement au lexeur C++ sur 63 mots français, alias anglais et
identifiants négatifs.

Le langage accepte désormais les littéraux chaîne UTF-8, terminés par zéro et
typés `constante caractère*`, ainsi que `&&` et `||` avec leurs priorités et un
vrai court-circuit dans le backend x86-64 comme dans l’évaluation constante.

La nouvelle bibliothèque `GsHebergee.GsA` fournit des vues texte, flux mémoire,
vecteurs et tables de symboles déterministes, puis un contrat minimal avec
l’hôte pour les fichiers et diagnostics. Comme `GsSysteme.GsA`, elle ne cache
aucune allocation : l’appelant fournit tout le stockage.

La version 0.16 a livré la première **bibliothèque système native Gs++** sous la
forme de `GsSysteme.GsA`. Elle fournit des primitives freestanding de mémoire,
des vues non propriétaires d’octets et de texte, des opérations de bits 32/64
bits, des opérations atomiques x86-64 et un verrou léger. Elle n’effectue
aucune allocation, ne lance aucune exception et ne dépend d’aucun runtime ou
import externe caché.

Le langage comprend désormais les opérateurs `~`, `&`, `^`, `|`, `<<` et `>>`
avec leurs priorités usuelles. Les décalages sont définis modulo la largeur de
l’opérande. Le backend intègre directement les primitives atomiques réservées
avec `xchg`, `lock xadd`, `lock cmpxchg`, `mfence` et `pause`; elles ne deviennent
donc jamais des imports GsE. Les prototypes d’interface qui ne sont pas
réellement utilisés ne créent plus non plus d’import obligatoire.

L’interface canonique est
`GsPlusPlus/Bibliotheques/Systeme/Systeme.HGsPP`. Les API françaises vivent dans
`Gs::Systeme` et leurs alias anglais sont accessibles dans `Gs::System`.

La version 0.15 a introduit les **valeurs structurées complètes**. Les structures
et unions peuvent être initialisées par listes imbriquées, copiées, affectées,
passées à une fonction et retournées par valeur. Les tableaux fixes acceptent
également les initialisations agrégées ; les éléments absents et les octets de
bourrage sont mis à zéro.

Une structure passée par valeur est copiée dans le cadre local de la fonction.
Une structure retournée est écrite dans une zone fournie par l’appelant, y
compris lors d’un appel indirect. Ce contrat est identifié par
`GsAbi:x64-ms-v1` et par l’ABI GsObj 1. Il s’agit de la renumérotation canonique
du contrat complet déjà validé pour les valeurs structurées ; aucune
fonctionnalité ABI n’a été retirée.

La version 0.14 introduit les **pointeurs de fonction typés**. Une fonction peut
être prise par adresse, stockée dans une locale, une globale, un paramètre, un
champ ou un tableau, retournée par une autre fonction puis appelée
indirectement. Le compilateur vérifie intégralement sa signature et l’inscrit
dans le contrat ABI des objets natifs.

La série 0.13 introduit une véritable **compilation séparée**. La révision
0.13.1 fixait historiquement `.GsPPH` pour les interfaces courtes et `.GsObj`
pour les objets natifs. Depuis 0.22.1, les interfaces utilisent `.HGs++`,
`.HGsPP` ou `.HeaderGsPlusPlus`. La révision 0.13.2 rend également le format
reconnaissable sans ambiguïté grâce à la signature binaire historique
`GSOBJ\0`. Gs++ 0.17.1 la remplace par la signature canonique `GSOBJ:0`. Une unité
Gs++ est compilée dans un objet natif versionné `GsObj 1.0`, plusieurs objets
peuvent être réunis dans une bibliothèque statique `GsA 1.0`, puis l’éditeur
de liens produit le GsE final sans refaire l’analyse des sources. Les membres
d’une bibliothèque ne sont extraits que lorsqu’ils définissent un symbole
réellement requis.

Les interfaces `.HGs++`, `.HGsPP` et `.HeaderGsPlusPlus` acceptent les structures,
énumérations, alias, prototypes de fonctions et déclarations de globales. Les
fonctions et globales d’une interface sont externes implicitement. Chaque objet
conserve une signature ABI canonique et sa position source ; une divergence de
type, de genre de symbole ou de disposition de structure est refusée lors de
la liaison avec les deux emplacements concernés.

Les projets `.GsPj`/`.GsProject` et solutions `.GsPs` construisent dans l’ordre
les objets, bibliothèques et exécutables. L’option `--carte` produit une carte
de liens contenant les adresses, sources et signatures ABI.

La version 0.13 conserve les **types système complets** introduits en 0.12 :
entiers signés et non signés de 8, 16, 32 et 64 bits, booléen distinct, octet,
caractère, tableaux fixes, énumérations, unions, qualificateurs
`constante`/`const` et `volatile`, ainsi que les conversions explicites
`convertir<T>`/`cast<T>`.

Les charges, stockages, paramètres, retours, comparaisons et divisions suivent
la largeur et la signedness du type.

Les véritables alias applicatifs de 0.11 restent disponibles. Une fonction, une
variable globale, une structure ou un champ peut recevoir un autre nom sans
dupliquer son code, son stockage ou sa disposition.

Elle utilise le format exécutable canonique **GsE 1.0**. Sa disposition
conserve les fonctionnalités de l’ancienne numérotation locale GsE 2.0,
introduite avec Sanctuaire SE 0.10.2. Les noms de
symboles ne sont plus placés dans des champs fixes : les imports et exports
référencent désormais une table de chaînes UTF-8. Un nom peut occuper jusqu’à
1 024 octets UTF-8. Les images portant les anciennes numérotations locales 1.1
ou 2.0 doivent être recompilées.

- fichiers sources `.GsPP` et `.GsPlusPlus` ;
- interfaces `.HGs++`, `.HGsPP` et `.HeaderGsPlusPlus` ;
- objets natifs `.GsObj` avec informations de source et contrats ABI ;
- bibliothèques statiques `.GsA` avec extraction à la demande ;
- édition de liens multi-unités et vérification ABI ;
- projets `.GsPj`/`.GsProject` et solutions `.GsPs` ;
- cartes de liens contenant adresses, symboles, sources et signatures ;
- ancien mode de compilation monolithique de plusieurs fichiers conservé ;
- identifiants UTF-8, notamment `Mémoire` et `Évaluer` ;
- mots-clés français natifs et alias anglais ;
- déclarations `alias NomAnglais = NomFrancais;` résolues entre fichiers ;
- alias de fonctions, variables globales, structures et champs ;
- chaînes d’alias, cibles qualifiées et alias publics utilisables comme point d’entrée ;
- un seul corps machine, stockage ou emplacement de champ pour tous les alias d’une déclaration ;
- espaces de noms, fonctions et symboles publics ;
- déclarations de fonctions externes avec `externe` ou `extern` ;
- déclarations de variables globales externes ;
- paramètres et arguments, jusqu’à quatre ;
- paramètres et retours pour tous les types scalaires et les pointeurs ;
- copies et affectations de structures ou d’unions ;
- initialisations agrégées imbriquées des structures, unions et tableaux fixes ;
- paramètres et retours de structures par valeur, y compris dans les callbacks ;
- initialisation agrégée des globales avec relocalisations de fonctions imbriquées ;
- pointeurs de fonction typés, paramètres callbacks, retours callbacks et appels indirects ;
- entiers `entier8`/`int8` à `entier64`/`int64` et `naturel8`/`uint8` à `naturel64`/`uint64` ;
- littéraux couvrant toute la plage signée et non signée de 64 bits ;
- `booléen`/`bool`, `octet`/`byte` et `caractère`/`char` distincts ;
- qualificateurs `constante`/`const` et `volatile` ;
- tableaux fixes multidimensionnels, énumérations portées et unions ;
- conversions scalaires explicites `convertir<T>`/`cast<T>` avec contrôle des constantes hors plage ;
- variables locales et globales ;
- données globales initialisées et zone globale initialisée à zéro ;
- structures, unions, pointeurs, `&`, `*`, `.`, `->` et indexation des pointeurs ou tableaux ;
- expressions arithmétiques, comparaisons, conditions et boucles ;
- opérateurs binaires `~`, `&`, `^`, `|`, `<<` et `>>` sur les entiers ;
- opérateurs logiques `&&` et `||` avec court-circuit ;
- littéraux chaîne UTF-8 constants, échappés et terminés par zéro ;
- bibliothèque freestanding `GsSysteme.GsA` pour mémoire, vues, texte et bits ;
- bibliothèque hébergée `GsHebergee.GsA` pour flux, fichiers, diagnostics et
  conteneurs déterministes à stockage explicite ;
- premier composant du compilateur écrit en Gs++ et comparé au lexeur C++ ;
- atomiques x86-64 32/64 bits, barrière mémoire, pause processeur et verrou léger ;
- suppression des imports issus de prototypes d’interface inutilisés ;
- analyse sémantique et vérification statique des types ;
- génération directe de code machine x86-64 ;
- objets COFF avec sections `.text`, `.data` et `.bss` ;
- exécutables `.GsE` 2.0 avec segments séparés lecture/exécution et lecture/écriture ;
- sections GsE de code, données, zéro, imports, exports, chaînes, relocalisations et métadonnées ;
- métadonnées d’application en syntaxe GsC ;
- vérificateur autonome `gseverifier` ;
- chargeur autonome `gsechargeur`, avec l’alias anglais `gseload` ;
- copie des segments GsE avec initialisation de `.zero` ;
- résolution des imports et application des relocalisations `REL32`/`Adresse64`/`BASE64` ;
- calcul des adresses d’exports et du point d’entrée après relogement ;
- protections mémoire lecture/écriture/exécution en mode hébergé ;
- exécution contrôlée d’un point d’entrée natif ou noyau ;
- ABI bilingue `ContexteDemarrage`/`BootContext` version 3 étendue de façon ascendante à 320 octets ;
- première application freestanding `BOOTX64.EFI` sans bibliothèque externe ;
- lecture de `Noyau.GsE` par le protocole de fichiers UEFI ;
- chargement du noyau, carte mémoire, GOP/framebuffer, ACPI et `ExitBootServices` ;
- normalisation des régions UEFI conventionnelles et récupération des régions `BootServices` après exclusion de la pile active et de l’image du chargeur ;
- allocateur physique Gs++ multipage et multiplage, avec libération, réutilisation, fusion des plages contiguës, statistiques et réserve 0.7 comme secours ;
- tas noyau Gs++ multi-page avec en-tête séparé des données, suivi des allocations, libération transactionnelle et refus des doubles libérations ;
- gestionnaire Gs++ de l’état de pagination, de sa racine, de sa couverture et des bornes de pages cartographiées ;
- tables de pages x86-64 propres au noyau, page nulle absente et cartographie identitaire jusqu’à 512 Gio ;
- activation de NX, `CR0.WP` et protections W^X réelles des sections EFI/GsE ;
- GDT x86-64 et IDT installées après `ExitBootServices` ;
- 32 stubs d’exceptions distincts avec étape, vecteur, code d’erreur, `RIP`, `CR2` et diagnostic framebuffer/série ;
- pile noyau dédiée de 64 Kio et pile IST de 32 Kio, chacune précédée d’une page garde inaccessible ;
- TSS x86-64 chargé avec `ltr`, `RSP0`, `IST1` pour la double faute et `IST2` pour les IRQ ;
- découverte ACPI du MADT, du Local APIC, des I/O APIC et des redirections ISA ;
- masquage du PIC historique et routage I/O APIC des vecteurs horloge 32 et clavier 33 ;
- horloge PIT à 100 Hz, clavier PS/2 et acquittement Local APIC ;
- stubs IRQ sauvegardant les 15 registres généraux, conservant leur cadre dans `R15` et validant `RIP/CS/RFLAGS` avant et après le répartiteur Gs++ ;
- table assembleur des adresses IRQ avec relocalisations PE `DIR64`, sans chargement ELF `GOTPCRELX` ambigu ;
- boucle inactive `sti; hlt` sur la pile noyau après l’initialisation ;
- première console framebuffer Gs++ avec primitives de pixel et de rectangle ;
- témoin vert de validation de l’allocateur physique, du tas et de la pagination ;
- image ESP FAT32 déterministe de 64 Mio ;
- première image noyau Gs++ multifichier ;
- diagnostics français ou anglais, avec fichier, ligne et colonne ;
- sorties déterministes sans horodatage.

Le format objet, les bibliothèques, les interfaces et les fichiers de projet
sont décrits dans
[`COMPILATION_SEPAREE_GS_PLUS_PLUS_0.13.md`](COMPILATION_SEPAREE_GS_PLUS_PLUS_0.13.md).
Les types et l’ABI scalaire restent documentés dans
[`TYPES_SYSTEME_GS_PLUS_PLUS_0.12.md`](TYPES_SYSTEME_GS_PLUS_PLUS_0.12.md).
La syntaxe, le typage et l’ABI des callbacks sont décrits dans
[`POINTEURS_FONCTION_GS_PLUS_PLUS_0.14.md`](POINTEURS_FONCTION_GS_PLUS_PLUS_0.14.md).
Les copies, agrégats et conventions d’appel des structures sont décrits dans
[`VALEURS_STRUCTUREES_GS_PLUS_PLUS_0.15.md`](VALEURS_STRUCTUREES_GS_PLUS_PLUS_0.15.md).
La bibliothèque freestanding, ses contrats et ses limites sont décrits dans
[`BIBLIOTHEQUE_SYSTEME_GS_PLUS_PLUS_0.16.md`](BIBLIOTHEQUE_SYSTEME_GS_PLUS_PLUS_0.16.md).
Les littéraux, la bibliothèque hébergée et le premier composant migré sont
décrits dans
[`AUTOHEBERGEMENT_GS_PLUS_PLUS_0.17.md`](AUTOHEBERGEMENT_GS_PLUS_PLUS_0.17.md).

Les programmes `GsPlusPlus/Tests/Integration/TypesSysteme.GsPP`, `GsPlusPlus/Tests/Integration/PointeursFonction.GsPP` et
`GsPlusPlus/Tests/Integration/ValeursStructures.GsPP` sont compilés en GsE puis réellement exécutés
pendant `make test`. Leurs codes de retour attendus sont respectivement `120`,
`44` et `45`. Un second scénario lie deux `.GsObj` échangeant une structure par
valeur et retourne `46`. L’application `GsPlusPlus/Tests/Integration/BibliothequeSysteme.GsPP` lie la
bibliothèque GsA, exécute ses quatre modules et retourne `64`. Le rapport
de la bibliothèque hébergée retourne `170`, et le classificateur Gs++
concorde sur 63 entrées. Le rapport complet de la version courante se trouve
dans
[`VALIDATION-GS-PLUS-PLUS-0.17.1.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.17.1.md).

## Construction

Avec GNU Make :

```bash
make
make test
make bibliotheque-systeme
make bibliotheque-hebergee
make autohebergement
make image-uefi
```

Avec CMake :

```bash
cmake -S . -B build-cmake
cmake --build build-cmake
ctest --test-dir build-cmake
cmake --build build-cmake --target bibliotheque_systeme
cmake --build build-cmake --target bibliotheque_hebergee
cmake --build build-cmake --target autohebergement_images
cmake --build build-cmake --target image_uefi
```

Les exécutables obtenus avec Make sont :

- `Construction/Bin/gsppc` : compilateur Gs++ ;
- `Construction/Bin/gseverifier` : vérificateur structurel GsE ;
- `Construction/Bin/gsechargeur` : chargeur et exécuteur hébergé GsE ;
- `Construction/Bin/gseload` : alias exécutable anglais du chargeur.

La cible `bibliotheque-systeme` produit `Construction/Artefacts/GsPlusPlus/Bibliotheques/Systeme/GsSysteme.GsA`. Une application
utilise son interface et la lie comme ceci :

```bash
Construction/Bin/gsppc GsPlusPlus/Bibliotheques/Systeme/Systeme.HGsPP application.GsPP \
    --format gsobj -o Construction/Application.GsObj
Construction/Bin/gsppc Construction/Application.GsObj Construction/Artefacts/GsPlusPlus/Bibliotheques/Systeme/GsSysteme.GsA \
    --format gse --point-entree Principal -o Construction/Application.GsE
```

La cible `bibliotheque-hebergee` produit `Construction/Artefacts/GsPlusPlus/Bibliotheques/Hebergee/GsHebergee.GsA`.
`make autohebergement` construit en plus
`Construction/Artefacts/GsPlusPlus/AutoHebergement/ClassificateurMotsCles.GsE` et vérifie son résultat
contre le lexeur C++.

La construction EFI demande GNU `ld` avec l’émulation `i386pep`, GNU `objcopy` et Python 3. Aucun SDK UEFI externe, mtools ou outil de formatage FAT n’est requis.

La cible `image-uefi` produit également :

- `Construction/SanctuaireSE/Amorcage/UEFI/BOOTX64.EFI` : application UEFI PE32+ x86-64 ;
- `Construction/Artefacts/SanctuaireSE/Noyau/Noyau.GsE` : noyau Sanctuaire ;
- `Construction/Artefacts/SanctuaireSE/UEFI/Sanctuaire-ESP.img` : volume FAT32 prêt à être présenté à un micrologiciel UEFI.

## Utilisation

Produire un objet COFF :

```bash
Construction/Bin/gsppc GsPlusPlus/Exemples/Bonjour.Gs++ -o Construction/Bonjour.obj
```

Compiler séparément une bibliothèque et une application :

```bash
Construction/Bin/gsppc GsPlusPlus/Tests/Integration/Separation/Calculs.HGsPP \
    GsPlusPlus/Tests/Integration/Separation/Calculs.GsPP \
    --format gsobj -o Construction/Calculs.GsObj

Construction/Bin/gsppc GsPlusPlus/Tests/Integration/Separation/Calculs.HeaderGsPlusPlus \
    GsPlusPlus/Tests/Integration/Separation/Principal.GsPP \
    --format gsobj -o Construction/Principal.GsObj

Construction/Bin/gsppc Construction/Calculs.GsObj \
    --format gsa -o Construction/Calculs.GsA

Construction/Bin/gsppc Construction/Principal.GsObj Construction/Calculs.GsA \
    --format gse \
    --point-entree Essai::Separation::Principal \
    --carte Construction/Application.map \
    -o Construction/Application.GsE
```

Construire une solution versionnée :

```bash
Construction/Bin/gsppc GsPlusPlus/Tests/Integration/Separation/Compilation.GsPs
```

La solution d’exemple construit d’abord `Bibliotheque.GsPj`, puis
`Application.GsProject`. Les chemins d’un projet sont résolus relativement au
fichier de projet.

Produire un `.GsE` à partir de plusieurs sources :

```bash
Construction/Bin/gsppc \
    GsPlusPlus/Exemples/Point.GsPP \
    GsPlusPlus/Exemples/Globales.GsPP \
    GsPlusPlus/Exemples/Application.GsPP \
    --format gse \
    --point-entree Shrine::Exemples::Principal \
    --nom "Application Sanctuaire" \
    --version-application 0.10.2 \
    --editeur "Galactic Shrine" \
    -o Construction/Application.GsE
```

Vérifier indépendamment le fichier produit :

```bash
Construction/Bin/gseverifier Construction/Application.GsE
```

Charger l’application et fournir l’adresse de son import obligatoire :

```bash
Construction/Bin/gsechargeur Construction/Application.GsE \
    --base 0x10000000 \
    --resoudre Shrine::Exemples::Journaliser=0x10002000
```

L’alias anglais de `--resoudre` est `--resolve`.

## Premier noyau Sanctuaire

Construire l’image du noyau minimal :

```bash
make image-noyau
```

La commande équivalente est :

```bash
Construction/Bin/gsppc \
    SanctuaireSE/Noyau/Sources/ContexteDemarrage.GsPP \
    SanctuaireSE/Noyau/Sources/Memoire.GsPP \
    SanctuaireSE/Noyau/Sources/Pagination.GsPP \
    SanctuaireSE/Noyau/Sources/Console.GsPP \
    SanctuaireSE/Noyau/Sources/Exceptions.GsPP \
    SanctuaireSE/Noyau/Sources/Interruptions.GsPP \
    SanctuaireSE/Noyau/Sources/Noyau.GsPP \
    --format gse \
    --point-entree Sanctuaire::Noyau::Demarrer \
    --nom "Noyau Sanctuaire" \
    --version-application 0.10.2 \
    -o Construction/Artefacts/SanctuaireSE/Noyau/Noyau.GsE
```

Le noyau de diagnostic ne possède aucun import. Il peut être chargé puis exécuté sur une machine x86-64 compatible :

```bash
Construction/Bin/gseverifier Construction/Artefacts/SanctuaireSE/Noyau/Noyau.GsE
Construction/Bin/gsechargeur Construction/Artefacts/SanctuaireSE/Noyau/Noyau.GsE --executer-noyau
```

Le résultat attendu est `Code de retour : 5`. L’alias anglais de `--executer-noyau` est `--execute-kernel`. Cette option exécute réellement du code machine : elle doit être réservée aux images de confiance.

Les API du noyau conservent aussi une forme française canonique avec alias anglais : `InitialiserMemoire`/`InitializeMemory`, `AllouerPage`/`AllocatePage`, `AllouerPages`/`AllocatePages`, `LibererPage`/`FreePage`, `LibererPages`/`FreePages`, `InitialiserTasNoyau`/`InitializeKernelHeap`, `AllouerMemoireNoyau`/`AllocateKernelMemory`, `LibererMemoireNoyau`/`FreeKernelMemory`, `InitialiserPages`/`InitializePaging`, `PaginationDisponible`/`PagingAvailable`, `InitialiserConsole`/`InitializeConsole`, `InitialiserInterruptions`/`InitializeInterrupts`, `GererInterruption`/`HandleInterrupt`, `LireDernierCodeClavier`/`ReadLastKeyCode` et `Demarrer`/`Start`. Depuis Gs++ 0.11.0, ces formes anglaises sont de vrais alias et non plus des fonctions-ponts.

La fonction non appelée `TesterDivisionZero` — alias `TestDivideByZero` — permet de produire volontairement l’exception processeur 0 dans une image de diagnostic dédiée. Elle ne doit jamais être exécutée avec le chargeur hébergé, qui ne possède pas l’IDT UEFI de Sanctuaire.

## Image UEFI

Construire toute la chaîne d’amorçage :

```bash
make image-uefi
```

L’image FAT32 contient exactement cette arborescence logique :

```text
EFI/BOOT/BOOTX64.EFI
Sanctuaire/Noyau.GsE
```

Au démarrage, `BOOTX64.EFI` :

1. ouvre le volume depuis lequel il a été chargé ;
2. lit et contrôle l’en-tête et les segments de `Noyau.GsE` ;
3. refuse actuellement les imports externes du noyau ;
4. alloue et initialise l’image du noyau ;
5. découvre le framebuffer GOP et les tables ACPI ;
6. réserve jusqu’à 16 Mio de secours et prépare la GDT, l’IDT, la TSS, deux piles protégées et les tables de pages ;
7. obtient la carte mémoire finale, normalise `ConventionalMemory` et récupère les régions `BootServices` qui ne contiennent ni la pile UEFI active ni l’image du chargeur ;
8. appelle `ExitBootServices`, active NX et `CR0.WP`, puis charge le nouveau `CR3` ;
9. bascule `RSP` sur la pile noyau et transmet un `ContexteDemarrage*` à `Sanctuaire::Noyau::Demarrer` ;
10. après l’initialisation Gs++, découvre le MADT ACPI, active xAPIC/I/O APIC, programme le PIT et initialise le clavier PS/2 ;
11. active les interruptions puis attend avec `hlt` ; les vecteurs 32 et 33 sont distribués à `GererInterruption`.

Le chargeur EFI est un PE32+ relogeable, sans DLL ni runtime C/C++. L’outil FAT32 relit ensuite les deux fichiers à travers leurs chaînes de clusters afin de vérifier l’image produite.

Les tests réels des rc1 et rc2 ont identifié la même faute `#GP(0)` à l’étape 06. En rc2, le dernier cadre IRQ est resté nul : le stub n’avait donc jamais été atteint. L’analyse des relocalisations a montré que `R_X86_64_REX_GOTPCRELX` chargeait les huit premiers octets du stub (`4157565553525150`) au lieu de son adresse. La rc3 a corrigé le défaut avec une table assembleur relogeable, comme les exceptions déjà fonctionnelles. Son démarrage UEFI réel a réussi le 18 juillet 2026 sous QEMU 11.0.50 et OVMF. Sanctuaire SE 0.10.2 a ensuite été validé le 22 juillet 2026 sous QEMU 11.0.50 et OVMF avec l’ancienne numérotation locale GsE 2.0 : le témoin mémoire vert, l’horloge PIT/IRQ0 et le clavier PS/2 ont tous été confirmés. L’image ESP validée possède l’empreinte SHA-256 `9081371cc4b407d115e4ececc3d93d7e0eed43bf036e787b59bae12f4b3a451d`.

Afficher les étapes de compilation :

```bash
Construction/Bin/gsppc GsPlusPlus/Exemples/Bonjour.Gs++ --jetons --ast -o Construction/Bonjour.obj
```

Utiliser les diagnostics anglais :

```bash
Construction/Bin/gsppc source.GsPP --langue-diagnostics anglais
```

Les options françaises importantes ont aussi un alias anglais : `--point-entree`/`--entry-point`, `--nom`/`--name`, `--version-application`/`--application-version` et `--editeur`/`--publisher`.

## Français natif et alias anglais

Forme française canonique :

```gspp
espace Shrine::Exemples
{
    publique entier32 NombreAppels = 0;
    externe entier32 Journaliser(entier32 valeur);

    publique entier32 Principal()
    {
        NombreAppels = NombreAppels + 1;
        retourner Journaliser(NombreAppels);
    }
}
```

Un nom anglais applicatif se déclare sans recopier l’implémentation :

```gspp
espace Sanctuaire::Noyau
{
    publique entier32 Demarrer(ContexteDemarrage* contexte)
    {
        retourner contexte->Version;
    }

    alias Start = Demarrer;
    alias BootContext = ContexteDemarrage;
}
```

`Demarrer` et `Start` possèdent alors la même adresse. Une déclaration d’alias
de fonction externe est normalisée vers l’import canonique et ne crée jamais
un second import obligatoire. La sémantique complète est décrite dans
[`ALIAS_GS_PLUS_PLUS_0.11.md`](ALIAS_GS_PLUS_PLUS_0.11.md).

Forme anglaise équivalente :

```gspp
namespace Shrine::Exemples
{
    public int32 NombreAppels = 0;
    extern int32 Journaliser(int32 valeur);

    public int32 Principal()
    {
        NombreAppels = NombreAppels + 1;
        return Journaliser(NombreAppels);
    }
}
```

## Pointeurs de fonction

Le français et l’anglais produisent le même type interne :

```gspp
pointeur_fonction<entier32(entier32)> operation = &Doubler;
function_pointer<int32(int32)> operation = &Doubler;
```

Un nom de fonction peut aussi être affecté sans `&`. Les appels utilisent la
syntaxe ordinaire : `operation(21)`, `objet.Executer(21)` ou
`table[index](21)`. Une initialisation globale accepte une adresse de fonction
directe ; le format GsE la matérialise au chargement avec `BASE64`.

## Format GsE 1.0

Le format expérimental sépare les régions exécutables et modifiables afin de respecter la règle W^X. Un fichier peut contenir les sections suivantes :

| Section française | Rôle | Alias conceptuel anglais |
|---|---|---|
| `.texte` | code machine | `.text` |
| `.donnees` | données initialisées | `.data` |
| `.zero` | mémoire initialisée à zéro | `.bss` |
| `.imports` | symboles externes | imports |
| `.exports` | symboles publics | exports |
| `.relog` | relocalisations d’import et de base interne | relocations |
| `.chaines` | noms UTF-8 des imports et exports | string table |
| `.meta` | métadonnées GsC | metadata |

Chaque entrée `.imports` ou `.exports` contient une position et une longueur
référençant `.chaines`. Les chaînes sont terminées par un octet nul, encodées en
UTF-8 et limitées à 1 024 octets, sans compter la terminaison. Les entrées
d’import et d’export occupent chacune 32 octets. Le format canonique 1.0 ne
charge pas les fichiers portant les anciennes numérotations locales 1.1 ou 2.0.

`gseverifier` contrôle notamment l’en-tête, la taille de l’image, les limites des tables et sections, les alignements, les chevauchements, W^X, le point d’entrée, les références vers la table de chaînes, l’encodage UTF-8, les imports, les exports, les relocalisations et la cohérence des métadonnées.

`gsechargeur` refuse une image invalide avant toute allocation exécutable. Il copie les segments, conserve les zones non initialisées à zéro, résout les imports, applique les relocalisations, puis matérialise les protections W^X avant une éventuelle exécution.

La disposition binaire complète est décrite dans
[`FORMAT_GSE_1.0.md`](FORMAT_GSE_1.0.md).

## Limites actuelles

- quatre emplacements de paramètres ou arguments au maximum ; le pointeur de
  retour caché d’une structure en réserve un et limite alors la fonction à
  trois paramètres explicites ;
- structures et unions transmises ou retournées par valeur, mais tableaux
  fixes encore non copiables, non transmissibles et non retournables ;
- initialisations agrégées positionnelles, sans champs désignés ;
- énumérations avec type sous-jacent `entier32` fixe ;
- conversions dynamiques réductrices définies par troncature, sans piège d’exécution automatique ;
- les vues de la bibliothèque système ne possèdent pas leur stockage ; l’appelant
  doit garantir sa durée de vie, et `CopierMemoire` exige des régions sans
  chevauchement ;
- les atomiques 32/64 bits exigent un stockage naturellement aligné et ciblent
  actuellement x86-64 ;
- initialisation globale limitée aux constantes scalaires, agrégats constants
  et adresses de fonctions ; une chaîne peut être utilisée dans une fonction,
  mais son adresse n’est pas encore admise comme initialiseur global ;
- le classificateur des mots-clés est auto-hébergé, mais le découpage complet
  des jetons, le parseur, l’analyse sémantique et le backend restent en C++ ;
- portée locale sans masquage lexical complet ;
- backend x86-64 uniquement ;
- GsObj est le format objet natif ; COFF reste une sortie interopérable mais n’est pas lu par l’éditeur de liens Gs++ ;
- les alias de fonctions, globales, structures et champs sont pris en charge, mais l’alias d’un espace de noms complet ne l’est pas encore ;
- l’allocateur et le tas ne sont pas encore synchronisés pour plusieurs processeurs ;
- le tas réserve actuellement des pages entières et ne subdivise pas encore une page en petits objets ;
- les exceptions sont identifiées et affichées, puis arrêtent le processeur ; la reprise après exception n’est pas encore prise en charge ;
- l’initialisation matérielle actuelle cible xAPIC, I/O APIC, PIT et clavier PS/2 ; x2APIC, HPET, USB HID et les pilotes matériels génériques restent à écrire ;
- le chargeur UEFI 0.10.2 refuse les imports GsE externes ;
- la pagination initiale est identitaire, limitée à 512 Gio ; Gs++ en possède désormais les métadonnées et les contrôles de bornes, mais ne modifie pas encore dynamiquement les entrées ni n’installe un noyau en adresse virtuelle haute ;
- aucune signature Secure Boot/GsE n’est encore produite ;
- Sanctuaire SE 0.10.2 et son image utilisant l’ancienne numérotation locale
  GsE 2.0 ont été validés en démarrage UEFI réel ; l’image reconstruite au
  format canonique GsE 1.0 a également réussi le test réel QEMU/OVMF le
  15 août 2026 ;
- la normalisation Unicode NFC reste à renforcer ;
- signature cryptographique GsE prévue pour une version ultérieure.

Ce projet fournit maintenant la chaîne native `interfaces Gs++ → objets GsObj → bibliothèques GsA → Noyau.GsE → BOOTX64.EFI → CR3 W^X → pile noyau → xAPIC/I/O APIC → IRQ → Gs++`. Les valeurs structurées de 0.15, `GsSysteme.GsA` en 0.16 et `GsHebergee.GsA` en 0.17 permettent désormais d’exécuter un premier composant du compilateur écrit dans le langage. La prochaine priorité historique était le modèle objet système de Gs++ 0.18. La feuille de route courante du compilateur est décrite dans [`FEUILLE_DE_ROUTE_GS_PLUS_PLUS.md`](FEUILLE_DE_ROUTE_GS_PLUS_PLUS.md).
