# Réorganisation du monorepo — 2026-07-23

- séparation du compilateur, du SDK, des bibliothèques et de l’auto-hébergement ;
- déplacement de Sanctuaire SE dans un projet indépendant ;
- ajout du mode de projet agrégé, désormais exprimé par l’attribut XML
  `ModeCompilation="agregee"`, pour les programmes
  monolithiques composés de plusieurs fichiers, utilisé par le noyau ;
- séparation des tests Gs++ et Sanctuaire SE ;
- centralisation des résultats dans `Construction/`.

# Journal des modifications

## Développement après Gs++ 0.27.0-alpha.7 — 2026-08-26

- adoption de `GalacticShrine::GsPP::` comme préfixe public et ABI canonique,
  `GsPP::` seul restant exclu des contrats exportés ;
- ajout des conventions de code Gs++ 1.0 en Markdown, avec accolades ouvrantes
  sur la ligne de la déclaration, indentation de quatre espaces et blocs
  documentaires `/** … **/` ;
- ajout des sections `<résumé>...</résumé>` et des balises typées
  `@Paramètre(type: nom)` et `@Retourner(type)` aux API documentées ;
- migration mécanique des sources livrées dans `Bibliotheques` et
  `AutoHebergement` vers la présentation canonique ;
- déplacement des sorties des presets CMake vers le dossier central voisin
  `../Construction/GsPlusPlus-Development`, hors du dépôt source ;
- ajout d’un contrôle CTest portable des espaces de noms, accolades,
  tabulations et formes de commentaires ;
- première sélection typée des surcharges libres à partir des paramètres,
  variables, conversions explicites et littéraux représentables ;
- résolution des champs, alias de champs et méthodes par `.` ou `->`, avec
  parcours de la chaîne d’héritage et identification des membres hérités ;
- extension du classement des surcharges aux paramètres par référence,
  agrégats temporaires et conversions `Dérivée → Base&` ou
  `Dérivée* → Base*` ;
- ajout des diagnostics `AucuneSurchargeCompatible` et
  `AppelSurchargeAmbigu`, `RecepteurMembreInvalide` et `MembreIntrouvable`,
  comparés au bootstrap C++ ;
- application de la visibilité `publique`, `protégée` et `privée` aux champs,
  méthodes et opérateurs, en autorisant la classe propriétaire et les classes
  dérivées dans le cas protégé ;
- résolution surchargée des constructeurs de variables locales de classes,
  avec distinction entre construction implicite et syntaxe explicite ;
- première sélection des opérateurs membres unaires et binaires à partir du
  type de l’opérande gauche et des paramètres explicites ;
- ajout des diagnostics `MembreInaccessible`, `ConstructeurInaccessible`,
  `ConstructeurNonDeclare`, `OperateurIntrouvable` et
  `InitialiseurClasseInterdit`, portant à trente-trois le nombre de corpus
  sémantiques négatifs comparés au bootstrap ;
- ajout des drapeaux de résolution `Constructeur`, `ConstructionExplicite` et
  `Operateur`, sans modification des tailles du contrat ABI ;
- validation complète avec Visual Studio 2026 et GNU/Linux, avec quatre images
  auto-hébergées identiques bit à bit entre les deux chaînes.

## Compilateur Gs++ 0.27.0-alpha.7 — 2026-08-25

- ajout de la première passe sémantique auto-hébergée, consommant l’AST compact
  sans le modifier et produisant une table de symboles et des résolutions ;
- indexation des types, fonctions, globales, alias, champs, alias de champs,
  énumérateurs, paramètres et variables locales ;
- résolution des portées locales et imbriquées, paramètres, globales
  qualifiées, récepteurs `soi` / `this` et `parent` / `super`, et groupes de
  surcharges appelés ;
- ajout de quinze corpus négatifs dont les diagnostics positionnés sont
  comparés au bootstrap C++ ;
- maintien d’un contrat à stockage fourni par l’appelant, avec tailles ABI de
  48 octets pour un symbole, 32 pour une résolution, 56 pour le résultat et
  120 pour la requête ;
- ajout de `AnalyseurSemantique.GsE` à la construction, aux tests, à
  l’installation et aux paquets ;
- migration volontaire de tous les symboles Gs++ actuels de `Gs::…` vers le
  préfixe canonique `GalacticShrine::GsPP::…`, sans alias de compatibilité ;
- adoption de la forme `/** … **/` pour les commentaires de bloc multilignes
  du code Gs++ actif et ajout de cette forme au corpus différentiel du lexeur ;
- conservation des formats GsObj/GsA/GsE en version 1.0, des champs ABI à 1 et
  des signatures `GSOBJ:0`, `GSA:0` et `GSE:0`.

## Compilateur Gs++ 0.27.0-alpha.6 — 2026-08-25

- extension de l’AST auto-hébergé aux onze genres d’expressions du frontend
  bootstrap : entier, chaîne, variable, unaire, binaire, affectation, appel,
  membre, index, conversion et agrégat ;
- reproduction en Gs++ des priorités, associativités et parcours récursifs des
  six opérateurs unaires et des dix-huit opérateurs binaires ;
- rattachement en préordre des sous-expressions aux globales, énumérateurs,
  champs, constructeurs, retours, variables locales, contrôles et instructions
  d’expression ;
- conservation des valeurs entières, chaînes décodées, noms qualifiés,
  opérateurs et empreintes de types de conversion dans le nœud ABI compact ;
- ajout des drapeaux publics pour les littéraux booléens, les accès membres par
  pointeur et les références à la base `parent` / `super` ;
- maintien de `NoeudDeclaration` à 64 octets, de la requête à 80 octets et du
  résultat à 48 octets, sans renumérotation des genres 0 à 21 ;
- ajout d’un corpus différentiel français/anglais couvrant chaque genre,
  opérateur, position et relation parent-enfant d’expression ;
- extension à trente-trois diagnostics syntaxiques comparés au bootstrap,
  notamment pour les conversions, appels, indexations, agrégats, opérandes
  absents et dépassements d’entiers 64 bits ;
- installation du logo Gs++ avec les README afin que l’identité visuelle reste
  résolue dans les paquets Windows et Linux ;
- conservation des formats GsObj/GsA/GsE en version 1.0, des champs ABI à 1 et
  des signatures `GSOBJ:0`, `GSA:0` et `GSE:0`.

## Compilateur Gs++ 0.27.0-alpha.5 — 2026-08-24

- extension de l’AST auto-hébergé à la hiérarchie des blocs et instructions
  des fonctions libres et des membres exécutables de classes ;
- ajout de six genres de nœuds stables pour les blocs, retours, instructions
  d’expression, variables locales, conditionnelles et boucles `tantque` ;
- conservation des relations parent-enfant en préordre entre fonction, bloc,
  instruction de contrôle et branche imbriquée ;
- description de la présence d’une expression, d’une branche `sinon`, d’un
  initialiseur local ou d’une construction explicite, sans exposer encore
  l’AST interne des expressions ;
- ajout des alias ABI génériques `NoeudSyntaxique`, `ResultatAnalyseSyntaxique`
  et `RequeteAnalyseSyntaxique`, compatibles avec le contrat compact existant ;
- extension de l’oracle différentiel C++ à des corps bilingues imbriquant
  variables locales, blocs, retours, conditionnelles et boucles ;
- passage de quatorze à vingt-deux diagnostics syntaxiques comparés au bootstrap,
  avec ligne et colonne identiques ;
- maintien des tailles ABI de 64 octets pour le nœud, 48 octets pour le
  résultat et 80 octets pour la requête ;
- validation complète sous Visual Studio 2026 et GNU/Linux, conformité 20/20
  et reproductibilité bit à bit des images auto-hébergées.

## Compilateur Gs++ 0.27.0-alpha.4 — 2026-08-24

