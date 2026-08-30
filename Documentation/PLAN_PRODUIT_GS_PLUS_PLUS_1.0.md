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
| Signature de liaison | `GsAbi:x64-ms-v1` |
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
13/13 cas de conformité sur chaque chaîne et la preuve QEMU/OVMF. Le rapport
est consigné dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.24.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.24.0.md).

### Gs++ 0.25 — initialisation et durée de vie — terminé

- valeurs par défaut des champs de classes ;
- délégation `soi`/`this` entre constructeurs sans cycle ;
- arguments uniformes réévalués pour chaque élément de tableau d’objets ;
- exclusion explicite des objets de classe globaux ;
- tests RAII, diagnostics et conformité étendue à 16 exigences.

La 0.25.0 réussit 5/5 tests sous MSVC, 6/6 sous GNU/WSL, 16/16 cas de
conformité sur chaque chaîne, les benchmarks smoke et la preuve QEMU/OVMF. Le
rapport est consigné dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.25.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.25.0.md).

### Gs++ 0.26 — bibliothèque hébergée — terminé

- compléter les chaînes, conteneurs, fichiers, chemins et diagnostics ;
- garantir le profil système sans dépendance hébergée cachée ;
- fournir les structures nécessaires à la migration du compilateur.

La 0.26.0 ajoute cinq imports d’hôte explicites, les chaînes UTF-8,
vecteurs dynamiques, table de symboles, arène stable, chemins et fichiers
alloués. Elle réussit 5/5 tests MSVC, 6/6 tests GNU/WSL, 18/18 exigences sur
chaque chaîne, les benchmarks smoke et la preuve QEMU/OVMF. Le rapport est
[`Validations/VALIDATION-GS-PLUS-PLUS-0.26.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.26.0.md).

### Gs++ 0.27 — frontend auto-hébergé

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
  littéraux et diagnostics de types ;
- **EN COURS** : compléter le typage récursif, les qualifications, les
  conversions implicites et la résolution globale ;
- **EN COURS** : comparer systématiquement les résultats au bootstrap C++.

Le contrat et les preuves intermédiaires du lexeur et de l’AST sont décrits dans
[`FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md`](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md).

### Gs++ 0.28 — backend et chaîne auto-hébergés

- achever l’analyse sémantique ;
- migrer la génération x86-64 ;
- migrer les écrivains GsObj/GsA/GsE et l’éditeur de liens ;
- migrer l’orchestration de projets ;
- obtenir les générations N+1 et N+2 fonctionnelles.

### Gs++ 0.29 — durcissement produit

- déterminisme et reproductibilité ;
- corpus malformés et fuzzing ;
- conformité complète Windows/GNU ;
- installation, SDK et paquets locaux ;
- documentation finale ;
- validation UEFI avec la toolchain produite.

### Gs++ 1.0.0 — sortie produit

La version 1.0.0 n’est autorisée que lorsque tous les critères de sortie
ci-dessous sont satisfaits.

## Critères de sortie 1.0

- [ ] périmètre du langage 1.0 figé et documenté ;
- [ ] spécifications GsObj, GsA et GsE 1.0 complètes ;
- [ ] ABI 1 documentée et couverte par des tests inter-unités ;
- [ ] bibliothèques système et hébergée suffisantes pour le compilateur ;
- [ ] compilateur principalement maintenu en Gs++ ;
- [ ] génération N+1 capable de produire une génération N+2 fonctionnelle ;
- [ ] comparaison N+1/N+2 conforme à la règle de reproductibilité ;
- [ ] suite de conformité entièrement réussie sous MSVC et GNU ;
- [ ] lecteurs binaires durcis contre les fichiers malformés ;
- [ ] installation et paquets locaux vérifiés ;
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

Les sous-systèmes Linux/Windows, la plateforme Unreal Engine Sanctuaire SE, le
SDK applicatif et les services avancés restent documentés mais non développés
activement avant la sortie produit de Gs++.

## Règle de suivi

Chaque jalon doit conserver les statuts suivants sans les confondre :

- `VALIDÉ` : implémenté et couvert par une preuve exécutable actuelle ;
- `PARTIEL` : présent mais ne satisfaisant pas encore son contrat final ;
- `PRÉVU` : architecture ou fonction non encore démontrée ;
- `OBSOLÈTE` : convention rejetée ou remplacée.

La feuille de route et les synthèses doivent être mises à jour après chaque
jalon, mais ce document reste la source de vérité pour la priorité produit et
les critères de sortie de Gs++ 1.0.
