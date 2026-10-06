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

### Séparer compilation et construction — décision du 5 octobre 2026

**DÉCIDÉ — PRÉVU POUR LE JALON 0.28, NON IMPLÉMENTÉ.** Un outil de construction
distinct prend en charge les projets et solutions. Le nom proposé est
**GsBuild**, avec la commande `gsbuild` (`gsbuild.exe` sous Windows).
`GsConstruction` reste une alternative de nom ; `GsConstructeur` est évité
pour ne pas confondre l'outil avec les constructeurs du langage.

La séparation visée est la suivante :

| Composant | Responsabilité prévue |
| --- | --- |
| `gsppc` | Compiler les sources et interfaces Gs++ en objets ; fournir les diagnostics, vues de jetons et d'AST et options de compilation. |
| GsBuild / `gsbuild` | Lire les projets `.GsPj`/`.GsProject` et solutions `.GsPs`, sélectionner les unités, ordonner leur construction et piloter compilation, création des bibliothèques et édition de liens. |
| Services d'archivage et de liaison | Produire les bibliothèques et exécutables à partir des objets, sous le contrôle de GsBuild ; ne pas réintroduire l'orchestration dans `gsppc`. |

GsBuild reprend le rôle d'un moteur de construction pour les projets Gs++,
analogue au rôle de MSBuild, sans annoncer une compatibilité avec ses fichiers
de projets, tâches ou options. CMake et MSBuild restent utilisables pour
construire la toolchain elle-même, notamment le bootstrap C++ et la solution
Visual Studio 2026 native ; ils ne deviennent pas des dépendances obligatoires
pour construire un projet Gs++ avec GsBuild.

Le premier périmètre reprend le contrat XML 1.0 et les comportements existants :
chemins relatifs au projet, ordre des projets de solution, modes séparé/agrégé,
interfaces, bibliothèques, sorties, cartes de liens et métadonnées. La future
sélection de cible doit respecter les mêmes priorités que la compilation :
ligne de commande, puis projet XML, puis cible native prise en charge. Le seul
changement d'outil ne modifie ni les formats binaires, ni le schéma XML, ni l'ABI.
Les reconstructions incrémentales et la planification parallèle seront des
extensions à spécifier et tester, pas des capacités réputées disponibles.

La migration doit :

1. extraire l'orchestration existante de `ConstructeurProjet` vers GsBuild,
   en réutilisant les services de compilation et liaison sans les dupliquer ;
2. adapter les scripts, intégrations CMake/MSBuild, exemples, documentation,
   conformité et paquets à la nouvelle commande ;
3. vérifier sous Windows et GNU/Linux les projets et solutions, les deux modes
   de compilation, les diagnostics et la reproductibilité des sorties ;
4. retirer de `gsppc` les entrées de projets/solutions et les modes de création
   d'archives et de liaison, avec diagnostic de migration vers GsBuild.