- extension de l’AST auto-hébergé aux méthodes, constructeurs, destructeurs et
  surcharges d’opérateurs déclarés dans les classes ;
- ajout de quatre genres de nœuds dédiés, rattachés à leur classe par la
  relation parent-enfant, sans exposer comme source le paramètre implicite
  `soi` synthétisé par le bootstrap ;
- conservation de la visibilité, de la présence du corps, des modificateurs
  `virtuel` et `remplacer`, ainsi que des listes d’initialisation de base, de
  champ ou de constructeur délégué ;
- analyse bilingue des paramètres explicites et délimitation des corps et
  arguments d’initialisation imbriqués, sans revendiquer encore leur AST
  d’instructions ou d’expressions ;
- ajout de diagnostics positionnés pour les opérateurs invalides, les
  modificateurs dupliqués ou interdits, les paramètres de destructeur et les
  listes d’initialisation invalides ;
- extension de la comparaison différentielle avec le bootstrap C++ à des
  classes bilingues mêlant champs et membres exécutables ;
- maintien des tailles ABI de 64 octets pour `NoeudDeclaration`, 48 octets
  pour le résultat et 80 octets pour la requête ;
- validation complète sous Visual Studio 2026 et GNU/Linux, conformité 20/20
  et reproductibilité bit à bit des images auto-hébergées.

## Compilateur Gs++ 0.27.0-alpha.3 — 2026-08-23

- extension de l’AST auto-hébergé aux variables globales, structures, unions,
  classes de données, champs, énumérations, énumérateurs et alias ;
- conservation du contrat ABI compact de 64 octets par nœud, avec nouveaux
  genres bilingues, relations parent-enfant et drapeaux de visibilité,
  définition, initialiseur et héritage ;
- prise en charge des globales publiques, externes, initialisées et en tableau,
  ainsi que des agrégats d’initialisation correctement délimités ;
- analyse des champs de structures, unions et classes, des sections de
  visibilité, de l’héritage simple et des initialiseurs de champs de classes ;
- analyse des énumérations avec valeurs implicites ou explicites et des alias
  de déclarations ou de champs ;
- délimitation sûre des initialiseurs imbriqués sans revendiquer encore leur
  AST d’expression, réservé à la tranche suivante ;
- comparaison différentielle avec le bootstrap C++ sur des corpus de données
  français et anglais, en plus des corpus de fonctions de l’alpha.2 ;
- diagnostics positionnés ajoutés pour les initialiseurs externes, crochets,
  séparateurs et initialiseurs de champs invalides ;
- refus distinct des méthodes, constructeurs, destructeurs et opérateurs de
  classes, afin de rendre visible la frontière restante du frontend 0.27 ;
- validation complète 3/3 sous Visual Studio 2026 et 4/4 sous GNU/Linux,
  conformité 20/20 et quatre scénarios de benchmark smoke réussis sur chaque
  chaîne ;
- reproductibilité bit à bit des trois images auto-hébergées entre MSVC et GNU,
  dont `AnalyseurDeclarations.GsE` de 72 001 octets.

## Compilateur Gs++ 0.27.0-alpha.2 — 2026-08-23

- ajout de la première représentation AST auto-hébergée, sous forme de nœuds
  de déclarations de 64 octets à stockage fourni par l’appelant ;
- migration en Gs++ de l’analyse des fonctions libres, paramètres et espaces
  de noms simples, imbriqués ou qualifiés ;
- normalisation bilingue des empreintes de types, couvrant les types natifs,
  types qualifiés, qualificatifs, pointeurs, références et tableaux fixes ;
- construction des jetons et de l’AST de travail dans l’arène hébergée 0.26,
  avec interrogation de capacité, sortie partielle bornée et libération
  vérifiée de toutes les allocations ;
- propagation positionnée des erreurs lexicales et diagnostics syntaxiques
  bornés pour les identifiants, types, parenthèses, accolades et déclarations
  externes ;
- ajout de `AnalyseurDeclarations.GsE` à la construction, à l’installation et
  aux paquets Windows/Linux, sans nouvelle dépendance d’hôte ;
- comparaison différentielle avec l’AST du bootstrap C++ sur des corpus
  français et anglais, espaces qualifiés, tableaux, types qualifiés et trois
  familles d’erreurs syntaxiques ;
- conservation explicite du statut partiel du frontend 0.27 : les structures,
  énumérations, alias, variables globales, instructions et expressions restent
  à migrer avant de déclarer l’analyseur syntaxique complet ;
- validation complète 3/3 sous Visual Studio 2026 et 4/4 sous GNU/Linux,
  conformité 20/20 et quatre scénarios de benchmark smoke réussis sur chaque
  chaîne ;
- reproductibilité bit à bit de `Lexeur.GsE` et
  `AnalyseurDeclarations.GsE` entre les constructions MSVC et GNU.

## Compilateur Gs++ 0.27.0-alpha.1 — 2026-08-23

- première préversion publique destinée aux essais, sans promesse de stabilité
  de l’interface en ligne de commande avant Gs++ 1.0 ;
- remplacement du prototype texte des fichiers `.GsPj`, `.GsProject` et
  `.GsPs` par le format XML strict 1.0, en vocabulaire français ou anglais ;
- refus explicite de l’ancien format `clé = valeur`, des versions de schéma
  inconnues, des éléments ou attributs inconnus et des alias dupliqués ;
- ajout d’une construction CMake autonome de `Gs++` sous Windows et Linux,
  sans inclure `SanctuaireSE` et sans exiger ni distribuer `Noyau.GsE` ;
- adoption de Visual Studio 2026, du générateur CMake
  `Visual Studio 18 2026` et du runner GitHub Windows correspondant ;
- refonte du README français autour de l’identité et des usages du langage,
  accompagnée d’un README anglais équivalent ;
- ajout du lexeur auto-hébergé Gs++ et de sa preuve différentielle ; le reste
  du frontal auto-hébergé 0.27 demeure en développement et cette version reste
  donc une alpha ;
- validation autonome 3/3 sous MSVC et 4/4 sous GNU/WSL, conformité 20/20 et
  quatre scénarios de benchmark smoke réussis sur chaque hôte ;
- conservation des formats GsObj/GsA/GsE en version 1.0, des champs ABI à 1 et
  des signatures `GSOBJ:0`, `GSA:0` et `GSE:0`.

## Compilateur Gs++ 0.26.0 — 2026-08-23

- ajout de la bibliothèque hébergée propriétaire nécessaire à la future
  migration du compilateur : chaînes UTF-8, vecteurs d’octets et de naturels
  dynamiques, table de symboles dynamique et arène à adresses stables ;
- validation UTF-8 stricte avec refus des surlongueurs, substituts UTF-16 et
  points de code supérieurs à `U+10FFFF` ;
- ajout des chemins UTF-8, vues de nom et d’extension, chargement de fichier
  alloué en deux requêtes et écriture à résultat explicite ;
- ajout du modèle d’erreur `CodeErreurHebergee`, sans exception, avec réserve,
  affectation et croissance transactionnelles en cas d’échec d’allocation ;
- extension du contrat d’hôte à exactement cinq imports explicites :
  allocation, libération, lecture, écriture et diagnostic ;
- ajout d’une initialisation et d’une destruction explicites pour chaque type
  propriétaire, y compris `FichierAlloue`, sans dépendre d’une valeur locale
  non initialisée pour représenter le pointeur nul ;
- correction de l’analyse sémantique afin qu’une conversion explicite de
  pointeur conserve les qualificatifs de sa cible, notamment
  `convertir<constante T*>(T*)` ;
- extension du test GsE de la bibliothèque hébergée avec croissance, échec
  transactionnel, auto-ajout, table réallouée, stabilité d’arène, chemins,
  fichiers alloués et contrôle de toutes les libérations ;
