# Feuille de route prioritaire de Gs++

## Principe de versionnement

Le compilateur Gs++ possède désormais un cycle de versions indépendant de
Sanctuaire SE. La version 0.10.2 de Sanctuaire SE reste la référence UEFI
validée et sert de test d’intégration réel. Aucun nouvel ordonnanceur, processus
ou pilote n’est ajouté au système tant que le cœur du langage nécessaire à ces
composants n’est pas stabilisé.

## Jalons

Le commit signé [`039bd3f`](https://github.com/Galactic-Shrine/GsPlusPlus/commit/039bd3f8887c9d8bc826e000d3b8099609d52850)
publie les tranches jusqu'à la normalisation des déclarations libres, à 2 719
refus sémantiques. Sa signature est vérifiée et sa
[CI à trois chaînes](https://github.com/Galactic-Shrine/GsPlusPlus/actions/runs/37485258264)
réussit. Les mentions locales des jalons historiques décrivent leur état au
moment de validation ; aucune nouvelle release ni mise à jour des paquets
alpha.10 n'est réalisée.

### Ajout local du 6 octobre 2026 — catalogue de fichiers du produit

**VALIDÉ dans le périmètre testé sur les trois chaînes.** La lecture/résolution
est extraite des tests vers `CatalogueInclusions` dans `gspp_compiler` :
instantané propriétaire, alias canoniques, noms virtuels séparés des chemins,
indices communs, copies interdites, déplacements sûrs, parcours itératif et
bornes configurables. `PreparerSourceAvecOrigines` appelle l'export Gs++ pour
mesurer puis publier texte et origines ; les matrices existantes emploient
cette API du produit jusqu'à la sémantique avec origines. Durée de vie,
instantané après modification du disque, limites, contrats ABI et graphe
de 512 niveaux testés. CTest Windows 5/5, GNU/Linux 6/6, validation native VS
2026 réussis ; conformité 20/20 par chaîne, total sémantique 2 751 inchangé.
Trois images identiques vérifiées de 542 527 octets, 99 exports et deux imports.
Lecture du graphe encore anticipée et découverte des chemins par le lexeur
C++ : la lecture à la demande et l'intégration au pilote par défaut restent
ouvertes. Alpha.10, formats 1.0, ABI 1 et parcours habituel de `gsppc` inchangés.
Travail local non commité/non poussé, sans nouvelle release ni frontend complet.
Voir le [bilan de raccordement](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#catalogue-hôte-réutilisable-et-préparation-du-produit--6-octobre-2026).

### Ajout local du 6 octobre 2026 — expansion des inclusions avec origines

**VALIDÉ dans le périmètre testé sur les trois chaînes.** Le frontend développe
`#inclure` / `#include` et `#pragma once` sur le catalogue lu/résolu par l'hôte :
identités canoniques, diagnostics distincts des alias, lexage complet avant les
directives, protection once au point de rencontre, cycles et profondeur 128/129.
Contrats dédiés de 32/32/128/48 octets, deux exports français/anglais, pile
itérative et arène libérée ; texte et origines transactionnels, sans nouvel
import d'hôte. Les deux matrices d'inclusions utilisent maintenant cette
expansion ; la sélection C++ reste l'oracle. 41 corpus français/anglais,
15 valides et 26 refus bilingues sur les dossiers testés ; variante de casse
adaptée au système de fichiers réel. Diagnostics, capacités, sentinelles,
allocations, états réinitialisés et catalogue/liens invalides vérifiés.
CTest Windows 5/5, GNU/Linux 6/6 et validation native VS 2026 réussis ;
conformité 20/20 par chaîne ; total **2 751 refus sémantiques** inchangé.
Ordre des entrées CMake/natif synchronisé ; trois images identiques et
vérifiées de 542 527 octets, 99 exports et deux imports. Alpha.10, formats 1.0
et ABI 1 conservés. Lecture et résolution restent côté hôte ; intégrer ce
catalogue et ce chemin préparé au pilote de compilation de fichiers de `gsppc`
reste ouvert. Travail local non commité/non poussé, sans release ni validation
d'un frontend 0.27 complet.
Voir le [bilan d'expansion](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#expansion-des-inclusions-avec-origines--6-octobre-2026).

### Ajout local du 6 octobre 2026 — préparation lexicale avec origines

**VALIDÉ dans le périmètre testé sur les trois chaînes.** Le frontend construit
désormais le texte développé et les origines à partir des fragments originaux
sélectionnés par l'hôte. `PreparerDeclarationsAvecOrigines` et son alias anglais
utilisent une requête additive de 112 octets, des fragments de 40 octets et un
résultat de 48 octets. Les lexèmes/chaînes sont conservés sans réencodage, avec
BOM optionnel, LF/CRLF et EOF. Lexage avant publication, capacités exactes,
deux sorties transactionnelles, diagnostics lexicaux originaux, refus des
directives restantes et absence d'allocation vérifiés. Les deux matrices
d'inclusions utilisent maintenant cette préparation ; 40 lexèmes, quatre modes
et 21 fragments refusés, arguments, refus tardifs et dépassements testés.
Total inchangé : **2 751 refus sémantiques différentiels**.
CTest Windows 5/5, GNU/Linux 6/6 et validation native VS 2026 réussis ;
conformité 20/20 par chaîne ; trois images identiques et vérifiées de
524 319 octets, 97 exports et deux imports. Formats 1.0, ABI 1 et alpha.10
conservés. Lecture, résolution des chemins, expansion des inclusions, once et
cycles restent côté hôte ; le pilote de compilation de fichiers de `gsppc`
n'est pas remplacé. Travail local non commité/non poussé, sans nouvelle release.
Voir le [bilan de préparation](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#préparation-lexicale-avec-origines--6-octobre-2026).

### Ajout local du 6 octobre 2026 — assemblage des inclusions et diagnostics originaux

**VALIDÉ dans le périmètre testé sur les trois chaînes.** Les nouvelles entrées
raccordent les origines des jetons aux assemblages brut/normalisé et à la
sémantique par unité : modes mixtes, déclaration retenue et diagnostics dans
le fichier original. Les inclusions restent dans leur unité ; les imports
ne fuient pas vers une unité séparée. Les contrats existants sont conservés,
avec deux requêtes additives de 40 octets et une table de 16 octets par unité.
Douze corpus bilingues valides, six conflits bilingues de normalisation
(12 refus), trois refus syntaxiques et sept refus sémantiques bilingues passent ;
total **2 751 refus sémantiques**, distinct des refus de syntaxe/normalisation.
Capacités, sorties transactionnelles des assemblages, contrats sémantiques
historiques, échecs d'allocation et tables/textes altérés vérifiés.
CTest Windows 5/5, GNU/Linux 6/6, solution et validation MSBuild natives réussis ;
conformité 20/20 par chaîne ; trois images identiques de 516 863 octets,
vérifiées, 95 exports et deux imports. Formats 1.0, ABI 1 et alpha.10 conservés.
Lecture/expansion restent côté hôte et le chemin de compilation de fichiers
de `gsppc` n'est pas encore remplacé. Tranche locale non commitée/non poussée,
distincte de la CI de `039bd3f` et des paquets publiés ; aucun frontend 0.27
complet ni nouvelle release revendiqués.
Voir le [bilan du raccordement](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#assemblage-des-inclusions-et-diagnostics-originaux--6-octobre-2026).

### Ajout local du 6 octobre 2026 — origines des inclusions préparées

**VALIDÉ dans le périmètre testé sur les trois chaînes.** Une nouvelle entrée
analyse une unité déjà développée par l'hôte avec une origine pour chaque jeton,
fin comprise : modes source/interface au jeton décisif, priorité du mode global
d'interface et restitution des diagnostics dans leur fichier d'origine. L'AST
garde ses positions synthétiques ; un export de localisation permet également
de traduire les diagnostics sémantiques. Les nouveaux contrats de 40/56 octets
ne modifient pas les anciens. Vingt corpus bilingues syntaxiquement valides,
sept refus syntaxiques et trois refus sémantiques bilingues passent, avec
vraies inclusions, chemins relatifs/Unicode, `#pragma once`, modes mixtes,
chaque jeton/EOF, capacités et échecs d'allocation. Total : **2 737 refus
sémantiques**, distinct des refus syntaxiques et de normalisation.
CTest Windows 5/5, GNU/Linux 6/6, solution et validation MSBuild natives réussis ;
conformité 20/20 par chaîne ; trois images identiques de 505 599 octets,
vérifiées, avec 87 exports et deux imports. Formats 1.0, ABI 1 et alpha.10 conservés.
Lecture et expansion restent côté hôte ; le raccordement de ces origines à
l'assemblage/normalisation multi-unités reste ouvert. Tranche locale non
commitée/non poussée, distincte de la CI de `039bd3f` et des paquets publiés ;
aucune compilation autonome de fichiers ni clôture du frontend 0.27 revendiquée.
Voir le [bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#origines-des-inclusions-préparées--6-octobre-2026).

### Ajout local du 6 octobre 2026 — normalisation des membres et groupes mixtes

**VALIDÉ dans le périmètre testé sur les trois chaînes.** L'entrée préparée
normalise maintenant méthodes, constructeurs, destructeurs et opérateurs membres
avec les fonctions libres, en tenant compte du récepteur implicite `Classe&`.
Les définitions sont préférées dans l'ordre de première déclaration des fonctions ;
types/champs sont placés avant celles-ci et les parents réindexés sans déplacement
du texte ni des origines. Types/classes répétés non fusionnés, assemblage brut
et contrats publics conservés. Matrice : 39 corpus bilingues valides, 64 refus
de normalisation, un refus syntaxique et 14 refus sémantiques bilingues,
soit **2 731 refus sémantiques**. CTest Windows 5/5, GNU/Linux 6/6,
solution et validation MSBuild natives réussis ; conformité 20/20 par chaîne ;
trois images identiques de 498 479 octets, vérifiées, avec 83 exports.
Formats 1.0, ABI 1 et alpha.10 conservés. Tranche locale non commitée/non poussée,
distincte de la CI de `039bd3f` à 2 719 refus et des paquets publiés.
Le flux d'inclusions et ses origines internes restent à raccorder ; aucune
compilation autonome de fichiers ni validation du frontend 0.27 complet n'est revendiquée.
Voir le [bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#normalisation-des-membres-et-groupes-mixtes--6-octobre-2026).

### Ajout local du 6 octobre 2026 — normalisation préparée des déclarations libres

**VALIDÉ dans le périmètre testé sur les trois chaînes.** Une entrée additive
normalise les fonctions/opérateurs libres, globales et alias racines préparés :
types et noms exacts avant résolution des alias, définitions préférées aux
prototypes à la place de la première déclaration, origines conservées.
Vingt-deux corpus bilingues valides, trente refus différentiels de normalisation,
un refus syntaxique bilingue et huit refus sémantiques bilingues ; capacités,
échecs d'allocation et priorités après fusion vérifiés. La matrice sémantique
atteint **2 719 refus**, sans additionner les trente conflits de normalisation.
CTest Windows 5/5, GNU/Linux 6/6, solution et validation MSBuild natives réussis ;
conformité 20/20 par chaîne ; trois images identiques de 490 623 octets, vérifiées,
avec 83 exports. Anciens contrats, formats 1.0, ABI 1 et alpha.10 conservés.
Membres/groupes mixtes et flux d'inclusions restent ouverts. Tranche locale
non commitée/non poussée, distincte de la CI de `c58874b` et des paquets publiés.
Les preuves figurent dans le
[bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#normalisation-préparée-des-déclarations-libres--6-octobre-2026).

### Ajout local du 6 octobre 2026 — assemblage préparé et origines des unités

**VALIDÉ dans le périmètre testé sur les trois chaînes.** Le frontend Gs++
assemble plusieurs interfaces/sources préparées en texte, AST et table
d'origines. Une nouvelle entrée sémantique isole les imports directs/transitifs
par unité et restitue les diagnostics locaux. Treize corpus bilingues valides,
douze refus sémantiques et six refus syntaxiques bilingues ; sorties sans
publication partielle, échecs d'allocation et origines altérées vérifiés.
La matrice atteint **2 703 refus**. CTest Windows 5/5, GNU/Linux 6/6,
solution et validation MSBuild natives réussis ; conformité 20/20 par chaîne ;
trois images identiques de 466 447 octets, vérifiées, avec 81 exports.
Anciens contrats, formats 1.0, ABI 1 et alpha.10 conservés.
La normalisation prototype/définition et le raccordement des inclusions
restent ouverts ; aucune compilation autonome de fichiers n'est revendiquée.
Tranche locale non commitée/non poussée, distincte de la CI de `c58874b`
à 2 301 refus et des paquets publiés. Les preuves figurent dans le
[bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#assemblage-préparé-et-origines-des-unités--6-octobre-2026).

### Ajout local du 6 octobre 2026 — interfaces préparées en mémoire

**VALIDÉ dans le périmètre testé sur les trois chaînes.** L'analyseur Gs++
dispose maintenant d'une entrée d'interface réelle, avec prototypes externes,
visibilité des membres et le contrat mémoire existant. Vingt-deux corpus
bilingues syntaxiques/sémantiques, six interfaces de types/données, quatorze
refus syntaxiques et quinze refus sémantiques bilingues passent ; capacités,
sentinelles, AST intact et alternance des modes sont vérifiés. La matrice
sémantique atteint **2 679 refus**. CTest Windows 5/5, GNU/Linux 6/6,
solution et validation MSBuild natives réussis ; conformité 20/20 par chaîne ;
trois images identiques de 452 815 octets et vérifiées.
Formats 1.0, ABI 1, alpha.10 et structures publiques conservés ; deux exports
ajoutés. Lecture/expansion, assemblage avec les sources et origines des
inclusions restent à raccorder. Tranche locale non commitée/non poussée,
distincte de la CI de `c58874b` à 2 301 refus et des paquets publiés.
Les preuves figurent dans le
[bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#interfaces-préparées-en-mémoire--6-octobre-2026).

### Ajout local du 6 octobre 2026 — opérateurs des initialiseurs agrégés

**VALIDÉ dans le périmètre testé sur les trois chaînes.** Vingt-sept corpus
bilingues exécutés contrôlent formes imbriquées, stockage, mutations, copies,
retours, callbacks et constructions. Deux écarts de l'analyseur Gs++ sont corrigés :
la forme des agrégats affectés et retournés est contrôlée avant leurs feuilles.
Quarante-deux refus bilingues portent la matrice locale à **2 649**, avec code,
ligne, colonne et AST intact comparés au bootstrap. CTest Windows 5/5,
GNU/Linux 6/6, solution et validation MSBuild natives réussis ; conformité
20/20 par chaîne ; trois images identiques de 451 599 octets et vérifiées.
Version alpha.10 et contrats publics conservés ; backend C++ inchangé.
Tranche locale non commitée/non poussée, distincte de la CI de `c58874b`
à 2 301 refus et des paquets publiés.
Les preuves figurent dans le
[bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#opérateurs-des-initialiseurs-agrégés--6-octobre-2026).

### Ajout local du 6 octobre 2026 — références des opérateurs mixtes et constructions

**VALIDÉ dans le périmètre testé sur les trois chaînes.** Vingt-quatre corpus
bilingues exécutés vérifient les opérateurs recevant des références : mutations,
redirections de pointeurs, qualifications, conversions de classes et constructions.
Une trace exportée contrôle l'ordre des expressions imbriquées et successives,
ainsi que l'absence d'appels dans les courts-circuits logiques intégrés.
Vingt-quatre refus bilingues portent la matrice locale à **2 565**, avec
ambiguïtés, qualifications, accès privés et priorités des diagnostics vérifiés.
CTest Windows 5/5, GNU/Linux 6/6, solution et validation MSBuild natives réussis ;
conformité 20/20 par chaîne ; trois images identiques, inchangées et vérifiées.
Aucune nouvelle correction nécessaire, version alpha.10 et contrats publics
conservés. Tranche locale non commitée/non poussée, distincte de la CI de
`c58874b` à 2 301 refus et des paquets publiés.
Les preuves figurent dans le
[bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#références-des-opérateurs-mixtes-et-constructions--6-octobre-2026).

### Ajout local du 6 octobre 2026 — références des groupes mixtes et constructions

**VALIDÉ dans le périmètre testé sur les trois chaînes.** Vingt-deux corpus
bilingues exécutés vérifient la sélection entre méthodes et fonctions libres,
qualifications des références, conversions de classes dérivées vers une base,
récepteurs constants/volatiles, alias et mutations. Les appels dans les champs
par défaut, bases, membres et délégations sont également couverts. Vingt-quatre
refus bilingues portent la matrice locale à **2 517**, avec les ambiguïtés et
priorités des diagnostics vérifiées. CTest Windows 5/5, GNU/Linux 6/6, solution
et validation MSBuild natives réussis ; conformité 20/20 par chaîne ; trois
images identiques, inchangées et vérifiées. Aucune nouvelle correction nécessaire,
version alpha.10 et contrats publics conservés. Tranche locale non commitée/non
poussée, distincte de la CI de `c58874b` à 2 301 refus et des paquets publiés.
Les preuves figurent dans le
[bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#références-des-groupes-mixtes-et-constructions--6-octobre-2026).

### Ajout local du 6 octobre 2026 — structures et pointeurs référencés des callbacks imbriqués

**VALIDÉ dans le périmètre testé sur les trois chaînes.** Vingt-quatre corpus
bilingues exécutés vérifient les structures transmises sans copie, copies par
valeur indépendantes, champs/éléments, constructions et emplacements de pointeurs
redirigés. Les adresses réelles, dispositions natives et traces avant/après sont
contrôlées, avec protection des données constantes. Vingt-quatre refus bilingues
portent la matrice locale à **2 469**. CTest Windows 5/5, GNU/Linux 6/6, solution
et validation MSBuild natives réussis ; conformité 20/20 par chaîne ; trois
images identiques, inchangées et vérifiées. Aucune nouvelle correction des
analyseurs ou du backend nécessaire ; version alpha.10 et contrats publics
conservés. Tranche locale non commitée/non poussée, distincte de la CI de
`c58874b` à 2 301 refus et des paquets publiés.
Les preuves figurent dans le
[bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#références-de-structures-et-de-pointeurs-dans-les-callbacks-imbriqués--6-octobre-2026).

### Ajout local du 6 octobre 2026 — arguments référencés des callbacks imbriqués

**VALIDÉ dans le périmètre testé sur les trois chaînes.** Vingt-quatre
corpus bilingues exécutés composent références de callbacks, paramètres référencés
et retours référencés. Les adresses et traces de mutation, appels imbriqués,
constructions et courts-circuits sont vérifiés. Vingt refus bilingues portent
la matrice locale à **2 421**. CTest Windows 5/5, GNU/Linux 6/6, solution et
validation MSBuild natives réussis ; conformité 20/20 par chaîne ; trois images
identiques, inchangées et vérifiées. Cette couverture ne modifie pas les analyseurs,
le backend ni les contrats publics ; version alpha.10 conservée, tranche non
commitée/non poussée, distincte de la CI de `c58874b` à 2 301 refus.
Les preuves figurent dans le
[bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#arguments-référencés-des-callbacks-imbriqués--6-octobre-2026).

### Ajout local du 6 octobre 2026 — références de callbacks paramétrés et stockage constant

**VALIDÉ dans le périmètre testé sur les trois chaînes.** Vingt-deux
corpus bilingues exécutés vérifient les appels paramétrés, cibles, lectures et
copies indépendantes. Les deux analyseurs protègent le stockage des callbacks
constants, sans interdire l'appel ni la réaffectation des pointeurs vers des
données constantes. Vingt-cinq refus bilingues portent la matrice locale à
**2 381** ; huit refus unitaires protègent également le bootstrap. CTest Windows
5/5, GNU/Linux 6/6, solution et validation MSBuild natives réussis, conformité
20/20 par chaîne et trois images identiques acceptées par le vérificateur. Backend,
AST public, formats et ABI inchangés, version alpha.10 conservée ; tranche non
commitée et non poussée, distincte de `c58874b` et de sa CI à 2 301 refus.
Les preuves figurent dans le
[bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#références-de-callbacks-paramétrés-et-stockage-constant--6-octobre-2026).

### Ajout local du 6 octobre 2026 — références de pointeurs et cibles de tableaux

**VALIDÉ dans le périmètre testé sur les trois chaînes.** Vingt corpus
bilingues exécutés vérifient les références vers des emplacements de pointeurs,
leurs cibles et les données pointées. Le frontend distingue un déréférencement
d'un véritable tableau ou sous-tableau, sans changer les diagnostics ni le
bootstrap/backend. Quinze refus bilingues portent la matrice locale à **2 331**.
CTest Windows 5/5, GNU/Linux 6/6, solution et validation MSBuild natives réussis ;
conformité 20/20 par chaîne et trois images identiques acceptées par le vérificateur.
La consolidation précédente `c58874b` est signée, poussée et sa CI réussit sur
les trois chaînes avec 2 301 refus ; cette nouvelle tranche n'y est pas incluse.
Version alpha.10 conservée. Les preuves figurent dans le
[bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#références-de-pointeurs-des-callbacks-et-cibles-de-tableaux--6-octobre-2026).

### Ajout local du 6 octobre 2026 — qualifications des champs adressés via callbacks

**VALIDÉ dans le périmètre testé.** Le frontend
Gs++ conserve `volatile` et `constante volatile` lors de la prise d'adresse
des champs et éléments, directement ou par flèche, sans modifier le type des
callbacks stockés dans des champs qualifiés. Dix corpus bilingues exécutés
supplémentaires et huit refus bilingues portent la matrice locale à **2 301** ;
le groupe cumulé compte 25 corpus exécutés. CTest Windows 5/5, GNU/Linux 6/6,
solution et validation natives réussis, conformité 20/20 par chaîne ; trois
images identiques et vérifiées. Bootstrap C++, backend, formats et ABI inchangés ;
version alpha.10 conservée, aucune publication. Les preuves
figurent dans le [bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#qualifications-des-champs-adressés-via-callbacks--6-octobre-2026).

### Ajout local du 6 octobre 2026 — retours par référence des callbacks

**VALIDÉ dans le périmètre testé.** Le backend
C++ conserve l'adresse retournée et distingue lecture et adressage, sans traiter
une référence de structure comme un retour par valeur. Le bootstrap propage la
constance des appels ; le frontend Gs++ conserve celle des champs et éléments
adressés. Quinze corpus bilingues exécutés avec callbacks C++ fournis par l'hôte
vérifient le stockage et le nombre d'appels ; quinze refus bilingues portent la
matrice locale à **2 285**. CTest Windows 5/5, GNU/Linux 6/6, solution et validation
natives Visual Studio 2026 réussis ; conformité 20/20 par chaîne, trois images
reconstruites identiques et vérifiées. Aucun retour par référence de fonction
ordinaire Gs++ n'est ajouté, aucune publication. Les preuves figurent dans le
[bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#retours-par-référence-des-callbacks--6-octobre-2026).

### Ajout local du 6 octobre 2026 — arguments agrégés des callbacks et diagnostics

**VALIDÉ dans le périmètre testé.** Les diagnostics internes des appels agrégés
ne sont plus remplacés par une incompatibilité du champ par défaut. Neuf corpus
bilingues exécutés supplémentaires couvrent les structures et tableaux, signatures
imbriquées, champs objets, bases, délégations et retours booléens ; un corpus
supplémentaire est uniquement sémantique. Vingt-deux refus bilingues portent la
matrice locale à **2 255**. CTest Windows 5/5, GNU/Linux 6/6, solution et validation
natives Visual Studio 2026 réussis ; conformité 20/20 par chaîne, trois images
reconstruites identiques et vérifiées. Version alpha.10 conservée, tranche locale
non publiée. Les preuves figurent dans le [bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#arguments-agrégés-des-callbacks-et-diagnostics-des-champs--6-octobre-2026).

### Ajout local du 6 octobre 2026 — callbacks des champs par défaut

**VALIDÉ dans le périmètre testé.** Dix corpus bilingues exécutés couvrent les
callbacks partagés par plusieurs constructeurs, signatures imbriquées, références,
pointeurs, indexations, agrégats, tableaux et valeurs remplacées. Un corpus bilingue
contrôle uniquement la sémantique des références de callbacks constantes/volatiles.
Les types et positions exactes des paramètres retenus sont comparés au bootstrap ;
quatorze refus bilingues portent le total local à **2 211**. CTest Windows 5/5,
GNU/Linux 6/6, solution et validation natives Visual Studio 2026 réussis ; conformité
20/20 par chaîne. Cette extension des tests ne change ni les algorithmes du
compilateur ni la version alpha.10 et reste non publiée. Les preuves figurent
dans le [bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#callbacks-des-champs-par-défaut-par-constructeur--6-octobre-2026).

### Ajout local du 6 octobre 2026 — qualifications des constructions

**VALIDÉ dans le périmètre testé.** Les types des conversions dans les valeurs
de champs par défaut sont résolus depuis la classe du constructeur, y compris
dans des agrégats et signatures de callbacks. Les alias parents/importés masqués
ne sont plus sélectionnés à tort ; le type déclaré du champ et les valeurs par
défaut non évaluées restent préservés. Quinze corpus bilingues exécutés comparent
les cibles de constructions locales, bases, champs et délégations au bootstrap ;
seize refus bilingues portent le total local à **2 183**. CTest Windows 5/5,
GNU/Linux 6/6 et validation native Visual Studio 2026 réussis, conformité 20/20
par chaîne. Version alpha.10 conservée, tranche locale non publiée. Les preuves
figurent dans le [bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#qualifications-des-constructions-et-conversions-des-champs-par-défaut--6-octobre-2026).

### Ajout local du 5 octobre 2026 — portées des opérateurs dans les méthodes

**VALIDÉ dans le périmètre testé.** La recherche lexicale des opérateurs depuis
les méthodes, constructeurs, destructeurs et champs par défaut est couverte
par 17 nouveaux corpus bilingues exécutés et 17 refus bilingues. Les groupes
mixtes, masquages, accès, priorité du type de gauche et choix différents pour
un même champ évalué par plusieurs constructeurs sont comparés au bootstrap.
Le total local est de **2 151 refus différentiels** ; CTest Windows 5/5,
GNU/Linux 6/6 et validation native Visual Studio 2026 réussis, conformité
20/20 par chaîne. Cette extension des tests ne change ni les algorithmes du
compilateur ni la version alpha.10 et reste non publiée. Les preuves figurent
dans le [bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#portées-des-opérateurs-dans-les-méthodes--tranche-locale-du-5-octobre-2026).

### Ajout local du 5 octobre 2026 — priorités des bases, champs et initialiseurs

**VALIDÉ dans le périmètre testé.** Les constructions récursives implicites
sont vérifiées lors de la visite de la base ou du champ, avant les expressions
suivantes. Ces contrôles ne publient aucune étape ; le plan final conserve
l'ordre base, table virtuelle et champs déclarés. Les constructeurs explicites
des champs ne sont plus sélectionnés une seconde fois pour leur plan.
Les listes de base, champ et délégation utilisent le même parcours contextuel
des arguments que les constructions locales. `parent()` / `super()` sans
constructeur propre réalise la construction implicite de la base.
Les 12 corpus valides et 32 refus bilingues, plus les contrôles d'émission,
portent le total à **1 957 refus différentiels**. Les preuves et limites figurent
dans le [bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#priorités-des-bases-champs-et-initialiseurs--tranche-locale-du-5-octobre-2026).

### Ajout local du 5 octobre 2026 — constructions locales et durée de vie

**VALIDÉ dans le périmètre testé.** Le choix du constructeur et les plans de
construction/destruction sont traités lors de la visite de chaque variable,
avant l'instruction suivante. Les arités et préfixes incompatibles interrompent
la visite des arguments ; les agrégats attendent sélection et visibilité.
Les diagnostics récursifs pointent le champ concerné sans déplacer l'origine
du plan. Les tableaux sans arguments acceptent aussi `()`. Les plans de corps
des constructeurs sont contrôlés avant leurs instructions. Les 36 refus et
12 corpus valides bilingues, puis les contrôles d'émission, portent le total
à **1 881 refus différentiels**. Les preuves et limites figurent dans le
[bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#constructions-locales-et-plans-de-durée-de-vie--tranche-locale-du-5-octobre-2026).

### Ajout local du 5 octobre 2026 — champs par défaut contextuels

**VALIDÉ dans le périmètre testé.** Les champs par défaut sont analysés avec
les paramètres de chaque constructeur qui les utilise, après sa liste
d'initialisation et avant son corps. Un initialiseur explicite remplace la
valeur par défaut ; un constructeur délégué ne la réévalue pas. Le bootstrap
possède désormais une copie d'expression par constructeur, afin de conserver
indépendamment ses choix de surcharges. Les contrôles des prototypes sont
vérifiés sur l'AST fourni à l'API sémantique. Les preuves et limites figurent
dans le [bilan du frontend](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md).

### Ajouts locaux des 4–5 octobre 2026 — inclusions et espaces utilisés

**VALIDÉ dans le bootstrap et les analyseurs auto-hébergés, dans le périmètre testé.** La syntaxe
adoptée est `#inclure "fichier"` / `#include "file"`, avec `#pragma once`,
et `utilisant espace N;` / `using namespace N;` sans `#`. Les inclusions et
imports de types, globales et groupes de surcharges sont testés dans `gsppc`.
Le lexeur, l'AST et la résolution sémantique auto-hébergés sont alignés ;
l'expansion des fichiers inclus reste réalisée par le bootstrap hôte.
Les utilisations dans les
blocs, macros, chemins `<...>` et répertoires `-I` ne sont pas inclus.
Les [règles et limites](SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md#inclusion-textuelle-et-utilisation-despaces-de-noms)
et les [preuves de cette tranche](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#inclusions-et-utilisations-despaces--tranche-locale-du-4-octobre-2026)
ne changent ni le numéro alpha.10 ni les paquets déjà publiés.

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
constructeurs globaux ni de déroulement d’exceptions.

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
classe dérivée restent prévus.

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
restent prévus.

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
détaillé de ces fonctions est désormais consolidé dans la spécification 1.0.

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
exceptions.

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
restent prévus.

### Gs++ 0.24 — contrat et conformité — terminé

- périmètre candidat du langage Gs++ 1.0 consolidé ;
- contrats GsObj, GsA et GsE 1.0 et ABI 1 documentés ;
- profils freestanding et hébergé séparés explicitement ;
- manifeste de conformité portable avec treize exigences ;
- extensions, formats, ABI, bilinguisme, reproductibilité et refus essentiels
  vérifiés sous MSVC et GNU ;
- résultats 5/5 sous MSVC, 6/6 sous GNU/WSL, benchmarks smoke et démarrage
  QEMU/OVMF réussis.

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

### Gs++ 0.26 — bibliothèque hébergée — terminé

- chaînes UTF-8 propriétaires avec validation stricte ;
- vecteurs dynamiques, table de symboles et arène à adresses stables ;
- chemins et fichiers alloués avec erreurs explicites sans exception ;
- exactement cinq imports dans `GsHebergee.GsA` et aucun import d’hôte dans
  `GsSysteme.GsA` ;
- conformité portée à dix-huit exigences ;
- résultats 5/5 sous MSVC, 6/6 sous GNU/WSL, benchmarks smoke et démarrage
  QEMU/OVMF réussis.

## Convergence vers le produit Gs++ 1.0

Le socle nécessaire à Sanctuaire SE est techniquement atteint depuis Gs++
0.23.0. La décision produit du 16 août 2026 impose néanmoins de terminer Gs++
comme produit réellement exploitable avant de développer activement
Sanctuaire SE 0.11, Gs# ou les autres couches.

La préversion actuelle est **0.27.0-alpha.10**, validée avec ses paquets extraits
dans sa [matrice propre](Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.10.md),
sans clôture du frontend 0.27.

Les prochains jalons sont donc réservés à Gs++ :

1. 0.24 — contrat du langage et suite de conformité — terminé ;
2. 0.25 — initialisation et durée de vie finalisées — terminé ;
3. 0.26 — bibliothèque hébergée suffisante pour le compilateur — terminé ;
4. 0.27 — frontend auto-hébergé — actif, lexeur et AST compact complet validés
   sous MSVC/GNU ; première passe sémantique partielle couvrant symboles,
   références, surcharges, membres, visibilité, constructeurs locaux et
   opérateurs membres, initialiseurs directs de sous-objets et bases
   implicites, tableaux d’objets, valeurs de champs par défaut et agrégats
   imbriqués avec contrôle de capacité, puis propagation récursive des types de
   retour des appels et opérateurs déjà sélectionnables, des indexations,
   adresses, déréférencements et appels indirects, puis plans ordonnés de
   construction, destruction et tables virtuelles avec disposition mémoire
   privée, puis opérateurs libres binaires et unaires avec classement des
   surcharges, qualifications de valeur et validation d’arité, puis validation
   différentielle des vingt-quatre formes intrinsèques, adaptations de
   littéraux et diagnostics de types, puis liaisons de références qualifiées,
   affectations et valeurs de retour, puis contraintes structurelles des
   déclarations globales, puis validation récursive de la forme constante de
   leurs initialiseurs, agrégats et pointeurs, puis calcul numérique des
   constantes et valeurs d’énumération avec contrôles de plage et de division
   par zéro, puis émission des octets globaux, disposition des zones
   données/zéro et relocalisations de fonctions, puis validation des conversions
   explicites et plages constantes dans l’alpha.9 ; adaptations implicites des
   constantes composées et sélection des surcharges, puis références de callbacks,
   signatures imbriquées et tableaux de pointeurs à indirections profondes dans
   le développement suivant, puis résolution contextuelle des types nommés
   dans les signatures et appels de callbacks stockés dans les champs, puis
   contraintes récursives de signatures, limites d'arité avec récepteur
   implicite et opérateurs libres sur structures/unions, puis chaînes d'alias
   de champs, validation des alias inutilisés et diagnostics de cycles/cibles
   introuvables avec stockage canonique commun aux accès et initialisations ;
   puis alias racines de types, fonctions libres et globales, chaînes et
   déclarations anticipées, noms qualifiés prioritaires, diagnostics 109–112
   et relocalisations vers les fonctions canoniques ; puis appels via alias de
   méthodes non liées, récepteur `Classe&` explicite, visibilité des appels
   directs, callbacks et relocalisations vers les méthodes canoniques ;
   puis déclarations d'héritage, bases canoniques dans l'espace déclarant,
   refus des bases absentes, non-classes ou non publiques et de l'auto-héritage,
   cycles indirects et diagnostics 113–115 dans le développement local après
   alpha.10 ; puis remplacements de méthodes, destructeurs et opérateurs virtuels,
   diagnostics 116–117 et dispositions des classes uniquement polymorphes par
   leur destructeur ou opérateur dans le périmètre testé ; puis refus des
   doublons de surcharges, avec paramètres canoniques, alias, retour exclu,
   méthodes, constructeurs, destructeurs et opérateurs, diagnostic 118 ;
   puis signatures non liées, récepteur implicite comparable à un premier
   paramètre explicite Classe&, et collisions méthode/fonction libre dans un
   espace homonyme de la classe ; puis collisions entre symboles de liaison
   calculés pour des surcharges distinctes, types canoniques, récepteurs,
   callbacks, espaces qualifiés ou UTF-8 et diagnostic 119, dans le périmètre
   différentiel testé ; puis sélection des appels qualifiés, par objet ou
   pointeur, dans les groupes mêlant méthodes et fonctions libres, récepteurs,
   scores, ambiguïtés, visibilité et déclaration choisie comparés au bootstrap ;
   puis groupes d'opérateurs unaires et binaires mixtes et priorité déterministe
   des doublons entre groupes indépendants, collisions de liaison et parcours
   des corps dans l'ordre source, dans le périmètre testé ; puis priorité des
   instructions, blocs, expressions de conditions, branches et boucles d'un
   même corps ; puis priorité des opérandes, de l'objet avant l'indice, de la
   cible d'affectation avant sa valeur et du type cible de conversion avant sa
   source, dans le périmètre testé le 4 octobre 2026 ; puis arguments d'appels
   dans l'ordre source, cible indirecte et arité contrôlées avant eux,
   rejet des groupes sans arité et récepteur recevables, sélection différée
   et appels imbriqués ; puis abandon des candidats après un préfixe incompatible
   dans l'ordre de déclaration, avant l'argument suivant, et erreurs de types de
   conversions différées jusqu'à la visite de leur expression, dans le périmètre
   testé ; puis arguments agrégés contextuels, après sélection et visibilité
   pour les groupes directs, dans l'ordre des arguments pour les callbacks,
   avec formes scalaires, structures, unions et tableaux de champs imbriqués ;
   puis calcul et contrôle de plage des conversions constantes au moment de leur
   visite, avec priorité des appels, agrégats contextuels et courts-circuits dans
   la matrice différentielle du 4 octobre 2026 ;
   puis initialiseurs locaux contextuels, forme et capacité avant les feuilles,
   validation des éléments dans l'ordre et refus avant l'instruction suivante,
   avec références, callbacks, classes et plages numériques dans le périmètre testé ;
   puis validation complète de chaque initialiseur global dans l'ordre source,
   formes et typage de toutes les feuilles avant la passe constante de la même
   globale, contrôles structurels des champs par défaut avant les globales et
   fonctions dans la matrice du 4 octobre 2026 ;
   contextes des constructions et autres priorités entre passes à compléter ;
   matrice des qualifications et autres familles sémantiques encore à compléter, raccordement aux
   écrivains d’objets auto-hébergés dans le jalon backend ;
5. 0.28 — backend, formats et linker auto-hébergés, sélection de cible native
   ou explicite, sorties Windows PE et Linux ELF et conservation de GsE pour
   la cible Galactic-Shrine ; remplacement de `.GsA` par `.Glib` pour les
   bibliothèques statiques à partir de 0.28.0, `.GdLib` réservé aux éventuelles
   bibliothèques dynamiques, `.GsE` conservé pour les exécutables ; séparation
   de la compilation `gsppc` et de la construction des projets/solutions dans
   GsBuild (nom proposé, commande `gsbuild`), avec archivage et liaison pilotés
   par l'outil de construction ;
6. 0.29 — durcissement, reproductibilité, SDK et distribution, exécution sur
   les systèmes cibles et validation des combinaisons de compilation croisée ;
7. 1.0.0 — sortie produit après satisfaction de tous les critères.

**Décision du 5 octobre 2026 — prévue pour le jalon 0.28, non implémentée :**
introduire un outil dédié de construction des projets `.GsPj`/`.GsProject` et
solutions `.GsPs`, proposé sous le nom **GsBuild** (`gsbuild`). Il pilote les
compilations, la création de bibliothèques et l'édition de liens ; `gsppc` se
limite à compiler les sources et interfaces Gs++ en objets. La séparation
reprend le rôle d'un moteur de construction comme MSBuild, sans promettre la
compatibilité avec ses projets ou tâches. CMake et MSBuild restent des moyens
de construire la toolchain. Les commandes 0.27 restent inchangées jusqu'à une
migration implémentée, testée et documentée ; aucun GsBuild n'est encore livré.
Le [plan produit](PLAN_PRODUIT_GS_PLUS_PLUS_1.0.md#séparer-compilation-et-construction--décision-du-5-octobre-2026)
définit les responsabilités et la transition.

**Décision du 21 septembre 2026 — prévue pour 0.28.0, non implémentée :**
les extensions de bibliothèques deviennent `.Glib` (statique, remplacement de
`.GsA`) et `.GdLib` (dynamique, si ce support est introduit). `.GsE` reste
l'extension des exécutables. Les outils et paquets 0.27 conservent `.GsA` ;
la migration des extensions ne change pas à elle seule les signatures binaires
ni l'ABI et ne signifie pas que la liaison dynamique est déjà disponible.

**Décision du 19 septembre 2026 — prévue, non encore implémentée :** compiler
pour l'environnement utilisé par défaut, avec choix explicite d'une autre
plateforme. La ligne de commande prime sur le projet XML, qui prime sur la
détection de l'hôte. Le premier périmètre est x86-64 sous Windows, GNU/Linux
et Sanctuaire SE / ShrineOS. Chaque cible nécessite un backend, un format de
sortie, des conventions de liaison et un SDK adaptés ; une cible indisponible
doit produire un diagnostic. Les tests actuels sur deux hôtes ne constituent
pas encore une preuve de production d'exécutables natifs PE et ELF.

Le périmètre détaillé, les invariants gelés et les critères de sortie se
trouvent dans
[`PLAN_PRODUIT_GS_PLUS_PLUS_1.0.md`](PLAN_PRODUIT_GS_PLUS_PLUS_1.0.md).

Les preuves intermédiaires du lexeur et de l’AST 0.27 se trouvent dans
[`FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md`](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md).

Sanctuaire SE reste sur la référence 0.10.2 et continue d’être reconstruit
comme preuve d’intégration réelle. Toute modification Gs++ susceptible
d’affecter le code natif doit encore produire `Noyau.GsE`, `BOOTX64.EFI` et
l’image ESP, puis réussir le test QEMU/OVMF. Aucun développement fonctionnel de
l’ordonnanceur, des processus ou du mode utilisateur n’est engagé avant Gs++
1.0.
