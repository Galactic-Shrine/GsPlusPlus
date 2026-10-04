# Plan produit Gs++ 1.0

## Décision normative

**DÉCIDÉ — 16 août 2026.** Gs++ devient le seul produit développé activement
dans l’écosystème système Galactic-Shrine jusqu’à l’obtention d’une version
1.0 réellement exploitable. Sanctuaire SE 0.10.2 reste figé fonctionnellement
et sert de banc d’intégration UEFI obligatoire. Le développement actif de
Gs#, de Sanctuaire SE 0.11 et des autres couches reprendra après la validation
des critères de sortie de Gs++ 1.0.

Cette décision ne signifie pas que Gs++ doit reproduire toutes les fonctions
de C++. Le produit est considéré comme complet lorsque son périmètre publié
est cohérent, autonome, documenté, testable et redistribuable.

### Compilation adaptée à la plateforme — décision du 19 septembre 2026

**DÉCIDÉ — IMPLÉMENTATION À RÉALISER.** Gs++ doit produire un programme pour
la plateforme de l'utilisateur par défaut et permettre de sélectionner une
autre plateforme explicitement. Une cible décrit le processeur, le système,
le format binaire et les conventions d'appel et de liaison (ABI). La machine
qui exécute le compilateur est l'hôte ; elle peut différer de la cible.

Le premier périmètre couvre les cibles x86-64 suivantes :

| Cible | Exécutable final prévu | Intégration système à fournir |
| --- | --- | --- |
| Windows | PE/COFF, `.exe` | démarrage, imports et SDK Windows ; interopérabilité Microsoft x64 |
| GNU/Linux | ELF | démarrage, liaison et SDK GNU/Linux ; interopérabilité System V AMD64 |
| Sanctuaire SE / ShrineOS | `.GsE` | contrat GsE et services du système cible ; ABI Gs++ documentée |

Le langage et son analyse restent communs. Les bibliothèques du profil hébergé
fournissent des services portables, implémentés pour chaque cible. Un programme
qui utilise directement une API propre à un système doit adapter cette partie
pour une autre cible. Le SDK décrit les interfaces et fournit les bibliothèques
et composants de démarrage ; le backend et l'éditeur de liens produisent le
code et le fichier binaire attendus par la cible.

Le comportement à implémenter est le suivant :

- choix de cible en ligne de commande prioritaire sur celui du projet XML ;
- cible déclarée dans le projet prioritaire sur la détection de l'hôte ;
- en l'absence de choix explicite, sélection de la cible native prise en
  charge pour l'environnement du compilateur, y compris Linux sous WSL ;
- affichage de la cible effective et séparation des sorties par cible et
  configuration dans les répertoires de construction ;
- diagnostic explicite pour une architecture, une cible ou un SDK absent,
  ainsi que pour un format demandé incompatible avec la cible ;
- compilation croisée lorsque le backend, l'éditeur de liens, le SDK et les
  bibliothèques de la destination sont disponibles ; l'exécution des tests
  exige aussi un environnement capable d'exécuter le programme cible ;
- rejet des objets ou bibliothèques incompatibles à la liaison ; l'identité
  de cible et d'ABI participe aux métadonnées et aux clés de reconstruction.

Les projets actuels qui produisent volontairement du GsE doivent conserver
une cible explicite lors de l'introduction du nouveau comportement par défaut.
Les formats GsObj/GsA/GsE et leur contrat 1.0 restent documentés comme tels ;
les sorties natives Windows et Linux ajoutent des contrats de cible distincts.
Le support d'autres systèmes ou architectures demande un portage et sa propre
validation ; il n'est pas acquis par la seule détection de la machine.

Aujourd'hui, `gsppc` choisit entre COFF, GsObj, GsA et GsE et génère du code
x86-64 selon le contrat Gs++ courant. Son fonctionnement sous Windows et Linux
ne démontre pas encore la production automatique de PE/ELF pour ces systèmes.
La sélection de cible, les sorties natives et les SDK associés font désormais
partie des travaux de convergence ci-dessous.

## Point de départ vérifié

Gs++ 0.26.0 constitue le jalon candidat actuel. Il conserve le contrat
candidat 1.0 de 0.24, l’initialisation déterministe de 0.25 et ajoute le socle
de propriété mémoire du profil hébergé.

### VALIDÉ