- extension de la conformité portable de 16 à 18 exigences : preuve des cinq
  imports de `GsHebergee.GsA` et preuve de l’absence de tout import
  `Gs::Hote` dans `GsSysteme.GsA` ;
- conservation stricte des signatures `GSOBJ:0`, `GSA:0` et `GSE:0`, des
  formats natifs en version `1.0`, des champs ABI à `1` et de la signature de
  liaison `GsAbi:x64-ms-v1` ;
- validation complète 5/5 sous MSVC et 6/6 sous GNU/WSL, conformité 18/18 sur
  chaque chaîne, quatre scénarios de benchmark smoke réussis sur chaque hôte et
  démarrage QEMU 11.0.50/OVMF réussi avec mémoire, horloge et clavier.

## Compilateur Gs++ 0.25.0 — 2026-08-16

- ajout des valeurs par défaut au point de déclaration des champs de classes,
  évaluées pour chaque construction et remplacées par une entrée explicite de
  la liste d’initialisation lorsqu’elle existe ;
- prise en charge des agrégats de tableaux non classes comme valeurs de champ
  par défaut, avec mise à zéro déterministe des éléments absents ;
- ajout de la délégation entre constructeurs avec `soi(arguments)` et
  `this(arguments)`, résolution de surcharge, exécution du corps délégant après
  la cible et refus statique des délégations directes ou cycliques ;
- ajout d’une liste d’arguments uniforme pour chaque élément des tableaux
  locaux et des tableaux de champs objets classes, avec réévaluation des
  expressions dans l’ordre croissant et destruction dans l’ordre inverse ;
- exclusion normative des objets de classe globaux, tableaux compris, afin de
  préserver le profil freestanding sans table cachée de constructeurs ou de
  destructeurs ;
- diagnostics bilingues pour les valeurs par défaut hors classe, les champs
  objets classes initialisés avec `=`, les classes sans constructeur explicite,
  les listes déléguées mélangées et les objets globaux incompatibles ;
- ajout de scénarios GsE monolithique et séparé produisant les traces de
  construction `123`, de destruction `321` et le code de retour `25` ;
- extension de la conformité portable de 13 à 16 exigences avec l’exécution du
  cycle de vie 0.25, le refus d’un objet de classe global et le refus d’un cycle
  de délégation ;
- conservation stricte des signatures `GSOBJ:0`, `GSA:0` et `GSE:0`, des
  formats natifs en version `1.0`, des champs ABI à `1` et de la signature de
  liaison `GsAbi:x64-ms-v1` ;
- validation complète 5/5 sous MSVC et 6/6 sous GNU/WSL, conformité 16/16 sur
  chaque chaîne, quatre scénarios de benchmark smoke réussis sur chaque hôte et
  démarrage QEMU 11.0.50/OVMF réussi avec mémoire, horloge et clavier.

## Compilateur Gs++ 0.24.0 — 2026-08-16

- gel du périmètre candidat Gs++ 1.0 dans une spécification canonique couvrant
  identité, extensions, types, expressions, modèle objet, durée de vie,
  compilation séparée, profils et limites explicitement différées ;
- publication de la spécification binaire GsObj 1.0 et consolidation des
  contrats GsA 1.0, GsE 1.0 et de l’ABI native `GsAbi:x64-ms-v1`, sans modifier
  les formats 1.0 ni les champs ABI à 1 ;
- définition normative des profils freestanding et hébergé, avec interdiction
  des dépendances hébergées implicites dans le noyau, le chargeur et
  `GsSysteme.GsA` ;
- ajout d’un manifeste de conformité versionné et d’un exécuteur Python 3
  portable produisant un rapport JSON par construction ;
- ajout de treize preuves de conformité couvrant les extensions source et
  interface, les trois en-têtes binaires, l’ABI, le bilinguisme français et
  anglais, la reproductibilité locale et les refus de `.GsO`, `.GsPPH`, `.Gs#`
  et d’une image GsE tronquée ;
- intégration de la conformité à CTest sous MSVC et GNU, en complément des
  tests unitaires, de l’intégration GNU, de l’auto-hébergement partiel, des
  benchmarks et de la preuve QEMU/OVMF ;
- adoption du plan produit Gs++ 1.0 : Gs++ reste le seul produit développé
  activement jusqu’à sa sortie exploitable ; Sanctuaire SE 0.10.2 demeure une
  barrière d’intégration et Gs# reste différé sans fichiers d’en-tête.

## Compilateur Gs++ 0.23.0 — 2026-08-16

- prise en charge des tableaux fixes de champs objets classes, y compris les
  tableaux multidimensionnels, sans modifier leur disposition mémoire ;
- construction par défaut de chaque élément dans l’ordre des indices et à son
  décalage réel, avec résolution du constructeur sans argument, contrôle
  d’accès et initialisation récursive des bases, champs et tables virtuelles ;
- destruction de chaque élément dans l’ordre strictement inverse, intégrée au
  plan RAII récursif du conteneur et conservée sur les fins de blocs, branches,
  boucles et retours anticipés ;
- acceptation de `Champ()` pour rendre explicite la construction par défaut de
  tout le tableau ; les arguments par élément et les initialiseurs agrégés
  d’objets classes restent volontairement différés et produisent un diagnostic
  bilingue précis ;
- extension du même cycle de vie aux tableaux locaux d’objets classes ;
- ajout de contrôles unitaires sur un tableau `[2][3]`, ses décalages de
  construction croissants et ses décalages de destruction décroissants ;
- ajout de scénarios GsE monolithique et séparé sur un tableau `[2][2]`, avec
  trace de destruction `4321` et code de retour `10` ;
- conservation stricte des signatures `GSOBJ:0`, `GSA:0` et `GSE:0`, des trois
  formats natifs en version `1.0`, de tous les champs ABI à `1` et de la
  signature de liaison `GsAbi:x64-ms-v1` ;
- validation propre 4/4 sous MSVC et 5/5 sous GNU/WSL, puis démarrage réel
  virtualisé réussi sous QEMU 11.0.50/OVMF avec mémoire, horloge et clavier ;
- ajout d’un protocole de benchmark sans revendication de performance, fondé
  exclusivement sur les corpus d’intégration validés, avec pilote commun
  Windows/GNU, sessions isolées, résultats JSONL/JSON/CSV, validation des
  signatures et séparation des étapes GsObj, GsA, GsE et exécution, puis
  contrôle fonctionnel des 16 couples scénario/condition sous Windows/MSVC et
  GNU/WSL.

## Compilateur Gs++ 0.22.1 — 2026-08-16

- adoption de l’inventaire consolidé des extensions Galactic-Shrine : sources
  Gs++ `.Gs++`, `.GsPP`, `.GsPlusPlus` et interfaces `.HGs++`, `.HGsPP`,
  `.HeaderGsPlusPlus` ;
- réservation distincte de `.Gs#`, `.GsS` et `.GsSharp` pour le futur
  compilateur Gs#, avec diagnostic de routage explicite lorsqu’elles sont
  données à `gsppc` ; Gs# suit le modèle C# et ne possède aucun fichier
  d’en-tête ;
- retrait des interfaces `.GsPPH` et `.GsPlusPlusHeader`, désormais refusées
  avec `.GsPH` et l’ancien objet `.GsO` comme extensions obsolètes ;
- remplacement de la signature GsA historique par `GSA:0` sur un champ de huit
  octets, sans agrandir son en-tête de 32 octets ;
- remplacement de la signature GsE historique par `GSE:0` sur un champ de huit
  octets, sans agrandir son en-tête de 112 octets ;
- maintien des trois formats GsObj, GsA et GsE en version `1.0`, avec ABI `1`
  maintenant explicite dans les en-têtes GsA et GsE ;
- mise à jour du chargeur hôte, du chargeur UEFI, de l’outil d’image ESP, des
  projets CMake/Visual Studio, des interfaces actives et des tests ; les
  artefacts locaux antérieurs doivent être entièrement reconstruits.