**État actuel :** la version 0.27 conserve ses commandes `gsppc` existantes.
Aucun exécutable GsBuild n'est livré par cette décision ; sa disponibilité doit
être établie par l'implémentation, les tests et la distribution du jalon futur.

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
  puis initialiseurs locaux contextuels, avec forme et capacité avant leurs
  éléments, feuilles dans l'ordre source et refus avant l'instruction suivante,
  références, callbacks et plages numériques dans le périmètre testé ;
  puis validation complète de chaque initialiseur global dans l'ordre source,
  typage de toutes les feuilles avant la passe constante de cette globale,
  contrôles structurels des champs par défaut prioritaires dans la matrice du
  4 octobre 2026 ;
  puis champs par défaut évalués dans chaque constructeur utilisateur, listes
  explicites avant valeurs par défaut et corps, remplacement et délégation,
  isolation des expressions du bootstrap et contrôle des prototypes fournis
  à l'API sémantique, dans la matrice du 5 octobre 2026 ;
  puis sélection et plans des constructions locales au moment de leur visite,
  arité et préfixes d'arguments avant sélection, visibilité avant agrégats,
  accès des constructeurs/destructeurs et positions des sous-objets, ainsi que
  plans de corps des constructeurs avant leurs instructions, dans le périmètre
  testé du 5 octobre 2026 ;
  puis validation récursive des bases et champs au moment de leur initialisation,
  avant l'expression suivante, sans étapes publiées lors du contrôle préalable ;
  sélection commune des arguments des bases, champs et délégations, avec
  plan final canonique et réutilisation des choix explicites, dans le périmètre
  testé du 5 octobre 2026 ;
  puis contrôles de déclarations locales lors de leur visite : types, doublons,
  variables `vide`, références et constantes sans initialiseur, constructions
  explicites de types non-classes ; paramètres répétés contrôlés à l'entrée de
  chaque fonction et visibilité des branches sans accolades, diagnostics
  bilingues 122–125 et positions comparés au bootstrap dans la matrice locale
  du 5 octobre 2026 ; puis correction de la génération machine C++ des noms
  locaux réutilisés dans des portées distinctes, avec emplacements par déclaration
  et noms activés à la visite puis retirés après les destructions ; dix corpus
  bilingues exécutés et exemple d'intégration bilingue, dans le périmètre testé
  du 5 octobre 2026 ; cette correction ne migre pas le backend vers Gs++ ;
  puis recherche lexicale dans les espaces parents sans import, depuis les
  espaces imbriqués et les méthodes de classes : types nommés, alias, valeurs
  d'énumération, globales, callbacks, bases de classes et opérateurs ; masquage
  au premier niveau contenant un nom, douze corpus bilingues exécutés et
  2 077 refus différentiels dans la matrice locale du 5 octobre 2026 ;
  puis contexte lexical effectif de classe dans les méthodes, constructeurs,
  destructeurs et champs par défaut : masquage des fonctions parentes et
  importées, paramètres prioritaires et récepteurs explicites inchangés ;
  quatorze corpus bilingues exécutés, déclarations et types de retour choisis
  comparés au bootstrap, et 2 117 refus différentiels dans la matrice locale
  du 5 octobre 2026 ; le bootstrap C++ n'est pas modifié par cette tranche ;
  puis portées d'opérateurs dans les méthodes et la durée de vie : groupes
  mixtes de la classe avant les espaces parents, priorité du groupe associé
  à l'opérande gauche, accès privés/protégés, surcharges différentes pour un
  même champ évalué par plusieurs constructeurs et alternance surchargé/intrinsèque ;
  dix-sept corpus bilingues exécutés et 2 151 refus différentiels dans la matrice
  locale du 5 octobre 2026 ; cette extension des tests ne modifie pas les
  algorithmes du compilateur ni les diagnostics publics ;
  puis correction du contexte des types de conversions dans les champs par défaut,
  y compris agrégats et signatures de callbacks : portée du constructeur et alias
  masquant les espaces parents/importés, sans modifier le type déclaré du champ
  ni analyser les valeurs remplacées ; quinze corpus bilingues exécutés avec
  constructeurs effectivement choisis comparés au bootstrap, et 2 183 refus
  différentiels dans la matrice locale du 6 octobre 2026 ; le bootstrap et les
  diagnostics publics restent inchangés ;
  puis callbacks dans les champs par défaut partagés par plusieurs constructeurs :
  paramètres et signatures comparés au bootstrap, appels imbriqués, pointeurs,
  références, agrégats et valeurs remplacées ; dix corpus bilingues exécutés et
  un corpus bilingue uniquement sémantique pour les références constantes/volatiles,
  avec 2 211 refus différentiels dans la matrice locale du 6 octobre 2026 ; cette
  extension des tests ne modifie pas les algorithmes du compilateur ;
  puis arguments agrégés des callbacks et constructions : neuf corpus bilingues
  exécutés supplémentaires et un corpus uniquement sémantique pour les retours
  de callbacks par référence dans des signatures ; préserver les diagnostics
  internes des expressions au lieu de les remplacer par une incompatibilité du
  champ ; 2 255 refus différentiels dans la matrice locale du 6 octobre 2026,
  sans modifier le bootstrap ni ajouter de diagnostic public ;
  puis retours par référence des callbacks : lecture/adressage et retours de
  structures corrigés dans le backend C++, constance des appels propagée par le
  bootstrap et qualifications des champs/éléments adressés conservées par Gs++ ;
  quinze corpus bilingues exécutés avec callbacks C++ fournis par l'hôte,
  stockage et nombre d'appels vérifiés, et 2 285 refus différentiels locaux ;
  CTest Windows 5/5, GNU/Linux 6/6, solution et validation natives réussis,
  conformité 20/20 par chaîne et trois images reconstruites identiques ;
  aucun retour par référence de fonction ordinaire Gs++ n'est ajouté ;
  puis qualifications `volatile` et `constante volatile` des champs/éléments
  adressés via callbacks, directement ou par flèche, sans qualifier les callbacks
  stockés dans les champs ; dix corpus bilingues exécutés et huit refus bilingues
  supplémentaires, portant la matrice locale à 2 301 refus ; CTest Windows 5/5,
  GNU/Linux 6/6, solution et validation natives réussis, conformité 20/20 par
  chaîne ; trois images identiques et vérifiées, bootstrap et backend inchangés ;
  puis références vers les emplacements de pointeurs retournés par callbacks :
  vingt corpus bilingues exécutés avec cibles, données et nombre d'appels exacts ;
  distinguer le déréférencement d'un pointeur extrait d'un tableau des dimensions
  encore présentes ; quinze refus bilingues, 2 331 refus locaux, CTest Windows
  5/5, GNU/Linux 6/6, solution et validation MSBuild natives réussis ; conformité
  20/20 par chaîne, trois images identiques et vérifiées ; cette tranche est distincte de
  la consolidation `c58874b` poussée, signée et validée par sa CI à 2 301 refus ;
  puis références de callbacks paramétrés : vingt-deux corpus bilingues exécutés
  avec cibles, lectures, appels et argument exacts ; protéger le stockage constant
  dans les deux analyseurs, sans interdire l'appel ni la réaffectation des pointeurs
  de données qualifiés ; vingt-cinq refus bilingues, 2 381 refus locaux et huit
  refus unitaires du bootstrap ; CTest Windows 5/5, GNU/Linux 6/6, solution et
  validation MSBuild natives réussis, conformité 20/20 par chaîne et trois images
  identiques et vérifiées ; tranche locale distincte de la CI publiée, backend
  et ABI inchangés ;
  puis paramètres référencés des callbacks imbriqués : vingt-quatre corpus
  bilingues exécutés composant référence de callback, paramètre et retour par
  référence ; contrôler les adresses réelles, traces de mutation et courts-circuits ;
  vingt refus bilingues, 2 421 refus locaux, CTest Windows 5/5, GNU/Linux 6/6,
  solution et validation MSBuild natives réussis, conformité 20/20 par chaîne ;
  trois images identiques, inchangées et vérifiées ; aucune correction supplémentaire
  des analyseurs ou du backend, contrats publics et version alpha.10 conservés ;
  puis structures et pointeurs référencés des callbacks imbriqués : vingt-quatre
  corpus bilingues exécutés contrôlent identité, dispositions natives, copies
  indépendantes, champs/éléments, constructions et redirections, avec adresses
  et traces exactes ; vingt-quatre refus bilingues, 2 469 refus locaux,
  CTest Windows 5/5, GNU/Linux 6/6, solution et validation MSBuild natives
  réussis, conformité 20/20 par chaîne ; trois images identiques, inchangées
  et vérifiées ; couverture de comportements déjà implémentés, sans nouvelle
  correction des analyseurs ou du backend, contrats publics et alpha.10 conservés ;
  puis références dans les groupes mêlant méthodes et fonctions : vingt-deux
  corpus bilingues exécutés vérifient les déclarations sélectionnées, mutations,
  qualifications, conversions de classes et constructions ; vingt-quatre refus
  bilingues, 2 517 refus locaux, ambiguïtés et priorités des diagnostics couvertes ;
  CTest Windows 5/5, GNU/Linux 6/6, solution et validation MSBuild natives réussis,
  conformité 20/20 par chaîne ; trois images identiques, inchangées et vérifiées ;
  règles de surcharge actuelles confirmées, sans nouvelle correction, changement
  de contrats publics ou de version alpha.10 ;
  puis opérateurs mixtes recevant des références : vingt-quatre corpus bilingues
  exécutés contrôlent les déclarations, mutations, redirections de pointeurs,
  conversions de classes, constructions et expressions imbriquées ; une trace
  exportée vérifie ordre et nombre d'appels, ainsi que les courts-circuits intégrés ;
  vingt-quatre refus bilingues, 2 565 refus locaux, priorités des expressions,
  constructions et affectations couvertes ; CTest Windows 5/5, GNU/Linux 6/6,
  solution et validation MSBuild natives réussis, conformité 20/20 par chaîne ;
  trois images identiques, inchangées et vérifiées ; aucune nouvelle correction,
  contrats publics et version alpha.10 conservés ;
  puis opérateurs des initialiseurs agrégés : vingt-sept corpus bilingues exécutés
  contrôlent stockage, mutations, valeurs capturées dans l'ordre, éléments omis,
  copies, retours, callbacks et constructions ; les priorités de forme avant
  les feuilles des agrégats affectés et retournés sont corrigées dans l'analyseur
  Gs++ ; quarante-deux refus bilingues, 2 649 refus locaux, code/ligne/colonne
  et AST intact vérifiés ; CTest Windows 5/5, GNU/Linux 6/6, solution et
  validation MSBuild natives réussis, conformité 20/20 par chaîne ; trois
  images identiques de 451 599 octets et vérifiées ; backend C++, contrats
  publics et version alpha.10 conservés ;
  puis interfaces préparées en mémoire : une entrée d'analyse Gs++ avec alias
  anglais produit les prototypes externes et conserve la visibilité des membres ;
  vingt-deux corpus bilingues syntaxiques/sémantiques et six interfaces de types
  ou données, quatorze refus syntaxiques et quinze refus sémantiques bilingues ;
  capacités, sentinelles et AST intact vérifiés ; 2 679 refus sémantiques locaux,
  CTest Windows 5/5, GNU/Linux 6/6, solution et validation MSBuild natives réussis,
  conformité 20/20 par chaîne ; trois images identiques de 452 815 octets et
  vérifiées ; structures publiques, formats 1.0, ABI 1 et alpha.10 conservés ;
  lecture/expansion, assemblage avec les sources et origines d'inclusion restent
  à raccorder ;
  puis assemblage préparé et origines des unités : texte, AST et table
  d'origines sans sorties partielles, sémantique par unité avec isolation
  des imports directs/transitifs et restitution des diagnostics locaux ;
  treize corpus bilingues valides, douze refus sémantiques et six refus
  syntaxiques bilingues ; capacités, échecs d'allocation et origines altérées
  vérifiés ; 2 703 refus locaux, CTest Windows 5/5, GNU/Linux 6/6,
  solution et validation MSBuild natives réussis ; conformité 20/20 par chaîne ;
  trois images identiques de 466 447 octets, vérifiées, avec 81 exports ;
  anciens contrats, formats 1.0, ABI 1 et alpha.10 conservés ; la normalisation
  prototype/définition et le raccordement du flux d'inclusions restent ouverts ;
  puis normalisation préparée des déclarations libres : entrée additive pour
  fonctions/opérateurs libres, globales et alias racines, avec types/noms exacts,
  définition préférée à la place de la première déclaration et origines conservées ;
  vingt-deux corpus bilingues valides, trente refus de normalisation, un refus
  syntaxique bilingue et huit refus sémantiques bilingues ; 2 719 refus sémantiques
  locaux, capacités et chaque échec d'allocation vérifiés ; CTest Windows 5/5,
  GNU/Linux 6/6, solution et validation MSBuild natives réussis ; conformité 20/20
  par chaîne ; trois images identiques de 490 623 octets, vérifiées, avec 83 exports ;
  membres/groupes mixtes et flux d'inclusions restent ouverts ;