- frontend et backend natif x86-64 ;
- compilation multi-unités et édition de liens ;
- projets `.GsPj`/`.GsProject` et solutions `.GsPs` ;
- objets `.GsObj`, bibliothèques `.GsA` et exécutables `.GsE` ;
- types système, valeurs structurées, callbacks et agrégats ;
- modèle objet freestanding, RAII, virtuel et héritage simple public ;
- initialisation de la base, des champs directs et des champs objets classes ;
- tableaux multidimensionnels d’objets classes et destruction inverse ;
- valeurs par défaut des champs, constructeurs délégués et arguments uniformes
  des tableaux d’objets ;
- exclusion normative des objets de classe globaux dans le profil
  freestanding ;
- constructions MSVC et GNU, tests hébergés et démarrage QEMU/OVMF ;
- formats natifs 1.0 et ABI 1 ;
- spécifications candidates du langage, de GsObj, GsA, GsE, de l’ABI et des
  profils ;
- chaînes UTF-8, conteneurs dynamiques, arène, chemins et fichiers du profil
  hébergé avec cinq imports explicites ;
- conformité 18/18 sous MSVC et GNU avec rapport JSON.

### PARTIEL

- auto-hébergement limité au premier composant
  `ClassificateurMotsCles` ;
- documentation finale encore distribuée entre les contrats 0.11 à 0.25 ;
- outils de diagnostic, d’installation et de distribution.

### PRÉVU

- migration progressive du compilateur C++ de bootstrap vers Gs++ ;
- bootstrap génération N vers N+1 puis N+2 ;
- extension de la conformité aux capacités des jalons 0.26 à 0.29 ;
- durcissement systématique des lecteurs GsObj/GsA/GsE ;
- installation locale et paquets reproductibles Windows et GNU/Linux.

## Invariants gelés

Les travaux menant à 1.0 doivent conserver les contrats suivants, sauf décision
normative explicite rendue nécessaire par une impossibilité démontrée.

| Élément | Contrat |
| --- | --- |
| Sources Gs++ | `.Gs++`, `.GsPP`, `.GsPlusPlus` |
| Interfaces Gs++ | `.HGs++`, `.HGsPP`, `.HeaderGsPlusPlus` |
| Projets | `.GsPj`, `.GsProject` |
| Solutions | `.GsPs` |
| Objet | `.GsObj`, `GSOBJ:0` + un zéro, format 1.0, ABI 1, en-tête 112 octets |
| Bibliothèque | `.GsA`, `GSA:0` + trois zéros, format 1.0, ABI 1, en-tête 32 octets |
| Exécutable | `.GsE`, `GSE:0` + trois zéros, format 1.0, ABI 1, en-tête 112 octets |
| Signature de liaison du contrat Gs++ actuel | `GsAbi:x64-ms-v1` ; les nouvelles cibles natives doivent définir leurs contrats distincts |
| Documentation canonique | Markdown `.md` |
| Langue canonique | français, avec alias anglais officiels lorsqu’ils existent |

Les extensions `.GsPH`, `.GsO`, `.GsPPH` et `.GsPlusPlusHeader` restent
obsolètes et refusées. Les sources Gs# `.Gs#`, `.GsS` et `.GsSharp` restent
réservées au futur compilateur Gs# ; Gs# ne possède aucun fichier d’en-tête.

## Définition d’un produit Gs++ exploitable

Gs++ 1.0 doit satisfaire simultanément les domaines suivants.

### 1. Contrat du langage

- spécification canonique couvrant syntaxe, types, conversions et durée de vie ;
- séparation claire des profils freestanding et hébergé ;
- disposition mémoire et ABI documentées ;
- comportement français/anglais équivalent ;
- fonctions prises en charge et fonctions volontairement absentes identifiées ;
- diagnostics normatifs pour les erreurs essentielles.

### 2. Chaîne native

- compilation source vers GsObj ;
- création et lecture de GsA ;
- édition de liens et création de GsE ;
- vérification et chargement de GsE ;
- constructions déterministes lorsque les entrées sont identiques ;
- erreurs sûres sur les entrées tronquées, incohérentes ou excessives ;
- cartes de liens et sorties machine utilisables par l’automatisation.

La décision multi-cible ajoute la sélection native ou explicite de plateforme,
la production d'exécutables Windows PE et Linux ELF et la liaison compatible
avec leurs SDK. Le format GsE conserve son usage pour la cible Galactic-Shrine.