- validation propre 4/4 sous MSVC et 5/5 sous GNU/WSL, puis démarrage réel
  virtualisé réussi sous QEMU 11.0.50/OVMF avec mémoire, horloge et clavier.

## Compilateur Gs++ 0.22.0 — 2026-08-16

- prise en charge des champs objets de type classe dans les listes de
  construction, avec résolution surchargée de `Champ(arguments)` et contrôle
  d’accès du constructeur sélectionné ;
- construction par défaut automatique des champs classes omis, dans l’ordre de
  déclaration, y compris à travers une classe intermédiaire qui ne déclare pas
  elle-même de constructeur ;
- plan récursif de construction des bases, des tables virtuelles et des champs
  imbriqués à leur décalage réel dans l’objet ;
- plan récursif de destruction exécuté dans l’ordre corps du destructeur,
  champs directs en ordre inverse, puis base, avec propagation aux sorties de
  bloc, branches, boucles et retours anticipés déjà couverts par le RAII ;
- diagnostics pour constructeur ou destructeur inaccessible, absence de
  constructeur sans argument compatible et arguments fournis à une classe qui
  ne déclare aucun constructeur ;
- rejet explicite et documenté des tableaux de champs objets classes, laissés
  hors du périmètre 0.22 avec les constructeurs délégués et les exceptions ;
- ajout de scénarios monolithique et séparé vérifiant la construction
  `1,2,3,4`, la destruction `9,5,6,7,8`, le dispatch virtuel d’un champ
  imbriqué sans constructeur et un code de retour GsE `91` ;
- extension des tests unitaires bilingues et de l’intégration, puis validation
  propre 4/4 sous MSVC, 5/5 sous GNU/WSL et démarrage réel de Sanctuaire SE
  sous QEMU 11.0.50/OVMF ;
- conservation stricte de la signature `GSOBJ:0`, des formats GsObj/GsA/GsE
  1.0 et de `GsAbi:x64-ms-v1` avec tous les champs ABI à 1.

## Compilateur Gs++ 0.21.0 — 2026-08-16

- extension de la liste de construction avec les initialisateurs de champs
  directs, par exemple `: parent(valeur), Bonus(bonus)` et sa forme anglaise
  `: super(value), Bonus(bonus)` ;
- obligation de placer `parent/super` en premier, puis les champs uniques dans
  leur ordre de déclaration, afin de rendre l’ordre d’évaluation explicite et
  déterministe ;
- normalisation des alias de champs vers leur stockage canonique et refus des
  champs inconnus ou hérités dans le constructeur dérivé ;
- prise en charge des champs scalaires, constants, pointeurs, structures non
  classes, tableaux et agrégats, avec mise à zéro conservée pour les champs non
  listés ;
- génération des initialisations après la construction de la base et
  l’installation de la table virtuelle courante, mais avant le corps du
  constructeur ;
- diagnostics d’arité et de type, rejet des doublons, de l’ordre incorrect et
  des champs objets de type classe tant que leur durée de vie récursive n’est
  pas spécifiée ;
- ajout de scénarios monolithique et séparé contrôlant la trace de construction
  `1,2,3,4,5`, la destruction `6,7` et un code de retour GsE `75` ;
- extension des tests unitaires bilingues et du script d’intégration, puis
  validation propre 4/4 sous MSVC, 5/5 sous GNU/WSL et démarrage réel de
  Sanctuaire SE sous QEMU 11.0.50/OVMF ;
- conservation stricte de la signature `GSOBJ:0`, des formats GsObj/GsA/GsE
  1.0 et de `GsAbi:x64-ms-v1` avec tous les champs ABI à 1.

## Compilateur Gs++ 0.20.0 — 2026-08-16

- ajout de l’initialiseur explicite de la base directe dans un constructeur,
  avec `: parent(arguments)` en français et `: super(arguments)` en anglais ;
- résolution surchargée du constructeur de base, adaptation des arguments selon
  l’ABI des paramètres et contrôle des accès publics ou protégés ;
- rejet d’un initialiseur sur une classe racine, d’arguments fournis à une base
  sans constructeur déclaré, d’un constructeur inaccessible et de l’absence
  d’un constructeur de base invocable sans argument lorsque nécessaire ;
- ajout de `parent.Methode()`/`super.Method()` pour appeler directement
  l’implémentation héritée, sans dispatch virtuel, tout en conservant les appels
  ordinaires virtuels au travers de `Base&` et `Base*` ;
- déplacement de l’appel des constructeurs de base et de l’installation des
  tables virtuelles dans le prologue de chaque constructeur déclaré ; les
  chaînes contenant des classes intermédiaires sans constructeur restent
  construites exactement une fois, de la racine vers la dérivée ;
- ajout de scénarios monolithique et séparé qui vérifient la trace complète
  `1,2,3,4`, le dispatch virtuel, l’appel parent direct et un retour GsE `82` ;
- extension du classificateur auto-hébergé à 83 classifications avec
  `parent/super`, sans réserver l’identifiant utilisateur `base` ;
- validation propre 4/4 sous MSVC, 5/5 sous GNU/WSL et démarrage réel de
  Sanctuaire SE sous QEMU 11.0.50/OVMF ;
- conservation stricte de la signature `GSOBJ:0`, des formats GsObj/GsA/GsE
  1.0 et de `GsAbi:x64-ms-v1` avec tous les champs ABI à 1.

## Compilateur Gs++ 0.19.0 — 2026-08-16

- ajout de l’héritage simple avec la syntaxe canonique
  `classe Derivee : publique Base` et l’alias anglais
  `class Derived : public Base` ;
- prise en charge sémantique des champs et méthodes hérités, avec accès
  `protégée/protected` depuis les classes dérivées et conservation de
  l’interdiction des membres privés ;
- ajout de `remplacer/override`, obligatoire lorsqu’une méthode virtuelle
  héritée est redéfinie, avec diagnostic si la signature exacte n’existe pas
  ou si la cible de base n’est pas virtuelle ;
- conversions implicites dérivée vers base limitées aux références et
  pointeurs ; les copies par valeur qui provoqueraient un slicing restent
  refusées ;
- disposition de la sous-classe de base au décalage zéro, héritage des
  emplacements virtuels, remplacement en place et ajout déterministe des
  nouvelles méthodes virtuelles ;
- construction automatique des bases de la racine vers la classe directe,
  installation de la table virtuelle appropriée à chaque étape et destruction
  de la dérivée vers la racine ;
- extension des signatures ABI de types avec la hiérarchie, le décalage du
  pointeur de table et le fournisseur de chaque emplacement virtuel, afin de
  refuser à l’édition de liens des définitions incompatibles ;
- scénarios GsE monolithique et séparé réellement exécutés, avec codes de
  retour respectifs `88` et `42` ;
- extension du classificateur auto-hébergé à 81 classifications avec
  `remplacer/override` ;
- validation propre 4/4 sous MSVC, 5/5 sous GNU/WSL et démarrage réel de
  Sanctuaire SE sous QEMU 11.0.50/OVMF ;
- conservation stricte de la signature `GSOBJ:0`, des formats GsObj/GsA/GsE
  1.0 et de `GsAbi:x64-ms-v1` avec tous les champs ABI à 1.

## Compilateur Gs++ 0.18.0 — 2026-08-15

- ajout des déclarations `classe/class` avec sections `publique/public`,
  `protégée/protected` et `privée/private`, visibilité privée par défaut et
  contrôle sémantique des accès aux champs et méthodes ;
- ajout des méthodes avec référence cachée `soi/this`, appels par `.` et `->`,
  constructeurs sur variables locales et destructeurs sans paramètre explicite ;
- ajout de `T&` pour les paramètres et variables locales, liaison obligatoire à
  une valeur gauche, représentation ABI 64 bits et marqueur `R` dans les
  signatures de compatibilité ;