- **EN COURS** : compléter la matrice des conversions et qualifications et les autres
  familles sémantiques, notamment les contextes des constructions et les
  interactions de priorité entre passes non encore testées, dont les
  combinaisons non couvertes d'initialiseurs globaux, les qualifications et
  signatures locales non représentées dans la matrice, les contextes non
  couverts de bases/champs et les combinaisons de conversions non encore
  couvertes ;
  raccorder les données émises aux futurs écrivains
  d’objets auto-hébergés dans le jalon backend ;
- **EN COURS** : comparer systématiquement les résultats au bootstrap C++.

Le contrat et les preuves intermédiaires du lexeur et de l’AST sont décrits dans
[`FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md`](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md).

Le passage au jalon 0.28 n'est pas encore validé. Les prochaines tranches
portent sur les interactions sémantiques restantes, les autres combinaisons de
constructions, la normalisation des membres/groupes mixtes dans l'assemblage
préparé et le raccordement des inclusions avec leurs origines internes, puis sur
la consolidation de la conformité et des
benchmarks du frontend 0.27. Le nombre de corpus réussis n'est ni un pourcentage
d'achèvement ni un déclencheur automatique de changement de version.
`VERSION` reste à `0.27.0-alpha.10` ; aucune date de sortie 0.28 n'est fixée ici.
L'ouverture d'une `0.28.0-alpha.1` marquera le début du jalon suivant, pas
l'achèvement du backend ni la livraison d'une `0.28.0` finale.