### 3. Bibliothèques

Le profil système doit rester freestanding, sans allocation, exception, import
ou initialisation cachée. Le profil hébergé doit fournir au minimum les outils
nécessaires au compilateur :

- chaînes et vues UTF-8 ;
- vecteurs et stockage dynamique explicite ;
- tables associatives et ensembles ;
- fichiers, chemins et flux ;
- diagnostics et modèle d’erreur explicite ;
- structures adaptées aux jetons, AST, symboles, types et relocalisations.

### 4. Auto-hébergement

Le compilateur Gs++ reconstruit doit pouvoir reconstruire une nouvelle
génération fonctionnelle :

```text
Compilateur de bootstrap N
        ↓
Compilateur Gs++ N+1
        ↓
Compilateur Gs++ N+2
```

N+1 et N+2 doivent repasser la même suite de conformité. Leur comparaison doit
être identique bit à bit lorsque le format le permet, ou reposer sur une
normalisation documentée et contrôlée. Le seul classificateur de mots-clés ne
suffit pas à déclarer l’auto-hébergement complet.

### 5. Qualité et sécurité

- tests positifs, négatifs, inter-unités et bilingues ;
- tests des incompatibilités ABI ;
- tests des limites et dépassements arithmétiques ;
- corpus de fichiers GsObj/GsA/GsE malformés ;
- fuzzing ou génération systématique d’entrées invalides ;
- absence de régression sous MSVC et GNU ;
- reconstruction de Sanctuaire SE et démarrage QEMU/OVMF après chaque jalon
  susceptible d’affecter le code natif ou les formats.

### 6. Distribution

- installation locale versionnée ;
- désinstallation propre ;
- SDK et modèles de projets ;
- paquets Windows et GNU/Linux ;
- sommes SHA-256 ;
- documentation utilisateur et développeur ;
- politique de compatibilité et de versionnement.

## Périmètre fonctionnel de 1.0

Les fonctions nécessaires à l’écriture robuste du compilateur et de ses
bibliothèques sont prioritaires. Gs++ 0.25 inclut les valeurs par défaut des
champs de classes, les constructeurs délégués et les arguments uniformes des
tableaux d’objets. Il exclut les objets de classe globaux afin de préserver le
profil freestanding sans initialisation ou destruction cachée. Le modèle
modèle d’erreur hébergé sans dépendance obligatoire aux exceptions est fourni
par la bibliothèque 0.26.

L’héritage multiple, l’héritage virtuel, la RTTI et les exceptions du langage
ne sont pas des conditions automatiques de Gs++ 1.0. Ils peuvent rester hors du
périmètre si leur absence est normative, diagnostiquée et compatible avec
l’auto-hébergement. Les méthodes virtuelles pures et les conversions
descendantes doivent recevoir la même décision explicite.

## Jalons de convergence

### Gs++ 0.24 — contrat et conformité — terminé

- figer le périmètre du langage 1.0 ;
- produire la spécification normative GsObj 1.0 manquante ;
- consolider les contrats GsA, GsE et ABI ;
- définir les profils freestanding et hébergé ;
- créer la structure de la suite de conformité ;
- transformer chaque limite actuelle en décision suivie.

La 0.24.0 livre ces éléments, réussit 5/5 tests sous MSVC, 6/6 sous GNU/WSL,
13/13 cas de conformité sur chaque chaîne et la preuve QEMU/OVMF.

### Gs++ 0.25 — initialisation et durée de vie — terminé

- valeurs par défaut des champs de classes ;
- délégation `soi`/`this` entre constructeurs sans cycle ;
- arguments uniformes réévalués pour chaque élément de tableau d’objets ;
- exclusion explicite des objets de classe globaux ;
- tests RAII, diagnostics et conformité étendue à 16 exigences.

La 0.25.0 réussit 5/5 tests sous MSVC, 6/6 sous GNU/WSL, 16/16 cas de
conformité sur chaque chaîne, les benchmarks smoke et la preuve QEMU/OVMF.

### Gs++ 0.26 — bibliothèque hébergée — terminé

- compléter les chaînes, conteneurs, fichiers, chemins et diagnostics ;
- garantir le profil système sans dépendance hébergée cachée ;
- fournir les structures nécessaires à la migration du compilateur.