- ajout des surcharges de fonctions et constructeurs avec résolution par types,
  adaptation bornée des constantes et noms de liaison déterministes ;
- ajout des surcharges d’opérateurs unaires et binaires du sous-ensemble
  système ;
- ajout du RAII local : destruction en ordre inverse à la sortie de bloc, des
  branches, des itérations et avant les retours, sans runtime ni exceptions ;
- ajout des méthodes virtuelles optionnelles, d’un pointeur de table au
  décalage zéro, de tables locales relocalisées et des appels indirects ;
- extension du classificateur auto-hébergé de 63 à 79 cas pour couvrir les
  nouveaux mots-clés objet sans renuméroter les jetons historiques ;
- test GsE exécuté couvrant classe, accès privé, référence, surcharge,
  constructeur, destructeur, opérateur et virtuel avec code de retour `25` ;
- test objet en compilation séparée exécuté avec retour `42`, contrôle de
  l’ordre des tables virtuelles entre unités et rejet ABI si cet ordre diverge ;
- validation propre 4/4 sous MSVC, 5/5 sous GNU/WSL et démarrage réel de
  Sanctuaire SE sous QEMU 11.0.50/OVMF ;
- lanceur de validation UEFI rendu compatible avec PowerShell 7, avec repli
  vers Windows PowerShell ;
- conservation stricte de `GSOBJ:0`, GsObj/GsA/GsE 1.0 et
  `GsAbi:x64-ms-v1`/ABI 1.

## Compilateur Gs++ 0.17.1 — 2026-08-15

- remplacement définitif de la signature objet locale `GSOBJ\0` par les sept
  octets `GSOBJ:0`, suivis d’un seul octet réservé nul ; l’en-tête GsObj reste
  aligné sur 112 octets et conserve ses champs aux mêmes positions ;
- fixation de tous les formats natifs sur leur première base canonique :
  GsObj 1.0, GsA 1.0 et GsE 1.0 ;
- renumérotation sans perte fonctionnelle de l’ABI complète actuelle en ABI 1,
  pour les objets GsObj, les imports GsE et les signatures textuelles
  `GsAbi:x64-ms-v1` ;
- refus explicite de `GSOBJ\0`, de la cible intermédiaire `GSO:0`, des GsE
  portant l’ancienne version locale 2.0 et des champs ABI locaux 2 ;
- aucune couche de compatibilité ni outil de conversion : tous les artefacts
  étant locaux et reconstruisibles, les anciens GsObj, GsA et GsE sont régénérés ;
- extension des tests binaires sur les octets exacts des en-têtes GsObj/GsA,
  les versions GsE, les signatures ABI et les rejets d’anciens artefacts ;
- reconstruction de `GsSysteme.GsA`, `GsHebergee.GsA`, des images
  auto-hébergées et de `Noyau.GsE` avec la base canonique 1.0.

## Compilateur Gs++ 0.17.0 — 2026-07-23

- ajout des littéraux chaîne UTF-8 avec échappements, terminaison nulle,
  stockage local déterministe dans `.data` et relocalisations inter-sections ;
- qualification `constante caractère*` des littéraux et refus des écritures
  directes dans leurs octets ;
- ajout de `&&` et `||` avec priorités dédiées, évaluation constante et
  génération x86-64 à court-circuit ;
- ajout de la bibliothèque native hébergée `GsHebergee.GsA`, sans allocation
  cachée, avec vues texte, flux mémoire bornés, vecteurs et tables de symboles
  à stockage fourni par l’appelant ;
- contrat hôte structuré pour lire et écrire des fichiers et publier des
  diagnostics avec fichier, ligne, colonne et niveau ;
- exposition du classificateur C++ de référence et première réécriture d’un
  composant du compilateur dans
  `GsPlusPlus/AutoHebergement/ClassificateurMotsCles/ClassificateurMotsCles.GsPP` ;
- compilation et exécution réelles du classificateur sous forme GsE, avec
  comparaison de 63 mots-clés, alias et non-mots contre le lexeur C++ ;
- exécution du test de la bibliothèque hébergée par trois imports résolus,
  vérifiant également le court-circuit par deux branches qui contiennent une
  division par zéro non évaluée ;
- construction Make/CMake, reproductibilité GsA, vérifications GsE et maintien
  de Sanctuaire SE 0.10.2 comme non-régression UEFI.

## Compilateur Gs++ 0.16.0 — 2026-07-22

- ajout des opérateurs entiers `~`, `&`, `^`, `|`, `<<` et `>>`, avec
  priorités dédiées et évaluation constante cohérente avec le backend ;
- définition des distances de décalage modulo la largeur 8/16/32/64 bits et
  distinction entre décalage droit logique et arithmétique ;
- ajout de douze intrinsèques x86-64 typées pour les charges, stockages,
  échanges, ajouts, comparaison-échange, barrière et pause ;
- génération directe de `xchg`, `lock xadd`, `lock cmpxchg`, `mfence` et
  `pause`, sans symbole d’import ni relocalisation ;
- validation stricte des prototypes réservés d’intrinsèques ;
- suppression des imports artificiels provenant de prototypes d’interface
  déclarés mais non référencés ;
- première bibliothèque native `GsSysteme.GsA`, composée des modules mémoire,
  vues/texte, bits et atomiques ;
- API française `Gs::Systeme`, alias anglais `Gs::System`, vues structurées et
  verrou atomique léger ;
- test de liaison et d’exécution de la bibliothèque couvrant les chemins 32 et
  64 bits avec le code de retour `64` ;
- test de reproductibilité de l’archive GsA et maintien de Sanctuaire SE
  0.10.2 comme non-régression UEFI.

## Compilateur Gs++ 0.15.0 — 2026-07-22

- ajout des initialisations agrégées `{...}` imbriquées pour structures, unions,
  tableaux fixes et scalaires contextualisés ;
- mise à zéro déterministe des éléments omis et des octets de bourrage ;
- sérialisation des agrégats globaux et relocalisations `Adresse64` de pointeurs
  de fonction placés dans un champ imbriqué ;
- copies et affectations complètes de structures et d’unions, sans partage du
  stockage source ;
- passage des structures par valeur au moyen d’une copie dans le cadre local du
  destinataire ;
- retour de structures dans une zone cachée fournie par l’appelant, pour les
  appels directs, indirects et inter-unités ;
- zones temporaires fixes dans le cadre de pile pour les agrégats et résultats
  structurés, y compris dans les expressions imbriquées ;
- nouvelle ABI `GsAbi:x64-ms-v2`, ABI GsObj 2 et ABI d’import GsE 2 ; les objets
  natifs produits avant la 0.15 doivent être recompilés ;
- ajout du programme `ValeursStructures.GsPP`, exécuté avec le retour `45`, et
  d’un test de liaison de deux GsObj échangeant une structure, retour `46` ;
- diagnostics pour les agrégats trop longs, types de copies incompatibles,
  unions multiéléments et fonctions à retour structuré dépassant trois
  paramètres explicites.

## Compilateur Gs++ 0.14.0 — 2026-07-22

- ajout du type bilingue `pointeur_fonction<Retour(Paramètres)>` / `function_pointer<Return(Parameters)>` ;
- prise d’adresse explicite avec `&Fonction` et conversion implicite d’un nom de fonction vers sa signature typée ;
- appels indirects depuis une variable locale, une globale, un paramètre, un champ, un tableau ou une valeur retournée ;
- vérification statique du nombre et du type des paramètres, du retour et des affectations de callbacks ;
- intégration récursive des signatures de fonctions dans le contrat ABI des objets GsObj et dans le contrôle de liaison ;
- génération x86-64 Microsoft ABI des appels indirects avec sauvegarde de la cible et `call r11` ;
- initialisation des pointeurs de fonction globaux par une fonction définie, avec relocalisation interne GsE `BASE64` ;
- application de `BASE64` par le chargeur hébergé et `BOOTX64.EFI`, et validation stricte de sa source, de sa cible et de son indice réservé ;
- nouveau programme d’intégration couvrant structures, tableaux, paramètres, retours, globales et déréférencement de callbacks, avec code de retour `44` ;
- génération des dépendances Make afin qu’une modification d’en-tête reconstruise automatiquement tous les objets concernés.