#### Inclusions et utilisation d'espaces — ajout local du 4 octobre 2026

Le bootstrap fournit maintenant `#inclure` / `#include` entre guillemets,
les inclusions imbriquées, `#pragma once` et les positions de diagnostic du
fichier inclus. `utilisant espace N;` / `using namespace N;` importe les noms
au niveau global ou d'un espace de noms, après la directive : types, alias,
énumérations, globales et groupes de fonctions. Les imports ne remplacent ni
la sélection des sources ni l'édition de liens des projets XML.

**VALIDÉ pour le bootstrap et les analyseurs auto-hébergés dans le périmètre testé.**
Le portage du 5 octobre aligne l'AST et l'analyse sémantique auto-hébergés,
y compris les alias, groupes de surcharges et opérateurs importés. La lecture
et l'expansion des fichiers inclus restent confiées au bootstrap hôte.
Les utilisations dans un bloc, les autres déclarations `using`, macros,
conditions de préprocesseur, chemins `<...>` et options `-I` ne sont pas
implémentés dans cette tranche. La
[spécification](SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md#inclusion-textuelle-et-utilisation-despaces-de-noms)
décrit le contrat actuel. Ce changement ne clôt pas 0.27 et ne constitue pas
une nouvelle release alpha.10 ni l'ouverture de 0.28.

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
- introduire GsBuild (nom proposé, commande `gsbuild`) et y migrer
  l'orchestration des projets et solutions, l'archivage et la liaison ;
  recentrer `gsppc` sur la compilation des sources/interfaces Gs++ en objets,
  selon la décision du 5 octobre 2026 ;
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
- [ ] séparation `gsppc` / GsBuild livrée et testée sous Windows et GNU/Linux,
  avec exemples, intégrations et paquets utilisant la nouvelle commande ;
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