La 0.26.0 ajoute cinq imports d’hôte explicites, les chaînes UTF-8,
vecteurs dynamiques, table de symboles, arène stable, chemins et fichiers
alloués. Elle réussit 5/5 tests MSVC, 6/6 tests GNU/WSL, 18/18 exigences sur
chaque chaîne, les benchmarks smoke et la preuve QEMU/OVMF.

### Gs++ 0.27 — frontend auto-hébergé

La préversion actuelle est **0.27.0-alpha.10**, avec sa
[matrice de validation](Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.10.md).
Cette version consolide les lots après alpha.9 et valide les paquets extraits ;
elle ne clôt pas le frontend 0.27.

- **VALIDÉ** : lexeur Gs++ complet, API bornée et comparaison différentielle
  MSVC/GNU avec le bootstrap C++ ;
- **PARTIEL — TRANCHE SYNTAXIQUE VALIDÉE** : AST compact des déclarations,
  membres exécutables, instructions et expressions, construit dans l’arène
  0.26 et comparé différentiellement au bootstrap C++ ;
- **PARTIEL — TRANCHE SÉMANTIQUE VALIDÉE** : tables de symboles, références,
  sélection typée des fonctions et méthodes, accès aux membres, visibilité,
  constructeurs locaux, opérateurs membres et libres, contraintes des
  expressions indirectes et plans ordonnés de construction, destruction et
  tables virtuelles des objets locaux et sous-objets de constructeurs, ainsi
  que les vingt-quatre formes d’opérateurs intrinsèques avec adaptation des
  littéraux et diagnostics de types, les liaisons de références qualifiées,
  les affectations, les valeurs de retour et les contraintes structurelles des
  déclarations globales, ainsi que la validation récursive des formes
  constantes, agrégats et pointeurs de leurs initialiseurs, l’évaluation
  numérique des constantes et valeurs d’énumération, les contrôles de plage et
  les divisions par zéro, puis l’émission en mémoire des données globales,
  de leur disposition et des relocalisations de fonctions ; l’alpha.9 ajoute
  les contraintes des conversions explicites, signatures et plages constantes ;
  le développement suivant ajoute l'adaptation implicite des constantes
  composées, leurs types effectifs et les contrôles de sélection de surcharges,
  puis les références de callbacks, signatures imbriquées et tableaux de
  pointeurs à indirections profondes, avec contrôle de leur disposition globale ;
  puis la résolution contextuelle des types nommés dans les signatures,
  les appels de callbacks stockés dans les champs et le diagnostic 100 pour les
  types déclarés inconnus, dans une copie privée préservant l'AST de l'appelant ;
  puis contraintes récursives des signatures de callbacks, paramètres et limites
  d'arité des fonctions/méthodes, diagnostics 101–106 et opérateurs libres sur
  structures/unions ; puis résolution itérative des chaînes d'alias de champs,
  validation des alias inutilisés et diagnostics 107–108, avec stockage
  canonique partagé par les accès et les initialiseurs de constructeurs ;
  puis alias racines de types, fonctions libres et globales, déclarations
  anticipées, chaînes, noms qualifiés et diagnostics 109–112, avec cibles
  canoniques communes aux types, accès, callbacks et relocalisations ;
  puis appels via alias de méthodes non liées, récepteur mutable `Classe&`
  explicite, visibilité des appels directs, signatures de callbacks et
  relocalisations vers la méthode canonique, sans modification de l'AST public ;
  puis déclarations d'héritage, résolution canonique des bases dans l'espace
  déclarant, refus des bases non publiques, absentes ou non-classes et de
  l'auto-héritage, cycles indirects, priorités et diagnostics 113–115 ; cette
  tranche est locale, postérieure à la publication alpha.10 ; puis remplacements
  de méthodes, destructeurs et opérateurs virtuels, signatures canoniques sans
  récepteur, diagnostics 116–117 et dispositions de classes possédant uniquement
  un destructeur ou opérateur virtuel, dans le périmètre différentiel testé ;
  puis doublons de surcharges libres et membres, constructeurs, destructeurs
  et opérateurs, paramètres canoniques et alias de types, retour exclu de
  l'identité d'une surcharge, diagnostic bilingue 118 ; puis comparaison des
  signatures non liées, récepteur implicite en première position, et collisions
  méthode/fonction libre de même nom complet dans un espace homonyme de la
  classe ; puis collisions entre symboles de liaison calculés pour des
  surcharges distinctes, affichage canonique des paramètres et récepteur
  implicite, espaces qualifiés ou UTF-8 et diagnostic bilingue 119 ; puis
  sélection des appels qualifiés, par objet ou pointeur, dans les groupes
  mêlant méthodes non liées et fonctions libres, avec score du récepteur,
  visibilité après sélection, ambiguïtés, masquage et déclaration effectivement
  retenue comparés au bootstrap, dans la matrice locale du 3 octobre 2026 ; puis
  groupes d'opérateurs unaires et binaires mixtes, sélection avec récepteur et
  visibilité après score ; priorité déterministe des doublons entre groupes
  indépendants suivant leur première déclaration, collisions de liaison et
  parcours des corps dans l'ordre source, dans le périmètre testé ; puis
  priorité des instructions successives, blocs, expressions de conditions,
  branches et boucles d'un même corps ; puis priorité des opérandes, de l'objet
  avant l'indice, de la cible d'affectation avant sa valeur et du type cible de
  conversion avant sa source, dans la matrice du 4 octobre 2026, avec parcours
  itératif sans pile auxiliaire, AST et sorties d'émission préservés ; puis
  arguments d'appels dans l'ordre source, cible indirecte et arité contrôlées
  avant eux, rejet des groupes sans arité et récepteur recevables, sélection
  différée et appels imbriqués ; puis abandon des candidats après un préfixe
  incompatible, dans l'ordre de déclaration avant l'argument suivant, avec
  erreurs de types de conversions différées jusqu'à la visite de leur expression,
  dans le périmètre testé le 4 octobre 2026 ; puis arguments agrégés contextuels,
  après sélection et visibilité pour les groupes directs, dans l'ordre des
  arguments pour les callbacks, avec formes scalaires, structures, unions,
  tableaux de champs imbriqués, alias et appels imbriqués dans la matrice testée ;
  puis priorité du calcul et du contrôle de plage des conversions constantes
  lors de leur visite, avec arguments, agrégats contextuels, déclarations et
  courts-circuits dans la matrice du 4 octobre 2026 ;