## Compilateur Gs++ 0.13.2 — 2026-07-22

- remplacement de la signature objet provisoire `GSO\0` par la signature explicite `GSOBJ\0` ;
- ajout de deux octets réservés nuls après la signature afin de conserver un en-tête aligné de 112 octets ;
- déplacement coordonné des champs de version, d’architecture, de tailles et de positions dans l’en-tête ;
- contrôle strict de la signature complète et des octets réservés par le lecteur ;
- ajout de tests binaires et de corruption couvrant la nouvelle disposition ;
- les objets `.GsObj` 0.13.1 doivent être recompilés, le format n’ayant pas encore été publié.

## Compilateur Gs++ 0.13.1 — 2026-07-22

- remplacement de l’extension courte d’interface `.GsPH` par `.GsPPH` ;
- remplacement de l’extension de fichier objet natif `.GsO` par `.GsObj` ;
- ajout de la forme canonique `--format gsobj` et mise à jour des sorties par défaut ;
- mise à jour des projets, solutions, exemples, diagnostics et tests de reproductibilité ;
- conservation du format binaire interne GsO 1.0 et de sa signature `GSO\0` : seule la convention de nommage des fichiers change.

## Compilateur Gs++ 0.13.0 — 2026-07-22

- ajout des interfaces `.GsPH` et `.GsPlusPlusHeader`, dont les prototypes et globales sont des déclarations externes sans mot-clé obligatoire ;
- ajout du format objet natif versionné `GsO 1.0`, avec code, données, zéro, symboles, relocalisations, signatures ABI et positions source ;
- ajout des bibliothèques statiques natives `GsA 1.0` et extraction à la demande des seuls membres nécessaires ;
- ajout de l’éditeur de liens multi-unités, du masquage des symboles locaux et du contrôle des définitions publiques dupliquées ;
- vérification stricte de l’ABI Microsoft x64 entre déclarations et définitions, y compris le genre de symbole et la disposition récursive des structures ;
- diagnostics d’incompatibilité ABI indiquant les deux fichiers, lignes et colonnes concernés ;
- ajout des cartes de liens `--carte`, avec adresse, visibilité, nature, source et signature ABI de chaque symbole ;
- ajout des projets `.GsPj`/`.GsProject` et des solutions `.GsPs`, avec construction ordonnée d’objets, bibliothèques et exécutables ;
- prise en charge des variables globales externes dans le langage et les objets natifs ;
- conservation du mode COFF/GsE monolithique et de Sanctuaire SE 0.10.2 comme tests de non-régression ;
- ajout de tests de corruption GsO/GsA, de reproductibilité, d’extraction statique, de liaison réelle et d’incompatibilité ABI.

## Compilateur Gs++ 0.12.0 — 2026-07-22

- ajout des entiers signés `entier8`, `entier16`, `entier32`, `entier64` et de leurs alias `int8` à `int64` ;
- ajout des entiers non signés `naturel8`, `naturel16`, `naturel32`, `naturel64` et de leurs alias `uint8` à `uint64` ;
- littéraux décimaux couvrant toute la plage de `naturel64`, y compris les deux bornes 64 bits ;
- nouveaux types distincts `booléen`/`bool`, `octet`/`byte` et `caractère`/`char` ;
- charges, stockages, paramètres et retours x86-64 adaptés aux largeurs 1, 2, 4 et 8 octets avec extension signée ou nulle ;
- sélection signée ou non signée des divisions, restes et comparaisons ;
- normalisation des résultats arithmétiques à la largeur déclarée ;
- ajout des qualificateurs `constante`/`const` et `volatile`, avec refus des affectations qui retireraient la constance ;
- ajout des tableaux fixes multidimensionnels pour les locales, globales et champs, avec indexation mise à l’échelle ;
- ajout des énumérations portées à base `entier32` et des unions à champs superposés ;
- ajout des conversions `convertir<T>`/`cast<T>`, avec contrôle statique des constantes hors plage ;
- sérialisation des globales selon leur taille réelle, de 1 à 8 octets ;
- documentation des tailles, alignements, règles de débordement et de l’ABI Microsoft x64 ;
- nouveau programme d’intégration `TypesSysteme.GsPP`, exécuté réellement avec le code de retour `120` ;
- conservation du noyau Sanctuaire SE 0.10.2 comme test de non-régression, avec code de retour hébergé `5` ;
- reprise de la correction Windows fournie : `NOMINMAX`, `WIN32_LEAN_AND_MEAN` et conversion explicite de `size_t` vers `uint32_t` ;
- correction associée du contexte hébergé : `NombrePlagesMemoireLibres` est de nouveau initialisé, au lieu d’écrire deux fois `CapacitePlagesMemoire`.

## Compilateur Gs++ 0.11.0 — 2026-07-22

- séparation du cycle de versions du compilateur et de celui de Sanctuaire SE, dont la référence UEFI reste 0.10.2 ;
- ajout du mot-clé `alias` et d’une déclaration applicative à cible qualifiée ;
- résolution anticipée des alias entre tous les fichiers d’une même compilation ;
- prise en charge des alias de fonctions, variables globales, structures et champs ;
- normalisation des chaînes d’alias vers une déclaration canonique unique ;
- détection des cycles, cibles absentes, doublons, conflits et cibles ambiguës ;
- émission COFF et GsE de plusieurs symboles publics partageant exactement le même code ou stockage ;
- possibilité d’utiliser un alias public comme point d’entrée GsE ;
- normalisation des appels, globales et champs avant génération des relocalisations ;
- absence de second import obligatoire lorsqu’un appel utilise l’alias d’une fonction externe ;
- remplacement des fonctions-ponts anglaises du noyau de référence par de véritables alias ;
- réduction du noyau de référence de 13 612 à 12 305 octets de code et de 271 à 239 relocalisations ;
- ajout des alias anglais `BootContext`, `MemoryPage`, `PhysicalMemoryRange` et des champs du contrat de démarrage ;
- validation unitaire des fonctions, globales, structures, champs, chaînes, exports et erreurs d’alias ;
- conservation sans modification du format exécutable GsE 2.0.

## 0.10.2 — 2026-07-22

- passage volontairement incompatible au format exécutable GsE 2.0 ;
- remplacement des noms fixes de 40 octets pour les exports et de 48 octets pour les imports par la section UTF-8 `.chaines` ;
- entrées `.imports` et `.exports` compactées à 32 octets avec position et longueur explicites ;
- noms de symboles limités défensivement à 1 024 octets UTF-8, terminaison nulle exclue ;
- vérification des positions, longueurs, débuts de chaînes, terminaisons, encodages UTF-8 et doublons ;
- mise à jour coordonnée du compilateur, du vérificateur, du chargeur hébergé et du chargeur UEFI ;
- restauration des API françaises complètes `NombreOctetsTasUtilises`, `InitialiserInterruptions` et `LireDernierCodeClavier` ;
- tests de la borne exacte de 1 024 octets, du refus à 1 025 octets et de l’exécution hébergée du noyau ;
- les fichiers GsE 1.1 locaux doivent être recompilés avec la chaîne 0.10.2.

## 0.10.1 — 2026-07-22

