# Protocole de benchmark Gs++ 0.25.0

## Statut

**PROTOCOLE REPRODUCTIBLE — aucune supériorité de performance revendiquée.**

Ce document définit comment mesurer Gs++ 0.25.0 sans confondre la version du
compilateur avec les formats GsObj/GsA/GsE 1.0 ou l'ABI 1. Il remplace les
propositions externes qui inventaient une syntaxe, des extensions ou des
options de ligne de commande.

Le pilote de référence est
[`Benchmarks/gspp_benchmark.py`](../Benchmarks/gspp_benchmark.py). Ses enveloppes
Windows et GNU/Linux sont documentées dans
[`Benchmarks/README.md`](../Benchmarks/README.md).

## Questions mesurées

Le banc répond uniquement aux questions suivantes :

1. quel est le temps mur, la mémoire résidente maximale et la taille des
   artefacts pour des chaînes GsObj, GsA et GsE réelles ;
2. quelle variation observe-t-on entre un répertoire d'artefacts neuf et une
   reconstruction immédiate avec les artefacts conservés ;
3. quel est le coût d'un plan de reconstruction ciblé après une modification
   neutre d'une unité feuille, d'une interface partagée ou d'une unité centrale ;
4. quelle part du pipeline visible revient à la compilation GsObj, à la
   construction GsA, à la liaison GsE et au chargement/exécution ;
5. quelle variance doit être prise en compte avant toute conclusion.

Le banc ne prétend pas mesurer séparément le lexing, le parsing, l'analyse
sémantique et la génération de code. Ces étapes sont actuellement réunies dans
un même processus `gsppc` et nécessitent une instrumentation interne future.

## Corpus validés

Les catégories S/M/L sont relatives au corpus actuel. Elles ne constituent pas
une norme de taille générale pour les programmes Gs++.

| Scénario | Classe | Entrées actuelles | Interface | Production |
| --- | ---: | ---: | --- | --- |
| `s_source_gsobj` | S | 1 fichier, 27 lignes, 548 octets | non | GsObj |
| `m_monolithic_gse` | M | 1 fichier, 52 lignes, 1 194 octets | non | GsE exécuté, retour 10 |
| `m_separate_gse` | M | 3 fichiers, 64 lignes, 1 489 octets | oui | 2 GsObj, GsE exécuté, retour 10 |
| `l_system_library` | L | 5 fichiers, 495 lignes, 17 491 octets | oui | GsA par projet GsPj |

Les empreintes SHA-256 et les tailles sont recalculées dans `session.json` à
chaque campagne. Une évolution du corpus reste donc détectable.

## Conditions

### `cold_artifacts`

Chaque répétition reçoit un nouveau répertoire de travail et aucun artefact
GsObj/GsA/GsE préexistant. Le cache du système d'exploitation n'est pas vidé :
le terme décrit l'état des artefacts, pas un démarrage matériel à froid.

### `warm_artifacts`

Une construction de référence non mesurée est suivie de la même chaîne avec
les artefacts conservés. Gs++ 0.25.0 n'exposant pas de cache désactivable, cette
condition ne doit jamais être présentée comme la mesure d'un cache interne.

### Modifications ciblées

Le pilote construit une copie de référence, ajoute un commentaire neutre dans
la copie ciblée, puis mesure les étapes prévues :

- `leaf_edit` : unité d'application ou source périphérique ;
- `interface_edit` : interface `.HGsPP` partagée ;
- `central_edit` : implémentation centrale du scénario.

Dans le scénario séparé, les étapes touchées sont sélectionnées explicitement,
par exemple recompilation de l'objet principal puis nouvelle liaison. Cela
mesure ce plan de reconstruction ; cela ne prouve pas que `gsppc` possède un
graphe d'invalidation automatique. Le scénario GsPj mesure au contraire
l'orchestrateur de projet complet après modification.

## Mesures et validation

Chaque processus fournit :

- temps mur monotone en nanosecondes et millisecondes ;
- pic de mémoire résidente, via l'API de processus Windows ou `/usr/bin/time`
  sous GNU/Linux ;
- code de sortie et journaux standard/erreur ;
- taille cumulée des artefacts produits.