- **EN COURS** : compléter la matrice des conversions et qualifications et les autres
  familles sémantiques, notamment les contextes des constructions et les
  interactions de priorité entre passes non encore testées, dont les
  initialiseurs de déclarations et les combinaisons de conversions non encore
  couvertes ;
  raccorder les données émises aux futurs écrivains
  d’objets auto-hébergés dans le jalon backend ;
- **EN COURS** : comparer systématiquement les résultats au bootstrap C++.

Le contrat et les preuves intermédiaires du lexeur et de l’AST sont décrits dans
[`FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md`](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md).

Le passage au jalon 0.28 n'est pas encore validé. Les prochaines tranches
portent sur les interactions sémantiques restantes, notamment les initialiseurs
de déclarations et les constructions, puis sur
la consolidation de la conformité et des
benchmarks du frontend 0.27. Le nombre de corpus réussis n'est ni un pourcentage
d'achèvement ni un déclencheur automatique de changement de version.
`VERSION` reste à `0.27.0-alpha.10` ; aucune date de sortie 0.28 n'est fixée ici.
L'ouverture d'une `0.28.0-alpha.1` marquera le début du jalon suivant, pas
l'achèvement du backend ni la livraison d'une `0.28.0` finale.

### Gs++ 0.28 — backend et chaîne auto-hébergés

- achever l’analyse sémantique ;
- migrer la génération x86-64 ;
- migrer les écrivains GsObj/bibliothèque/GsE et l’éditeur de liens ;
- remplacer l'extension des bibliothèques statiques `.GsA` par `.Glib` à partir
  de 0.28.0 ; adapter les outils, projets XML, exemples, paquets et tests ;
- réserver `.GdLib` aux bibliothèques dynamiques si ce support est introduit ;
  conserver `.GsE` pour les exécutables. Décision du 21 septembre 2026, prévue
  et non encore implémentée : 0.27 continue d'utiliser `.GsA`. Le nom réservé
  `.GdLib` ne constitue pas une annonce de support dynamique déjà disponible ;
- migrer l’orchestration de projets ;
- introduire la description de cible et sa sélection commune en ligne de
  commande et dans les projets XML, avec défaut natif et diagnostic des
  configurations non prises en charge ;