- correction critique du tas : les données commencent désormais après les 32 octets complets de l’en-tête `PageMemoire`, sans recouvrir le champ interne `Etat` ;
- conservation d’un alignement de 8 octets pour l’adresse retournée et correction du calcul du nombre de pages avec les 32 octets de métadonnées ;
- refus d’une allocation qui ferait déborder le compteur signé des octets actifs ;
- libération rendue transactionnelle : un échec de `LibererPages` ne retire plus le bloc de la liste du tas et ne fausse plus ses compteurs ;
- initialisation du tas rendue idempotente afin qu’un second appel ne perde pas les allocations actives ;
- autotests renforcés avec écriture dans le deuxième mot, contrôle de la fin d’une allocation de 5000 octets, échec forcé de libération puis nouvelle tentative réussie ;
- renommage des exports français trop longs en `OctetsTasUtilises`, `InitialiserIrq` et `LireCodeClavier`, avec conservation de leurs alias anglais ;
- contrôle global garantissant que tous les noms publics du noyau respectent la limite du format GsE ;
- reconstruction du noyau, de `BOOTX64.EFI` et de l’image ESP ; validation UEFI réelle de cette image corrective encore requise.

## 0.10.0 — 2026-07-18

- contrat `ContexteDemarrage` version 3 étendu de façon ascendante de 288 à 320 octets avec taille de page, quantité de tables, couverture de pagination, pages récupérées, limite et capacité des plages ;
- récupération après `ExitBootServices` des régions UEFI `BootServicesCode` et `BootServicesData`, avec exclusion intégrale du descripteur contenant la pile active et de toute région recoupant l’image du chargeur ;
- conservation de toutes les allocations Loader critiques hors des plages publiées au noyau ;
- nouvelle structure Gs++ `PageMemoire` de 32 octets, soit exactement 128 éléments par page de 4 Kio ;
- allocateur physique Gs++ contigu avec `AllouerPages`/`AllocatePages`, `LibererPage`/`FreePage` et `LibererPages`/`FreePages` ;
- réutilisation en premier des plages libérées, découpage des plages plus grandes et fusion répétée des voisines physiques ;
- refus des tailles de libération incohérentes et des doubles libérations, avec compteurs total, libre et alloué ;
- premier tas noyau Gs++ multi-page avec initialisation, allocation, libération, compte des allocations et octets actifs ;
- nouveau module `Pagination.GsPP` validant la racine, la taille de page, les tables, la couverture et l’absence de la page nulle ;
- API mémoire et pagination françaises canoniques avec alias anglais de même comportement ;
- autotest noyau des trois sous-systèmes et témoin framebuffer vert en cas de réussite complète ;
- chargeur hébergé étendu pour exécuter réellement les allocations mono/multi-pages, la fusion, une allocation de tas de 5000 octets, la libération, le refus de double libération, les statistiques et les bornes de pagination ;
- code de retour hébergé du noyau porté à `5` pour le contrat version 3 ;
- tests unitaires, intégration GsE, PE/UEFI, FAT32 et reproductibilité réussis ;
- statut : image prête pour un démarrage UEFI réel, validation visuelle 0.10.0 encore requise.

## 0.9.1 — 2026-07-18

- promotion de la rc3 après un démarrage UEFI réel réussi sous QEMU 11.0.50 et OVMF ;
- validation de la chaîne `UEFI → BOOTX64.EFI → Noyau.GsE → code noyau Gs++` ;
- confirmation visuelle du bandeau « GS » et du témoin cyan animé par les interruptions d’horloge ;
- image ESP candidate exactement testée : SHA-256 `76804ffb0b67aa1d13ab375b92d4ddc0e8255cfc9a842c9c99757be45e7c9040` ;
- conservation intégrale de la correction IRQ rc3 fondée sur `GsVecteursInterruption` et les relocalisations PE `DIR64` ;
- aucune modification fonctionnelle depuis la rc3 : seuls les libellés de version, les métadonnées et la documentation de publication changent ;
- reconstruction, tests automatisés et comparaison binaire des sections exécutables avant publication finale.

## 0.9.1-rc3 — 2026-07-18

- prise en compte du test UEFI réel de la rc2, qui reproduit `#GP(0)` avant toute entrée dans le gestionnaire IRQ ;
- identification de la cause exacte : `R_X86_64_REX_GOTPCRELX` chargeait le contenu du stub au lieu de son adresse lors de la liaison ELF vers PE ;
- ajout de `GsVecteursInterruption`, table assembleur de cinq pointeurs couverts par les relocalisations PE `DIR64` ;
- construction des portes IDT matérielles uniquement depuis cette table, sur le même modèle que les 32 exceptions fonctionnelles ;
- test de non-régression refusant toute relocalisation `GOTPCREL` d’un stub IRQ et exigeant les cinq relocalisations absolues ;
- conservation des protections rc2 : `IST2`, cadre dans `R15`, validation `RIP/CS/RFLAGS` et diagnostic `61` à `64` ;
- statut candidat : validation UEFI réelle encore requise.

## 0.9.1-rc2 — 2026-07-18

- prise en compte du test UEFI réel de la rc1 : `#GP(0)` à l’étape 06, avec un faux `RIP` égal aux huit premiers octets du stub IRQ ;
- séparation de la pile IST en `IST1` pour la double faute et `IST2` pour les interruptions matérielles ;
- conservation du pointeur exact des registres IRQ dans `R15`, sans case voisine de l’espace d’accueil Microsoft x64 ;
- transmission et validation du cadre matériel `RIP/CS/RFLAGS` avant et après le répartiteur Gs++ ;
- sous-étapes IRQ `61` à `64` et diagnostic complémentaire du dernier cadre dans le framebuffer et sur COM1 ;
- extension ascendante de `ContexteDemarrage` de 256 à 288 octets, sans modifier les champs précédents ;
- statut candidat : validation UEFI réelle encore requise.

## 0.9.1-rc1 — 2026-07-18

- prise en compte du test UEFI réel de la 0.9.0, arrêté par une exception processeur après le transfert ;
- extension ascendante de `ContexteDemarrage` de 232 à 256 octets ;
- capture de l’étape de démarrage, de `RIP` et de `CR2`, en plus du vecteur et du code d’erreur ;
- diagnostic hexadécimal visible dans le framebuffer et sortie complémentaire sur COM1 ;
- initialisation du noyau Gs++ sur sa pile dédiée avant la configuration APIC/PIT/PS2 ;
- suppression de la réécriture inutile de `IA32_APIC_BASE` lorsque xAPIC est déjà actif à l’adresse MADT ;
- conservation intégrale des 232 premiers octets du contrat 0.9.0 ;
- statut candidat : validation UEFI réelle encore requise.

## 0.9.0 — 2026-07-18

- contrat `ContexteDemarrage` version 2 étendu à 232 octets avec état des piles, APIC, horloge, clavier et IRQ ;
- pile noyau dédiée de 64 Kio et pile IST de 32 Kio, chacune protégée par une page garde non présente ;
- TSS x86-64 de 104 octets avec `RSP0`, `IST1`, descripteur système GDT et chargement par `ltr` ;
- utilisation de l’IST pour la double faute, vecteur 8 ;
- bascule assembleur définitive de `RSP` avant l’appel du noyau GsE ;
- validation du RSDP, du XSDT/RSDT et du MADT ACPI par sommes de contrôle ;
- découverte des adresses Local APIC, I/O APIC et des redirections d’interruptions ISA ;
- activation xAPIC, masquage du PIC historique et préparation des vecteurs d’erreur et parasite ;
- programmation du PIT à 100 Hz sur le vecteur 32 et initialisation PS/2 sur le vecteur 33 ;
- stubs IRQ sauvegardant et restaurant les 15 registres généraux, avec alignement Microsoft x64 et `iretq` ;
- export obligatoire du répartiteur Gs++ `GererInterruption`, alias `HandleInterrupt` ;
- API bilingues `InitialiserInterruptions`/`InitializeInterrupts`, `LireNombreTicks`/`ReadTickCount` et `LireDernierCodeClavier`/`ReadLastKeyCode` ;
- affichage framebuffer de l’activité horloge et clavier depuis le code Gs++ ;
- activation différée par `sti`, puis attente inactive `hlt` après le retour d’initialisation du noyau ;
- test hébergé réel des deux plages mémoire, de la réserve, de la console et du répartiteur IRQ Gs++ ;
- prise en compte de la validation UEFI réelle de Sanctuaire SE 0.8.0 dans une machine virtuelle.