Chaque échantillon est rejeté si une commande échoue, si une signature
`GSOBJ:0`, `GSA:0` ou `GSE:0` est absente, ou si les scénarios exécutables ne
renvoient pas le résultat fonctionnel attendu. Une mesure rapide mais
fonctionnellement incorrecte n'est jamais conservée comme succès.

L'affinité CPU est facultative et doit être identique entre campagnes
comparées. Le mode `full` utilise 30 répétitions et 3 échauffements. Les
résultats présentent la médiane, les quartiles, l'IQR, la MAD et un intervalle
de confiance bootstrap à 95 % de la médiane. Les cinq mesures du mode `pilot`
servent uniquement à estimer la dispersion ; elles ne suffisent pas à annoncer
une supériorité.

## Critères de conclusion

Aucun seuil universel tel que « 10 % du temps complet » n'est imposé. Avant une
comparaison, il faut définir :

1. la métrique principale ;
2. le scénario et la plateforme concernés ;
3. l'effet minimal intéressant, absolu et relatif ;
4. le budget de variance acceptable ;
5. l'intervalle de confiance utilisé.

Une amélioration n'est déclarée que si l'intervalle de confiance de la
différence reste du côté de l'effet minimal prédéfini. Une équivalence exige
une marge d'équivalence prédéfinie et un test adapté. Dans les autres cas, la
conclusion correcte est « inconclusif », et non « identique ».

Les valeurs atypiques restent dans les résultats bruts. Une exclusion n'est
permise qu'en présence d'un événement externe documenté et selon une règle
définie avant l'analyse.

## Extension vers C++20 et Rust

Une comparaison interlangage n'est recevable qu'après l'ajout d'adaptateurs
séparés respectant les conditions suivantes :

- même algorithme observable et mêmes valeurs de retour ;
- volume et structure de déclarations comparables, pas seulement le même
  nombre de lignes ;
- niveaux d'optimisation et informations de débogage explicitement alignés ;
- versions exactes des compilateurs et du système de construction enregistrées ;
- scénarios C++ avec en-têtes classiques et modules C++20 distingués ;
- scénarios Rust distinguant modules internes et crates séparées ;
- mesure séparée de la compilation complète, de la reconstruction ciblée, de
  la liaison et de l'exécution ;
- validation fonctionnelle commune avant comparaison des temps.

Le pilote actuel ne contient aucun adaptateur C++ ou Rust, afin d'éviter une
fausse équivalence. Leur ajout devra produire le même schéma JSON et conserver
les résultats bruts par outil.

## Menaces à la validité

- caches de fichiers et antivirus non neutralisés ;
- changement de fréquence, température ou politique d'alimentation ;
- différences entre MSVC/Windows et GNU/WSL ;
- corpus encore réduit par rapport à une application industrielle ;
- mémoire des processus enfants non agrégée sur toutes les plateformes ;
- instrumentation externe incapable de séparer les phases internes de
  `gsppc` ;
- reconstruction ciblée manuelle différente d'un futur moteur incrémental.

Les résultats doivent toujours être accompagnés de `session.json`,
`results.jsonl` et `status.json`. Un tableau résumé seul ne constitue pas une
preuve reproductible.

## Validation fonctionnelle du pilote

La campagne historique du 16 août 2026 a exécuté le pilote 1.0.0 avec les outils déclarant
exactement `Gs++ Compiler 0.23.0` et `Chargeur GsE 0.23.0` :

- Windows/MSVC : 4/4 scénarios `smoke`, puis 16/16 couples
  scénario/condition avec une répétition de contrôle ;
- GNU/WSL : 4/4 scénarios `smoke`, puis 16/16 couples scénario/condition avec
  une répétition de contrôle.

Les signatures GsObj/GsA/GsE et les retours fonctionnels ont été vérifiés pour
chaque échantillon. Les sessions correspondantes sont sous
`Construction/Benchmarks/GsPlusPlus/`. Ces exécutions valident le fonctionnement
du banc ; une répétition unique ne constitue pas une mesure de performance et
ne permet aucune comparaison entre Windows et WSL.