- ajouter la production native Windows PE et Linux ELF, les conventions
  d'interopérabilité, le démarrage et les bibliothèques de chaque cible ;
- obtenir les générations N+1 et N+2 fonctionnelles.

### Gs++ 0.29 — durcissement produit

- déterminisme et reproductibilité ;
- corpus malformés et fuzzing ;
- conformité complète Windows/GNU ;
- installation, SDK et paquets locaux ;
- exécution native des programmes sous Windows et Linux, validation de la
  cible GsE et des combinaisons hôte/cible de compilation croisée annoncées ;
- documentation finale ;
- validation UEFI avec la toolchain produite.

### Gs++ 1.0.0 — sortie produit

La version 1.0.0 n’est autorisée que lorsque tous les critères de sortie
ci-dessous sont satisfaits.

## Critères de sortie 1.0

- [ ] périmètre du langage 1.0 figé et documenté ;
- [ ] spécifications GsObj, bibliothèque `.Glib` (successeur de `.GsA` à partir
  de 0.28.0) et GsE 1.0 complètes ;
- [ ] ABI 1 documentée et couverte par des tests inter-unités ;
- [ ] bibliothèques système et hébergée suffisantes pour le compilateur ;
- [ ] compilateur principalement maintenu en Gs++ ;
- [ ] génération N+1 capable de produire une génération N+2 fonctionnelle ;
- [ ] comparaison N+1/N+2 conforme à la règle de reproductibilité ;
- [ ] suite de conformité entièrement réussie sous MSVC et GNU ;
- [ ] lecteurs binaires durcis contre les fichiers malformés ;
- [ ] installation et paquets locaux vérifiés ;
- [ ] sélection de cible native par défaut et surcharge explicite vérifiées ;
- [ ] exécutables PE Windows et ELF Linux construits et exécutés sur leur cible ;
- [ ] SDK des cibles annoncées distribués et exemples portables validés ;
- [ ] incompatibilités de cible, de format et d'ABI diagnostiquées ;
- [ ] combinaisons de compilation croisée annoncées construites et testées ;
- [ ] documentation utilisateur et développeur complète en `.md` ;
- [ ] `Noyau.GsE`, `BOOTX64.EFI` et l’ESP reconstruits par la chaîne candidate ;
- [ ] démarrage QEMU/OVMF réussi avec rapport machine ;
- [ ] aucun écart P0 ou P1 connu non résolu.

Une campagne de benchmark ne constitue pas à elle seule un critère de sortie et
ne doit produire aucune revendication comparative sans protocole approprié.

## Gel des autres couches

### Sanctuaire SE

**MAINTENANCE UNIQUEMENT.** La référence fonctionnelle reste 0.10.2. Les
corrections de sécurité et de non-régression restent autorisées. Les nouvelles
fonctions d’ordonnancement, de processus, de mode utilisateur et de services
sont différées jusqu’à Gs++ 1.0. Les reconstructions du noyau et les tests UEFI
restent obligatoires pour valider Gs++.

### Gs#

**DIFFÉRÉ.** Les décisions existantes sont conservées : sources `.Gs#`, `.GsS`
et `.GsSharp`, aucun fichier d’en-tête, cible native sans dépendance obligatoire
à .NET/CLR/Mono. Aucun compilateur Gs# actif n’est développé avant Gs++ 1.0.

### Autres couches

Les sous-systèmes Linux/Windows de Sanctuaire SE, la plateforme Unreal Engine
Sanctuaire SE, son SDK applicatif et ses services avancés restent documentés
mais non développés activement avant la sortie produit de Gs++. Les SDK de
compilation Gs++ pour Windows et GNU/Linux relèvent du produit Gs++ et sont
inclus dans la décision multi-cible ci-dessus.

## Règle de suivi

Chaque jalon doit conserver les statuts suivants sans les confondre :

- `VALIDÉ` : implémenté et couvert par une preuve exécutable actuelle ;
- `PARTIEL` : présent mais ne satisfaisant pas encore son contrat final ;
- `PRÉVU` : architecture ou fonction non encore démontrée ;
- `OBSOLÈTE` : convention rejetée ou remplacée.

La feuille de route et les synthèses doivent être mises à jour après chaque
jalon, mais ce document reste la source de vérité pour la priorité produit et
les critères de sortie de Gs++ 1.0.