## 0.8.0 — 2026-07-18

- contrat `ContexteDemarrage` étendu à 136 octets et nouvelle structure bilingue `PlageMemoirePhysique`/`PhysicalMemoryRange` ;
- normalisation de la carte UEFI finale en plages de mémoire conventionnelle de 4 Kio ;
- allocateur Gs++ capable de traverser plusieurs plages puis la réserve de secours ;
- test hébergé couvrant deux plages distinctes et le basculement vers la réserve ;
- création de tables de pages x86-64 indépendantes du micrologiciel ;
- cartographie identitaire adaptée à la largeur physique du processeur, plafonnée à 512 Gio ;
- page virtuelle nulle non présente ;
- détection CPUID de NX, activation de `EFER.NXE` et de `CR0.WP` ;
- protections page par page RX, RW+NX ou R+NX d’après les sections PE et segments GsE ;
- chargement du nouveau `CR3` après `ExitBootServices` ;
- allocation des fondations critiques sous la limite de pagination ;
- 32 stubs assembleur distincts pour les exceptions x86-64, avec normalisation des codes d’erreur ;
- diagnostic d’exception conservant vecteur et code dans le contexte puis affichant un bandeau framebuffer ;
- point d’entrée Gs++ optionnel `TesterDivisionZero`/`TestDivideByZero` pour valider le vecteur 0 ;
- contrôles statiques de `rdmsr`, `wrmsr`, `CR0`, `CR3` et des 32 symboles d’exception ;
- prise en compte de la validation réelle du démarrage UEFI de Sanctuaire SE 0.7.

## 0.7.0 — 2026-07-18

- indexation native des pointeurs avec `pointeur[indice]`, utilisable en lecture et en affectation ;
- calcul x86-64 de l’adresse indexée selon la taille réelle de l’élément ;
- contrat `ContexteDemarrage` étendu à 104 octets avec réserve de pages, GDT et IDT ;
- allocation UEFI d’une réserve physique de 16 Mio, avec replis à 4 Mio puis 1 Mio ;
- allocateur monotone Gs++ de pages de 4 Kio et alias anglais `AllocatePage` ;
- primitives framebuffer Gs++ et bandeau graphique de diagnostic au démarrage ;
- GDT 64 bits et IDT de secours à 256 portes installées après `ExitBootServices` ;
- arrêt d’urgence déterministe pour les exceptions avant les futurs gestionnaires détaillés ;
- correction de l’appel Microsoft x64 du chargeur hébergé GNU avec espace d’accueil stable ;
- test hébergé réel de l’allocation de page et du rendu framebuffer ;
- contrôles statiques des instructions `lgdt`, `lidt` et `lretq` dans le PE EFI ;
- noyau Gs++ désormais réparti entre contexte, mémoire, console et point d’entrée.

## 0.6.0 — 2026-07-18

- contrat `ContexteDemarrage` de 72 octets et alias `BootContext` ;
- définition Gs++ correspondante avec disposition mémoire identique ;
- point d’entrée noyau recevant le contexte selon l’ABI Microsoft x64 ;
- mode hébergé `--executer-noyau` et alias `--execute-kernel` ;
- application freestanding `BOOTX64.EFI` écrite en C++ sans runtime externe ;
- définitions françaises minimales des protocoles UEFI avec types anglais aliasés ;
- lecture de `Noyau.GsE` depuis le volume EFI ;
- validation bornée, allocation et copie des segments GsE ;
- découverte du framebuffer GOP et des tables ACPI ;
- acquisition de la carte mémoire et appel robuste de `ExitBootServices` ;
- production PE32+ x86-64 relogeable et déterministe, sans DLL ni CLR ;
- constructeur autonome d’image ESP FAT32 avec options françaises et anglaises ;
- relecture et comparaison des fichiers depuis les chaînes de clusters FAT32 ;
- tests de reproductibilité du chargeur EFI et de l’image de démarrage.

## 0.5.0 — 2026-07-18

- API `ChargeurGsE` séparée du compilateur et du vérificateur ;
- création en mémoire des segments texte, données et zéro ;
- résolution par rappel des imports GsE obligatoires ;
- application des relocalisations d’import `REL32` et `Adresse64` ;
- calcul des adresses relogées du point d’entrée et des exports ;
- nouvel outil français `gsechargeur` pour charger et inspecter une image, avec `gseload` comme alias anglais ;
- allocation native et protections mémoire W^X pour l’exécution hébergée ;
- exécution explicite des points d’entrée sans argument avec `--executer`/`--execute` ;
- premier noyau de diagnostic écrit en Gs++ ;
- durcissement du vérificateur sur la taille d’image, les tables et les segments ;
- tests de chargement, de résolution d’import et d’exécution du noyau.

## 0.4.0 — 2026-07-18

- variables globales initialisées et initialisées à zéro ;
- déclarations `externe` et alias `extern` pour les fonctions importées ;
- backend généralisé en sections texte, données et zéro ;
- objets COFF AMD64 avec `.text`, `.data`, `.bss` et relocalisations par section ;
- format exécutable GsE 1.1 avec plusieurs segments et séparation W^X ;
- tables d’imports, d’exports et de relocalisations d’import ;
- métadonnées GsC configurables depuis la ligne de commande ;
- vérificateur autonome `gseverifier` avec contrôles de structure et de sécurité ;
- tests unitaires et d’intégration couvrant les symboles globaux, imports et GsE corrompus ;
- documentation française mise à jour avec les alias anglais.

## 0.3.0 — 2026-07-18

- compilation et fusion de plusieurs fichiers sources ;
- structures à disposition séquentielle ;
- calcul des tailles, alignements et décalages de champs ;
- types pointeurs fondamentaux et opérateurs `&` et `*` ;
- accès aux membres avec `.` et `->` ;
- analyse sémantique séparée du parseur ;
- résolution des types et symboles entre plusieurs fichiers ;
- vérification des affectations, arguments et valeurs de retour ;
- diagnostics contenant le fichier, la ligne et la colonne ;
- stockage ABI correct de `entier32` sur 4 octets et des pointeurs sur 8 octets ;
- premier écrivain-lieur natif `.GsE` ;
- résolution directe des appels internes dans le segment GsE ;
- sélection explicite ou automatique du point d’entrée.

## 0.2.0 — 2026-07-18

- paramètres de fonctions, jusqu’à quatre dans le prototype ;
- variables locales et affectations ;
- comparaisons `==`, `!=`, `<`, `<=`, `>` et `>=` ;
- opérateur unaire `!` ;
- conditions `si/sinon` et alias `if/else` ;
- boucles `tantque` et alias `while` ;
- appels de fonctions avec respect de l’ABI Shrine x86-64 ;
- espace d’accueil de 32 octets et alignement de pile avant les appels ;
- relocalisations COFF `IMAGE_REL_AMD64_REL32` ;
- symboles externes non définis ;
- tests d’intégration bilingues étendus.

## 0.1.0 — 2026-07-18

- premier lexer Gs++ avec validation UTF-8 ;
- normalisation des mots-clés français et de leurs alias anglais ;
- premier analyseur syntaxique et AST ;
- prise en charge des espaces de noms et fonctions simples ;
- expressions entières avec priorité des opérateurs ;
- génération directe d’instructions x86-64 ;
- production d’objets COFF AMD64 déterministes ;
- diagnostics localisés en français et anglais ;
- tests unitaires et tests d’intégration ;
- construction GNU Make et CMake, notamment pour Visual Studio.
