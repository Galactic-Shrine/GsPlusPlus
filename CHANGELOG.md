# Réorganisation du monorepo — 2026-07-23

- séparation du compilateur, du SDK, des bibliothèques et de l’auto-hébergement ;
- déplacement de Sanctuaire SE dans un projet indépendant ;
- ajout du mode de projet agrégé, désormais exprimé par l’attribut XML
  `ModeCompilation="agregee"`, pour les programmes
  monolithiques composés de plusieurs fichiers, utilisé par le noyau ;
- séparation des tests Gs++ et Sanctuaire SE ;
- centralisation des résultats dans `Construction/`.

# Journal des modifications

## Priorité des conversions constantes et types par fichier — développement après Gs++ 0.27.0-alpha.10 — 2026-10-04

### Français

- calcul et contrôle de plage des conversions constantes au moment où leur
  expression est visitée, après validation du type cible, de la source et des
  signatures ; suppression de la passe globale tardive devenue redondante ;
- régression reproduite puis corrigée : `convertir<naturel8>(256); objet + 7;`
  signalait l’accès privé 25 au lieu du dépassement de plage 98 à la conversion ;
- priorités inverses, opérandes, conversions imbriquées, appels directs,
  callbacks, adresses de fonctions, arité, abandon de candidats et agrégats
  contextuels comparés au bootstrap ; déclarations locales, globales, champs
  par défaut et énumérations couvertes ; booléens, bornes, courts-circuits et
  conversions non constantes restent acceptés dans les cas valides ;
- 64 nouveaux refus sémantiques français/anglais, 24 corpus sémantiques valides,
  12 refus d’émission sans écriture partielle et quatre corpus d’émission valides ;
  total différentiel : **1 493 refus**, avec code, ligne, colonne et AST intact ;
- CTest Windows 5/5, GNU/Linux 6/6, solution et validation natives MSBuild,
  conformité 20/20 sur les trois constructions ; frontend identique de
  408 975 octets, 75 exports et deux imports, accepté par le vérificateur GsE ;
- exemple `TypesParFichier` : `Point.HGsPP`, `Etat.HGsPP` et `Principal.GsPP`,
  projet XML à interfaces explicites et exécution retournant 42 ; intégré au
  test Linux, expliqué dans la spécification et les README français/anglais ;
- bootstrap C++, contrats publics, diagnostics 0–119, formats 1.0 et ABI 1
  inchangés ; priorités des autres initialiseurs et contextes des constructions
  encore à consolider ; aucun frontend 0.27 complet annoncé ;
- alpha.10 inchangée ; ces tranches de développement ne font pas partie des
  paquets alpha.10 publiés ; aucun nouveau tag, paquet ou release.

### English

- evaluate and range-check constant casts when their expression is visited,
  after target, source and signature checks; remove the redundant late global pass;
- reproduce and fix `cast<uint8>(256); object + 7;`, which reported private
  access 25 instead of out-of-range constant cast 98 at the conversion;
- compare reverse priorities, operands, nested casts, direct calls, callbacks,
  function addresses, arity, candidate abandonment and contextual aggregates
  against the bootstrap; cover locals, globals, default fields and enumerations;
  retain valid booleans, boundaries, short circuits and nonconstant casts;
- 64 new French/English semantic rejections, 24 valid semantic corpora,
  12 emission rejections without partial writes and four valid emission corpora;
  differential total **1,493 rejections**, checking code, line, column and intact AST;
- Windows CTest 5/5, GNU/Linux 6/6, native MSBuild solution and validation,
  20/20 conformance on all three builds; identical 408,975-byte frontends,
  75 exports and two imports, accepted by the GsE verifier;
- add `TypesParFichier`: separate `Point.HGsPP`, `Etat.HGsPP` and `Principal.GsPP`,
  explicitly listed XML interfaces and an entry point returning 42; Linux
  integration coverage, specification and French/English README instructions;
- C++ bootstrap, public contracts, diagnostics 0–119, formats 1.0 and ABI 1
  unchanged; other initializer priorities and construction contexts still need
  consolidation; no complete 0.27 frontend claimed;
- alpha.10 unchanged; these development tranches are not included in the
  published alpha.10 packages; no new tag, package or release.

## Arguments agrégés contextuels — développement après Gs++ 0.27.0-alpha.10 — 2026-10-04

### Français

- report du contenu des arguments `{…}` jusqu'à l'obtention du type attendu ;
  sélection et visibilité avant l'initialisation pour les groupes directs,
  initialisation dans l'ordre des arguments pour les callbacks et adresses
  explicites de fonctions ; ambiguïtés et refus de références conservés ;
- régression corrigée : `Scalaire({1, 2}, 0)` était accepté par le frontend
  (code 4 de demande de capacité), alors que le bootstrap exige le refus 44 ;
- validation de la forme scalaire, structure, union et tableau de champ avant
  leurs éléments, puis résolution et validation de chaque feuille dans l'ordre ;
  plages numériques, références, classes non agrégeables, alias, méthodes non
  liées, groupes mixtes et appels imbriqués couverts dans la matrice ;
- mémorisation privée des agrégats validés dans la table de types implicites
  existante, sans allocation supplémentaire ni duplication de résolutions
  lors des contrôles de préfixes et de l'appel complet ; réutilisation du
  validateur récursif d'initialiseurs pour les agrégats imbriqués ;
- 66 nouveaux refus sémantiques français/anglais, 36 corpus sémantiques valides,
  12 refus d'émission sans écriture partielle et quatre corpus d'émission valides ;
  total différentiel : **1 417 refus**, code, ligne, colonne et AST intact vérifiés ;
- CTest Windows 5/5, GNU/Linux 6/6, solution et validation natives MSBuild,
  conformité 20/20 sur les trois constructions ; frontend identique de
  409 519 octets, 75 exports et deux imports, accepté par le vérificateur GsE ;
- bootstrap C++, contrats publics, diagnostics 0–119, formats 1.0 et ABI 1
  inchangés ; contextes des constructions et autres priorités entre passes,
  notamment conversions constantes et initialiseurs de déclarations, encore
  à compléter ; aucun frontend 0.27 complet annoncé ;
- alpha.10 inchangée, tranches locales non commitées et non publiées ; aucun
  nouveau push, tag, paquet ou release. La migration `.Glib` reste prévue en 0.28.

### English

- defer `{…}` argument contents until their expected type is known; selection
  and visibility precede initialization for direct groups, while callbacks and
  explicit function addresses initialize arguments in source order;
  retain ambiguity and reference-rejection diagnostics;
- fix `Scalaire({1, 2}, 0)`, previously accepted by the frontend (capacity-query
  code 4) although the bootstrap requires rejection 44;
- validate scalar, structure, union and field-array shape before visiting
  elements, then resolve and validate each leaf in source order; cover numeric
  ranges, references, non-aggregate classes, aliases, unbound methods, mixed
  groups and nested calls within the differential matrix;
- cache validated aggregate types privately in the existing implicit-type
  table, without added allocation or duplicate resolutions during prefix and
  complete-call checks; reuse the recursive initializer validator for nesting;
- 66 new French/English semantic rejections, 36 valid semantic corpora,
  12 emission rejections without partial writes and four valid emission corpora;
  differential total **1,417 rejections**, checking code, line, column and intact AST;
- Windows CTest 5/5, GNU/Linux 6/6, native MSBuild solution and validation,
  20/20 conformance on all three builds; identical 409,519-byte frontends,
  75 exports and two imports, accepted by the GsE verifier;
- C++ bootstrap, public contracts, diagnostics 0–119, formats 1.0 and ABI 1
  unchanged; construction contexts and other cross-pass priorities, including
  constant casts and declaration initializers, still need coverage;
  no complete 0.27 frontend claimed;
- alpha.10 unchanged; local tranches remain uncommitted and unpublished, with
  no new push, tag, package or release. The `.Glib` migration stays planned for 0.28.

## Abandon ordonné des candidats d'appel — développement après Gs++ 0.27.0-alpha.10 — 2026-10-04

### Français

- contrôle du préfixe des arguments résolus avant de visiter l'argument suivant,
  pour les fonctions libres, méthodes, groupes mixtes et alias non liés ;
  abandon des candidats incompatibles dans l'ordre de déclaration, sans évaluer
  prématurément les candidats suivants lorsqu'un préfixe reste recevable ;
- arité complète et récepteur toujours contrôlés ; sélection finale, scores,
  ambiguïtés et visibilité conservés ; régression corrigée : diagnostic 25 à
  1:559 dans le second argument au lieu du diagnostic 21 à 1:540 retenu par le
  bootstrap après refus du premier argument ;
- erreurs de types de conversions différées jusqu'à la visite de leur expression,
  avant l'opérande, pour ne pas remplacer le refus préalable d'un candidat ;
  normalisation des types privés et validation des types de déclarations
  conservées ; bootstrap C++ et diagnostics publics inchangés ;
- 56 nouveaux refus sémantiques français/anglais, 16 corpus sémantiques valides,
  12 refus d'émission sans écriture partielle et quatre corpus d'émission valides ;
  total différentiel : **1 339 refus**, code, ligne, colonne et AST intact vérifiés ;
- CTest Windows 5/5, GNU/Linux 6/6, solution et validation natives MSBuild,
  conformité 20/20 sur les trois constructions ; frontend identique de
  405 135 octets, 75 exports et deux imports, accepté par le vérificateur GsE ;
- modification locale de mise en forme de `ConteneursDynamiques.GsPP` conservée ;
  bibliothèque périmée reconstruite avant la nouvelle validation de
  reproductibilité Linux, réussie avec les sources actuelles ;
- aucun nouveau contrat public, format ou ABI ; parcours sans récursion ni
  allocation auxiliaire supplémentaire ; initialiseurs contextuels d'arguments,
  agrégats et autres interactions sémantiques encore à compléter ;
- version alpha.10 inchangée, tranches locales non commitées et non publiées ;
  aucun nouveau push, tag, paquet ou release. Le passage à 0.28 n'est pas validé.

### English

- check the resolved argument prefix before visiting the next argument for free
  functions, methods, mixed groups and unbound aliases; discard incompatible
  candidates in declaration order without prematurely evaluating later
  candidates while a prefix remains acceptable;
- retain full arity and receiver checks, final selection, scores, ambiguities
  and visibility; fix diagnostic 25 at 1:559 in the second argument instead of
  diagnostic 21 at 1:540 selected by the bootstrap after rejecting the first;
- defer cast-type errors until their expression is visited, before its operand,
  so they do not replace earlier candidate rejection; retain private-type
  normalization and declaration-type validation; C++ bootstrap and public
  diagnostics unchanged;
- 56 new French/English semantic rejections, 16 valid semantic corpora,
  12 emission rejections without partial writes and four valid emission corpora;
  differential total **1,339 rejections**, checking code, line, column and intact AST;
- Windows CTest 5/5, GNU/Linux 6/6, native MSBuild solution and validation,
  20/20 conformance on all three builds; identical 405,135-byte frontends,
  75 exports and two imports, accepted by the GsE verifier;
- preserve the local formatting change in `ConteneursDynamiques.GsPP`; rebuild
  the stale library before repeating Linux reproducibility validation,
  which passes with current sources;
- no new public contract, format or ABI; no added recursion or auxiliary
  traversal allocation; contextual argument initializers, aggregates and other
  semantic interactions still need coverage;
- alpha.10 unchanged; tranches remain local, uncommitted and unpublished, with
  no new push, tag, package or release. The transition to 0.28 is not validated.

## Priorité des cibles et arguments d'appel — développement après Gs++ 0.27.0-alpha.10 — 2026-10-04

### Français

- remplacement du parcours inversé des sous-arbres d'appels par le parcours
  itératif des arguments dans l'ordre source, y compris leurs expressions et
  appels imbriqués ; régression sur l'ancienne image : diagnostic 21 à 1:557
  au lieu du diagnostic 25 à 1:532 du premier argument dans le bootstrap ;
- contrôle de la cible indirecte et de son arité avant les arguments, dont
  cibles absentes, non appelables, membres absents ou privés et variable locale
  masquant une fonction ; validation du préfixe déjà parcouru des signatures
  de callbacks avant l'argument suivant ;
- rejet préalable des groupes sans signature d'arité et de récepteur
  recevables, puis sélection de la surcharge et visibilité après les arguments ;
- 60 nouveaux refus sémantiques français/anglais, 16 corpus sémantiques valides,
  12 refus d'émission sans écriture partielle et quatre corpus d'émission
  valides ; total différentiel : 1 271 refus, avec code, ligne, colonne et AST
  intact contrôlés ;
- CTest Windows 5/5, GNU/Linux 6/6, solution et validation natives MSBuild,
  conformité 20/20 sur les trois constructions ; frontend identique de
  401 855 octets, 75 exports et deux imports, accepté par le vérificateur GsE ;
- bootstrap inchangé, aucun nouveau diagnostic, aucune récursion ou allocation
  auxiliaire de parcours ; AST public, formats 1.0, ABI 1 et limite de quatre
  paramètres inchangés ;
- abandon d'un candidat après un premier argument incompatible, initialiseurs
  contextuels d'arguments et priorités entre passes encore à compléter ;
- version alpha.10 inchangée ; nouvelles tranches locales non commitées,
  sans nouveau push, tag, release ou remplacement des paquets publiés.

### English

- replace reverse call-subtree traversal with iterative argument traversal in
  source order, including nested expressions and calls; old-image regression:
  diagnostic 21 at 1:557 instead of diagnostic 25 at 1:532 in the first argument
  selected by the bootstrap;
- check the indirect target and arity before arguments, including unknown and
  non-callable targets, missing or private members, and a local variable hiding
  a function; validate the visited callback-signature argument prefix before
  moving to the next argument;
- reject groups without an acceptable arity and receiver before arguments,
  then defer overload selection and visibility checks until arguments are ready;
- 60 new French/English semantic rejections, 16 valid semantic corpora,
  12 emission rejections without partial writes and four valid emission corpora;
  differential total 1,271 rejections, checking code, line, column and intact AST;
- Windows CTest 5/5, GNU/Linux 6/6, native MSBuild solution and validation,
  20/20 conformance on all three builds; identical 401,855-byte frontends,
  75 exports and two imports, accepted by the GsE verifier;
- bootstrap unchanged, no new diagnostic, recursion or auxiliary traversal
  allocation; public AST, formats 1.0, ABI 1 and four-parameter limit unchanged;
- candidate discard after an incompatible first argument, contextual argument
  initializers and cross-pass priorities still need coverage;
- alpha.10 unchanged; new tranches remain local and uncommitted, with no new
  push, tag, release or replacement of published packages.

## Priorité des erreurs dans les expressions — développement après Gs++ 0.27.0-alpha.10 — 2026-10-04

### Français

- analyse des opérandes de gauche à droite, puis contrôles de l'expression
  parente ; objet avant indice, éléments d'agrégat dans l'ordre source,
  cible d'affectation contrôlée avant sa valeur et type cible de conversion
  avant sa source, conformément au bootstrap dans le périmètre testé ;
- régression reproduite avant correction : un opérateur privé à 1:326 devait
  être refusé avant le récepteur constant de l'opérande droit à 1:353 ;
- parcours itératif fondé sur les indices parents de l'AST préordonné, sans
  pile auxiliaire, allocation supplémentaire ou modification des nœuds ; les
  appels restent des unités conservant leur parcours interne déjà validé ;
- 50 nouveaux refus différentiels français/anglais, 18 corpus sémantiques
  valides dont une chaîne de 128 opérations binaires, 12 refus d'émission sans
  écriture partielle et quatre corpus d'émission valides ; total : 1 199 refus
  avec code, ligne, colonne et AST intact contrôlés ;
- CTest Windows 5/5, GNU/Linux 6/6, solution et validation natives MSBuild,
  conformité 20/20 sur les trois constructions ; frontend identique de
  396 543 octets, 75 exports et deux imports, accepté par le vérificateur GsE ;
- bootstrap, diagnostics 0–119, AST public, formats 1.0 et ABI 1 inchangés ;
  priorités internes des appels et interactions entre passes non couvertes
  encore à compléter ; aucun backend auto-hébergé ou sortie PE/ELF ajouté ;
- le commit signé `6686761` est déjà poussé ; les nouvelles tranches de
  priorité des instructions et expressions restent locales non commitées,
  version alpha.10 inchangée, sans nouveau push, tag, release ou remplacement
  des paquets publiés.

### English

- analyze operands left to right before parent-expression checks; indexed
  object before index, aggregate elements in source order, assignment target
  checked before its value and cast target type before its source, matching
  the bootstrap within the tested scope;
- regression reproduced before the fix: a private operator at 1:326 must be
  rejected before the right operand's const receiver at 1:353;
- iterative traversal using parent indices in the preorder AST, without an
  auxiliary stack, additional allocation or node changes; calls remain units
  retaining their previously validated internal traversal;
- 50 new French/English differential rejections, 18 valid semantic corpora
  including a chain of 128 binary operations, 12 emission rejections without
  partial writes and four valid emission corpora; total 1,199 rejections with
  code, line, column and intact input AST checked;
- Windows CTest 5/5, GNU/Linux 6/6, native MSBuild solution and validation,
  20/20 conformance on all three builds; identical 396,543-byte frontends,
  75 exports and two imports, accepted by the GsE verifier;
- bootstrap, diagnostics 0–119, public AST, formats 1.0 and ABI 1 unchanged;
  internal call priorities and untested cross-pass interactions remain to
  cover; no self-hosted backend or PE/ELF output added;
- signed commit `6686761` is already pushed; the new statement and expression
  priority tranches remain local and uncommitted, alpha.10 unchanged, with no
  new push, tag, release or replacement of published packages.

## Priorité des instructions dans un même corps — développement après Gs++ 0.27.0-alpha.10 — 2026-10-03

### Français

- parcours des instructions successives, blocs imbriqués, expressions de
  conditions, branches et boucles dans l'ordre source ; chaque instruction ou
  expression conserve son parcours interne, avec enfants résolus avant leurs
  contrôles et arguments disponibles avant sélection de la cible d'un appel ;
- test de régression : l'ancienne image signalait le refus du récepteur
  constant à 4:1, au lieu de l'opérateur privé à 3:7 dans le bootstrap ; les
  retours et le code inatteignable restent analysés dans l'ordre du bootstrap ;
- 40 nouveaux refus français/anglais avec priorité de ligne attendue assertée
  dans le bootstrap, 16 corpus valides, 12 refus d'émission sans écriture
  partielle et quatre corpus d'émission valides ; total différentiel : 1 137 ;
- CTest Windows 5/5, GNU/Linux 6/6, solution et validation natives MSBuild,
  conformité 20/20 sur les trois constructions ; frontend identique de
  393 775 octets, 75 exports et deux imports ;
- bootstrap inchangé dans cette tranche, aucun nouveau diagnostic, aucune
  allocation supplémentaire du parcours, AST public, formats 1.0 et ABI 1
  inchangés ; erreurs concurrentes dans une expression et priorités entre
  passes non couvertes encore à étendre ;
- tranche précédente enregistrée dans le commit signé `6686761` ; cette
  nouvelle tranche reste locale non commitée, version alpha.10 inchangée,
  sans push, tag, release ou remplacement des paquets publiés.

### English

- traverse successive statements, nested blocks, condition expressions, branches
  and loops in source order; retain traversal inside each statement or
  expression, resolving children before their checks and arguments before
  selecting a call target;
- regression: the old image reported the const-receiver rejection at 4:1,
  instead of the private operator at 3:7 in the bootstrap; return statements
  and unreachable code retain bootstrap analysis order;
- 40 new French/English rejections with expected line priority asserted in the
  bootstrap, 16 valid corpora, 12 emission rejections without partial writes,
  and four valid emission corpora; differential total 1,137;
- Windows CTest 5/5, GNU/Linux 6/6, native MSBuild solution and validation,
  20/20 conformance on all three builds; identical 393,775-byte frontends,
  75 exports and two imports;
- bootstrap unchanged in this tranche, no new diagnostic or traversal
  allocation; public AST, formats 1.0 and ABI 1 unchanged; competing errors
  within one expression and untested cross-pass priorities remain to cover;
- previous tranche recorded in signed commit `6686761`; this new tranche is
  local and uncommitted, alpha.10 unchanged, with no push, tag, release or
  replacement of published packages.

## Opérateurs mixtes et priorité des groupes invalides — développement après Gs++ 0.27.0-alpha.10 — 2026-10-03

### Français

- sélection des opérateurs unaires et binaires dans le groupe canonique complet,
  membres et fonctions libres compris ; récepteur comparé en première position,
  constance, références, adaptations de constantes, héritage et masquage,
  visibilité après sélection et drapeau `Methode` de la cible réelle ;
- ordre des groupes de surcharges du bootstrap rendu déterministe par leur
  première déclaration, sans dépendre de l'itération d'une table de hachage ;
  priorité des doublons avant les collisions de liaison et corps ; parcours
  auto-hébergé des déclarations dans l'ordre source, sans inverser les fonctions
  indépendantes ni modifier le parcours interne des arguments d'un appel ;
- 38 corpus valides français/anglais vérifiant la cible, le retour et les
  drapeaux ; 26 refus d'opérateurs, 28 refus de priorité et 12 refus d'émission
  sans écriture partielle ; total différentiel : 1 085 ; quatre corpus
  supplémentaires d'émission de données et relocalisations ;
- CTest Windows 5/5, GNU/Linux 6/6, solution et validation natives MSBuild et
  conformité 20/20 sur les trois constructions ; frontend identique de
  392 687 octets, 75 exports et deux imports ; AST public, diagnostics 0–119,
  formats 1.0 et ABI 1 inchangés ;
- trois en-têtes générés `VersionProduit.hpp` restés à alpha.9 régénérés depuis
  `VERSION` : Debug natif Gs++, configurations Windows et Linux du consommateur
  local ; aucune reconstruction de leurs exécutables Debug ou système revendiquée ;
- travail local non commité et non publié ; version et archives alpha.10
  inchangées. Les priorités entre erreurs dans un même corps et entre passes
  non couvertes restent à étendre ; pas de backend auto-hébergé ajouté.

### English

- select unary and binary operators from the complete canonical group,
  including members and free functions; score the receiver as the first
  parameter, with constness, references, constant adaptation, inheritance and
  hiding; check visibility after selection and flag only the actual method;
- make bootstrap overload-group iteration deterministic by first declaration,
  independent of host hash-table order; check duplicate overloads before link
  collisions and bodies; process self-hosted declarations in source order
  without reversing independent functions or changing argument traversal
  inside calls;
- 38 valid French/English corpora check the selected declaration, return type
  and flags; 26 operator rejections, 28 priority rejections and 12 emission
  rejections without partial writes; differential total 1,085; four additional
  data and relocation emission corpora;
- Windows CTest 5/5, GNU/Linux 6/6, native MSBuild solution and validation, and
  20/20 conformance on all three builds; identical 392,687-byte frontends,
  75 exports and two imports; public AST, diagnostics 0–119, formats 1.0 and
  ABI 1 unchanged;
- regenerate three stale alpha.9 `VersionProduit.hpp` headers from `VERSION`:
  native Gs++ Debug and the local consumer's Windows/Linux configurations;
  no claim of rebuilding their Debug or system executables;
- local, uncommitted and unpublished work; alpha.10 version and archives
  unchanged. Priority among errors within one body and untested interactions
  between passes still need coverage; no self-hosted backend added.

## Appels de groupes mixtes — développement après Gs++ 0.27.0-alpha.10 — 2026-10-03

### Français

- sélection sur tout le groupe de fonctions de même nom source complet,
  méthodes non liées et fonctions libres comprises ; appels qualifiés et
  appels par point/flèche alignés sur le bootstrap, sans retenir une première
  méthode ni exclure les fonctions libres du groupe ;
- récepteur évalué comme premier paramètre non lié, avec score de conversion,
  qualifications, héritage et masquage des groupes des bases ; visibilité
  contrôlée après sélection, ambiguïtés conservées et drapeau `Methode` réservé
  aux déclarations de méthodes effectivement choisies ;
- 66 corpus valides français/anglais comparant aussi la déclaration sélectionnée,
  le retour et les drapeaux ; 46 refus différentiels et huit refus d'émission
  sans écriture partielle ; total différentiel : 1 019 ; quatre corpus
  supplémentaires d'émission de données et relocalisations avec appels mixtes ;
- CTest Windows 5/5, GNU/Linux 6/6, solution et validation MSBuild natives et
  conformité 20/20 sur les trois constructions ; frontend identique de
  392 399 octets, 75 exports et deux imports ; bootstrap, AST public,
  diagnostics 0–119, formats 1.0 et ABI 1 inchangés ;
- tranche locale non commitée et non publiée ; version et publication alpha.10
  inchangées. Les combinaisons restantes, notamment les groupes d'opérateurs
  mixtes et les priorités entre groupes invalides indépendants, restent à
  compléter ; cette tranche n'ajoute pas de backend auto-hébergé.

### English

- select from the complete group sharing a fully qualified source name,
  including unbound methods and free functions; qualified, dot and arrow calls
  match the bootstrap without picking the first method or excluding the free
  functions in the group;
- evaluate the receiver as the first unbound parameter, with conversion scoring,
  qualifiers, inheritance and base-group hiding; check visibility after
  selection, retain ambiguities and set the `Method` flag only for a selected
  method declaration;
- 66 valid French/English corpora also compare the selected declaration, return
  type and flags; 46 differential rejections and eight emission rejections
  without partial writes; differential total 1,019; four additional data and
  relocation emission corpora containing mixed calls;
- Windows CTest 5/5, GNU/Linux 6/6, native MSBuild solution and validation, and
  20/20 conformance on all three builds; identical 392,399-byte frontend images,
  75 exports and two imports; bootstrap, public AST, diagnostics 0–119,
  formats 1.0 and ABI 1 unchanged;
- local, uncommitted and unpublished work; alpha.10 version and release unchanged.
  Remaining combinations, including mixed operator groups and diagnostic
  priority across independent invalid groups, still need coverage; no
  self-hosted backend is added in this tranche.

## Collisions de symboles de liaison — développement après Gs++ 0.27.0-alpha.10 — 2026-10-03

### Français

- contrôle des collisions entre noms de liaison calculés pour des surcharges
  distinctes : affichage canonique des paramètres aligné sur `TypeGs::Afficher()`
  et empreinte alignée sur `SuffixeSurcharge`, avec récepteur implicite en
  première position et retour exclu ; nouveau diagnostic bilingue 119 ;
- alias, callbacks imbriqués, qualifications, espaces qualifiés ou imbriqués et
  noms UTF-8 couverts ; contextes de noms et empreintes privés, sans modifier
  l'AST public, les codes 0 à 118 ni les formats 1.0 et l'ABI 1 ;
- collisions réelles vérifiées indépendamment dans les tests, sans modifier
  le bootstrap de référence ; première collision signalée à la fonction
  ultérieure dans l'ordre source, avant les remplacements virtuels et les corps ;
- 60 nouveaux refus différentiels français/anglais, huit refus d'émission sans
  écriture partielle, 24 corpus valides et quatre corpus d'émission valides ;
  total différentiel : 965 ;
- CTest Windows 5/5, GNU/Linux 6/6, solution et validation MSBuild natives et
  conformité 20/20 sur les trois constructions ; frontend identique de
  388 319 octets, 75 exports et deux imports ;
- tranche locale non commitée et non publiée ; version et publication alpha.10
  inchangées. La sélection générale des appels de groupes mixtes, les priorités
  entre groupes invalides indépendants et le backend auto-hébergé restent
  hors du périmètre généralisé.

### English

- check computed link-name collisions between distinct overloads: canonical
  parameter spelling matches `TypeGs::Afficher()` and the fingerprint matches
  `SuffixeSurcharge`, including the implicit receiver as the first parameter
  and excluding the return type; new bilingual diagnostic 119;
- cover aliases, nested callbacks, qualifiers, qualified or nested namespaces
  and UTF-8 names; private naming contexts and fingerprints, preserving the
  public AST, codes 0 through 118, formats 1.0 and ABI 1;
- independently verify real collisions in tests without modifying the reference
  bootstrap; report the first collision at the later function in source order,
  before virtual override and body checks;
- 60 new French/English differential rejections, eight emission rejections
  without partial writes, 24 valid corpora and four valid emission corpora;
  differential total 965;
- Windows CTest 5/5, GNU/Linux 6/6, native MSBuild solution and validation, and
  20/20 conformance on all three builds; identical 388,319-byte frontend images,
  75 exports and two imports;
- local, uncommitted and unpublished work; alpha.10 version and release unchanged.
  General call selection for mixed groups, diagnostic priority across independent
  invalid groups and the self-hosted backend remain outside the generalized scope.

## Signatures non liées — développement après Gs++ 0.27.0-alpha.10 — 2026-10-03

### Français

- correction des collisions méthode/fonction libre de même nom complet dans
  un espace homonyme de la classe : récepteur implicite comparé comme le
  premier paramètre de la signature non liée, diagnostic 118 réutilisé ;
- espaces imbriqués, déclarations anticipées, alias de classe, qualifications,
  références, callbacks, opérateurs et limite d'arité couverts ; retour et
  visibilité exclus de l'identité, sans changer les clés virtuelles ;
- 28 corpus valides français/anglais, 44 refus différentiels et huit refus
  d'émission sans écriture partielle ; total différentiel : 897 ; quatre
  corpus supplémentaires d'émission de données et relocalisations en présence
  de signatures mixtes distinctes ;
- CTest Windows 5/5, GNU/Linux 6/6, solution et validation MSBuild natives et
  conformité 20/20 sur les trois constructions ; frontend identique de
  376 287 octets, 75 exports et deux imports ; AST public, formats 1.0, ABI 1
  et diagnostics existants inchangés ;
- tranche locale non commitée et non publiée ; version et publication alpha.10
  inchangées. Les collisions de noms de liaison calculés, la sélection générale
  des appels de groupes mixtes et le backend auto-hébergé restent hors du
  périmètre validé.

### English

- fix collisions between methods and free functions with the same qualified
  source name in a namespace sharing the class name: compare the implicit
  receiver as the first unbound-signature parameter, reusing diagnostic 118;
- cover nested namespaces, forward declarations, class aliases, qualifiers,
  references, callbacks, operators and the arity limit; return types and
  visibility do not distinguish an overload, without changing virtual keys;
- 28 valid French/English corpora, 44 differential rejections and eight emission
  rejections without partial writes; differential total 897; four additional
  data and relocation emission corpora containing distinct mixed signatures;
- Windows CTest 5/5, GNU/Linux 6/6, native MSBuild solution and validation, and
  20/20 conformance on all three builds; identical 376,287-byte frontend images,
  75 exports and two imports; public AST, formats 1.0, ABI 1 and existing
  diagnostics unchanged;
- local, uncommitted and unpublished work; alpha.10 version and release unchanged.
  Computed link-name collisions, general call selection for mixed groups and
  the self-hosted backend remain outside the validated scope.

## Doublons de surcharges — développement après Gs++ 0.27.0-alpha.10 — 2026-10-02

### Français

- refus des signatures déclarées plusieurs fois, même inutilisées : nom source
  complet et paramètres canoniques, récepteur des méthodes compris, sans tenir
  compte du retour, des noms de paramètres ou de la visibilité ; diagnostic
  bilingue 118 à la seconde déclaration de la première paire identique ;
- fonctions libres, méthodes, constructeurs, destructeurs et opérateurs libres
  ou membres, alias de types et déclarations anticipées ; prototypes externes
  suivis d'une définition identique dans le même programme analysé refusés
  comme dans le bootstrap ;
- contrôle placé avant les remplacements virtuels et les corps, après les
  types, signatures et dispositions ; priorité des paires au sein d'un même
  groupe vérifiée sans généraliser celle de plusieurs groupes invalides ;
- 40 corpus valides français/anglais, 68 refus différentiels et dix refus
  d'émission sans écriture partielle ; total différentiel : 845 ; quatre
  corpus supplémentaires d'émission de données et relocalisations avec appels
  de surcharges distinctes, sans changer la limite des adresses surchargées ;
- CTest Windows 5/5, GNU/Linux 6/6, solution et validation MSBuild natives et
  conformité 20/20 sur les trois constructions ; frontend identique de
  376 031 octets, 75 exports et deux imports ; AST public, formats 1.0, ABI 1
  et diagnostics existants inchangés ;
- tranche locale non commitée et non publiée ; version et publication alpha.10
  inchangées. Collisions de symboles de liaison, indices et émission des tables
  virtuelles du futur backend auto-hébergé restent hors du périmètre validé.

### English

- reject repeated signatures, including unused declarations: canonical source
  names and parameter types, including method receivers, without using return
  types, parameter names or visibility to distinguish an overload; bilingual
  diagnostic 118 at the second declaration of the first identical pair;
- free functions, methods, constructors, destructors, free and member operators,
  type aliases and forward declarations; reject an external prototype followed
  by an identical definition in the same analyzed program, matching the bootstrap;
- validate after types, signatures and layouts, before virtual overrides and
  bodies; check pair-order priority within one group without generalizing
  ordering across several independent invalid groups;
- 40 valid French/English corpora, 68 differential rejections and ten emission
  rejections without partial writes; differential total 845; four additional
  data and relocation emission corpora calling distinct overloads, preserving
  the bootstrap restriction on addresses of overloaded functions;
- Windows CTest 5/5, GNU/Linux 6/6, native MSBuild solution and validation, and
  20/20 conformance on all three builds; identical 376,031-byte frontend images,
  75 exports and two imports; public AST, formats 1.0, ABI 1 and existing
  diagnostics unchanged;
- local, uncommitted and unpublished work; alpha.10 version and release unchanged.
  Link-symbol collisions, virtual-slot indices and table emission by the future
  self-hosted backend remain outside the validated scope.

## Remplacements virtuels — développement après Gs++ 0.27.0-alpha.10 — 2026-10-02

### Français

- validation des méthodes virtuelles héritées, même inutilisées : `remplacer`
  exige une clé compatible et une redéfinition compatible exige `remplacer` ;
  diagnostics bilingues 116–117 à la position de la méthode ;
- clés fondées sur le nom source, les paramètres explicites et le retour
  canoniques, sans récepteur implicite ; destructeurs et opérateurs inclus,
  hiérarchies contrôlées base puis dérivée, indépendamment de l'ordre lexical ;
- correction de la détection du polymorphisme pour les classes ne possédant
  qu'un destructeur ou opérateur virtuel ; offsets de tables et pas de tableaux
  d'objets comparés aux dispositions calculées par le bootstrap ;
- 46 corpus valides français/anglais, 64 refus différentiels et huit refus
  d'émission sans écriture partielle ; total différentiel : 767 ; quatre
  corpus supplémentaires d'émission de globales et de relocalisations ;
- comparaison des données utilisateur du bootstrap en isolant ses tables
  virtuelles de backend, qui ne sont pas émises par l'API de globales ;
- CTest Windows 5/5, GNU/Linux 6/6, validation MSBuild native et conformité
  20/20 sur les trois constructions ; images frontend identiques de 374 367
  octets, 75 exports et deux imports ; AST public, formats 1.0 et ABI 1 inchangés ;
- tranche locale non commitée et non publiée ; version et publication alpha.10
  inchangées. Collisions de surcharges, indices et émission des tables virtuelles
  du futur backend auto-hébergé restent hors du périmètre validé.

### English

- validate inherited virtual methods, including unused declarations: `override`
  requires a matching key, and a matching redefinition requires `override`;
  bilingual diagnostics 116–117 at the method position;
- match canonical source names, explicit parameters and return types without
  the implicit receiver; include destructors and operators, validating bases
  before derived classes regardless of lexical declaration order;
- detect polymorphism for classes with only a virtual destructor or operator;
  compare table offsets and object-array strides with bootstrap layouts;
- 46 valid French/English corpora, 64 differential rejections and eight emission
  rejections without partial writes; differential total 767; four additional
  global-data and relocation emission corpora;
- compare bootstrap user data separately from backend-generated virtual tables,
  which are not emitted by the frontend global-data API;
- Windows CTest 5/5, GNU/Linux 6/6, native MSBuild validation and 20/20
  conformance on all three builds; identical 374,367-byte frontend images,
  75 exports and two imports; public AST, formats 1.0 and ABI 1 unchanged;
- local, uncommitted and unpublished work; alpha.10 version and release unchanged.
  Overload collisions, slot indices and virtual-table emission by the future
  self-hosted backend remain outside the validated scope.

## Déclarations d'héritage — développement après Gs++ 0.27.0-alpha.10 — 2026-10-02

### Français

- validation des bases de classes, même inutilisées : héritage privé/protégé,
  bases absentes ou non-classes et auto-héritage direct ou via alias refusés
  avec les positions du bootstrap ; diagnostics bilingues 113–115, réemploi
  des codes 100 pour le type absent et 57 pour les cycles indirects ;
- résolution canonique des bases avec priorité au nom écrit puis au nom relatif
  à l'espace déclarant, y compris les noms qualifiés et chaînes d'alias ; cache
  privé partagé par les dispositions, conversions et recherches de membres ;
- conflits racines, énumérations et alias résolus avant l'héritage, puis types
  de champs et signatures, conformément aux priorités vérifiées du bootstrap ;
- 28 corpus valides français/anglais, 60 refus différentiels et 16 refus
  d'émission sans écriture partielle : total différentiel de 695 ; quatre
  nouveaux corpus d'émission valides comparés octet par octet ;
- CTest Windows 5/5, GNU/Linux 6/6, validation MSBuild native et conformité
  20/20 sur les trois constructions ; images frontend identiques de 368 879
  octets, 75 exports et deux imports ; AST public, formats 1.0 et ABI 1 inchangés ;
- développement local non publié, sans modification des paquets, du tag ni de
  la matrice alpha.10 ; `VERSION` reste à `0.27.0-alpha.10`. Les contraintes de
  remplacement virtuel et les autres familles sémantiques restent à compléter.

### English

- validate unused class bases as well: reject private/protected inheritance,
  missing or non-class bases, and direct or aliased self-inheritance at the
  bootstrap positions; bilingual diagnostics 113–115, reusing 100 for unknown
  types and 57 for indirect cycles;
- resolve canonical bases using the written name before its namespace-relative
  form, including qualified names and alias chains; share a private cache
  between layouts, conversions and member lookup;
- check root conflicts, enumerations and aliases before inheritance, then
  field types and signatures, following tested bootstrap diagnostic priorities;
- 28 valid French/English corpora, 60 differential rejections and 16 emission
  rejections without partial writes: differential total 695; four additional
  valid emission corpora compared byte for byte;
- Windows CTest 5/5, GNU/Linux 6/6, native MSBuild validation and 20/20
  conformance on all three builds; identical 368,879-byte frontend images,
  75 exports and two imports; public AST, formats 1.0 and ABI 1 unchanged;
- unpublished local development, leaving the alpha.10 packages, tag and release
  matrix unchanged; `VERSION` remains `0.27.0-alpha.10`. Virtual-override
  constraints and other semantic families remain to be completed.

## Gs++ 0.27.0-alpha.10 — 2026-10-02

### Français

- passage de la source centrale `VERSION` à `0.27.0-alpha.10`, propagée par
  CMake et MSBuild aux outils, métadonnées d'application GsE et noms de paquets ;
- consolidation des adaptations implicites composées, types et signatures de
  callbacks, alias de champs, alias racines et appels via alias de méthodes
  développés après alpha.9 ; suite différentielle de 619 corpus négatifs ;
- intégration de la solution native Visual Studio 2026 `.slnx` et de sa
  validation indépendante de CMake ;
- notes bilingues et matrice de validation propres à l'alpha.10 ; la preuve
  historique de publication alpha.9 reste intacte ;
- CTest Windows 5/5, GNU/Linux 6/6, validation MSBuild native et conformité
  20/20 sur les trois constructions ; benchmark smoke 4/4 Windows/GNU ; images
  frontend identiques de 366 719 octets, 75 exports et deux imports ;
- formats 1.0, ABI 1 et extension `.GsA` inchangés ; frontend encore partiel,
  migration `.Glib` / `.GdLib` toujours prévue pour 0.28.0 ;
- contrôles de distribution reproductibles : 11/11 sur chaque paquet extrait,
  dont exécution des alias de méthodes français/anglais et suite différentielle
  du frontend livré ; distribution en préversion depuis un commit et un tag signés.

### English

- update the central `VERSION` source to `0.27.0-alpha.10`, propagated by CMake
  and MSBuild to tools, GsE application metadata and package names;
- consolidate compound implicit adaptations, callback types and signatures,
  field aliases, root aliases and method-alias calls developed after alpha.9;
  619 negative differential corpora;
- include the native Visual Studio 2026 `.slnx` solution and validation without
  CMake;
- maintain bilingual alpha.10 notes and a dedicated validation matrix while
  preserving historical alpha.9 publication evidence;
- pass Windows CTest 5/5, GNU/Linux 6/6, native MSBuild validation and 20/20
  conformance on all three builds; Windows/GNU smoke benchmarks 4/4;
  identical 366,719-byte frontend images, 75 exports and two imports;
- retain formats 1.0, ABI 1 and `.GsA`; the frontend remains partial and the
  `.Glib` / `.GdLib` migration stays planned for 0.28.0;
- reproducible distribution checks: 11/11 on each extracted package, including
  French/English method-alias execution and the distributed frontend differential
  suite; distribute as a prerelease from a signed commit and tag.

## Alias de méthodes non liées — développement après Gs++ 0.27.0-alpha.9 — 2026-10-02

### Français

- validation des appels via alias racines de méthodes : le premier argument
  est une référence mutable vers la classe déclarante, suivie des paramètres
  explicites ; conversions dérivé/base et visibilité des appels directs
  alignées sur le bootstrap ;
- signatures de callbacks incluant ce récepteur, appels par adresse, conversions
  explicites, retours de structures et relocalisations globales vers la méthode
  canonique ; AST public, dispositions et ABI inchangés ;
- 48 corpus valides bilingues, 52 refus différentiels et six refus d'émission
  sans écriture partielle ; total différentiel contrôlé à l'exécution : 619 ;
  quatre corpus d'émission comparent octets, dispositions et relocalisations ;
- CTest Windows 5/5, GNU/Linux 6/6 et validation native Visual Studio 2026 sans
  CMake réussis, conformité 20/20 ; images frontend identiques, 366 718 octets,
  75 exports et deux imports ;
- prise d'adresse des méthodes privées/protégées conservée selon le comportement
  actuel du bootstrap, distinct du contrôle de visibilité d'un appel direct ;
- lot local non publié, `VERSION` inchangé ; les autres contraintes sémantiques
  et le backend auto-hébergé restent à compléter.

### English

- validate calls through root method aliases: a mutable reference to the
  declaring class comes first, followed by explicit parameters; match bootstrap
  derived-to-base conversions and direct-call visibility checks;
- include the receiver in callback signatures, address-based calls, explicit
  casts, aggregate returns and global relocations targeting the canonical
  method; preserve the public AST, layouts and ABI;
- add 48 bilingual valid corpora, 52 differential rejections and six emission
  rejections without partial writes; runtime-checked differential total: 619;
  compare bytes, layouts and relocations in four emission corpora;
- pass Windows CTest 5/5, GNU/Linux 6/6 and native Visual Studio 2026 validation
  without CMake, with 20/20 conformance cases; identical frontend images,
  366,718 bytes, 75 exports and two imports;
- preserve the current bootstrap behavior for taking private/protected method
  addresses, distinct from direct-call visibility checking;
- keep this local tranche unpublished and `VERSION` unchanged; other semantic
  constraints and the self-hosted backend remain unfinished.

## Alias racines — développement après Gs++ 0.27.0-alpha.9 — 2026-10-02

### Français

- résolution anticipée des alias de structures, unions, classes, fonctions et
  globales, y compris inutilisés, déclarations anticipées et chaînes ;
- cache itératif privé des cibles canoniques, noms qualifiés et priorité au nom
  complet écrit avant la recherche dans l'espace déclarant ; diagnostics
  bilingues 109–112 pour cycles, cibles absentes et ambiguïtés ;
- types canoniques dans les paramètres, retours, champs, tableaux, références,
  callbacks et conversions ; accès aux globales et appels libres vers la vraie
  déclaration, sans contourner la constance ni créer de stockage supplémentaire ;
- 64 corpus valides bilingues, dont une chaîne de 128 alias, 68 refus
  différentiels et six refus d'émission sans écriture partielle : 561 au total ;
  quatre corpus comparent octets, dispositions et relocalisations au bootstrap ;
- CTest Windows 5/5, GNU/Linux 6/6 et validation native Visual Studio 2026
  réussis, conformité 20/20 ; images frontend identiques, 75 exports et ABI
  publique inchangée ;
- développement local non publié, `VERSION` inchangé ; appels via alias de
  méthodes avec récepteur implicite et autres contraintes sémantiques encore
  à compléter, sans déclarer le frontend complet.

### English

- eagerly resolve structure, union, class, function and global aliases,
  including unused aliases, forward declarations and chains;
- private iterative canonical-target cache, qualified names and written-name
  priority before namespace-relative lookup; bilingual diagnostics 109–112
  for cycles, missing targets and ambiguities;
- canonical parameter, return, field, array, reference, callback and cast
  types; resolve global accesses and free calls to their actual declarations,
  preserving constness and avoiding duplicate storage;
- add 64 bilingual valid corpora, including a 128-alias chain, 68 differential
  rejections and six emission rejections without partial writes: 561 total;
  compare bytes, layouts and relocations in four bootstrap emission corpora;
- pass Windows CTest 5/5, GNU/Linux 6/6 and native Visual Studio 2026 validation,
  with 20/20 conformance cases, identical frontend images, 75 exports and
  unchanged public ABI;
- keep this local development unpublished and `VERSION` unchanged; method-alias
  calls with implicit receivers and other semantic constraints remain unfinished,
  without claiming a complete frontend.

## Retrait de l'ancien espace GSLSE — développement local — 2026-10-02

### Français

- trois worktrees historiques déplacés avec `git worktree move` dans
  `Construction/Worktrees/PackageSource` et `Construction/Worktrees/ReleaseSource` ;
- ancien dossier commun archivé hors des dépôts actifs dans
  `D:\『Projet』 Archives Transition\Retrait-GSLSE-2026-10-02\GSLSE`, sans
  suppression des archives, synthèses communes ni résultats historiques ;
- inventaire SHA-256 et contrôle des révisions, états et liens Git conservés
  avec l'archive ; liens documentaires locaux adaptés ;
- aucune évolution du langage, de `VERSION`, des formats ou de l'ABI dans
  cette opération ; les constructions actives restent propres à chaque projet.

### English

- move three historical worktrees through `git worktree move` into
  `Construction/Worktrees/PackageSource` and `Construction/Worktrees/ReleaseSource`;
- archive the former shared workspace outside the active repositories at
  `D:\『Projet』 Archives Transition\Retrait-GSLSE-2026-10-02\GSLSE`, preserving
  archives, shared documents and historical build/validation outputs;
- keep a SHA-256 inventory and Git revision, status and backlink checks beside
  the archive, and update local documentation paths;
- do not change language behavior, `VERSION`, binary formats or ABI; each
  active project retains its independent build tree.

## Alias de champs — développement après Gs++ 0.27.0-alpha.9 — 2026-10-02

### Français

- résolution anticipée de tous les alias de champs du frontend auto-hébergé,
  y compris inutilisés, avec chaînes et déclarations anticipées ;
- parcours itératif et cache privé des champs canoniques, sans modifier l'AST
  de l'appelant ni la disposition des structures ABI publiques ;
- diagnostics bilingues 107/108 pour les cycles et cibles introuvables ; les
  cibles restent des champs directs du type déclarant, comme dans le bootstrap ;
- normalisation commune aux accès, callbacks, tableaux et initialiseurs de
  constructeurs ; les alias ne contournent pas la visibilité du champ cible ;
- 30 corpus valides bilingues, dont une chaîne de 128 alias ; 40 refus
  différentiels et quatre refus d'émission supplémentaires, soit 487 au total,
  maintenant comptés à l'exécution ; deux corpus d'émission comparent le
  stockage canonique aux octets produits par le bootstrap ;
- CTest Windows 5/5, GNU/Linux 6/6 et validation native Visual Studio 2026
  réussis, conformité 20/20 ; frontend identique sur les trois constructions ;
- développement local non publié : `VERSION` reste à `0.27.0-alpha.9` et les
  alias de types, fonctions et globales restent à compléter dans l'auto-hébergement.

### English

- eagerly resolve all self-hosted field aliases, including unused aliases,
  forward declarations and chains;
- iterative traversal and private canonical-field cache, preserving the
  caller's AST and public ABI structure layouts;
- bilingual diagnostics 107/108 for cycles and unknown targets; targets remain
  direct fields of the declaring type, matching the bootstrap compiler;
- share canonical targets across member access, callbacks, arrays and constructor
  initializers; aliases cannot bypass the target field's access restrictions;
- add 30 bilingual valid corpora, including a 128-alias chain, 40 differential
  rejections and four emission rejections: 487 total, now counted at runtime;
  compare canonical storage against bootstrap bytes in two emission corpora;
- pass Windows CTest 5/5, GNU/Linux 6/6 and native Visual Studio 2026 validation,
  with 20/20 conformance cases and identical frontend images in all three builds;
- keep this local development unpublished, `VERSION` at `0.27.0-alpha.9`, and
  self-hosted type, function and global aliases explicitly unfinished.

## Construction indépendante et Visual Studio natif — développement local — 2026-09-21

### Français

- dépôt déplacé dans `D:\Langage-GsPlusPlus`, historique Git, worktrees et
  modifications locales conservés ; aucune nouvelle publication ;
- solution `GsPlusPlus.slnx` native Visual Studio 2026 / MSVC v145, sans CMake ;
- compilation des outils, bibliothèques, frontend et exécutables de tests par
  MSBuild ; cible explicite de validation utilisant aussi Python ;
- `VERSION` reste commun aux deux constructions ; sorties locales séparées dans
  `Construction/CMake` et `Construction/MSBuild`, chemins des benchmarks adaptés ;
- originaux graphiques externes, copie de diffusion du logo conservée dans `Assets`.

### English

- checkout relocated to `D:\Langage-GsPlusPlus`, preserving Git history, linked
  worktrees and local changes; no new release published;
- native Visual Studio 2026 / MSVC v145 `GsPlusPlus.slnx`, without CMake;
- MSBuild builds tools, libraries, frontend and test executables; an explicit
  validation target also uses Python;
- shared `VERSION`, separate local CMake/MSBuild output trees and updated benchmark paths;
- external original artwork, with the distribution logo retained in `Assets`.

## Contraintes des signatures — développement après Gs++ 0.27.0-alpha.9 — 2026-09-21

### Français

- validation récursive des signatures de callbacks : paramètres `vide` par
  valeur interdits, quatre paramètres au maximum, ou trois avec un retour
  d'agrégat par valeur ; priorité des erreurs imbriquées conservée ;
- validation des paramètres de fonctions, refus des références de tableaux et
  des définitions retournant une référence, limites d'arité comptant le
  récepteur implicite des méthodes et constructeurs ;
- diagnostics bilingues 101 à 106, sans changer les dispositions ABI publiques ;
- relecture du type de retour devant `opérateur` et sélection des opérateurs
  libres pour les structures et unions, en plus des classes ;
- 42 corpus valides français/anglais, 66 refus différentiels supplémentaires
  (443 au total), deux corpus d'émission et quatre refus d'émission contrôlant
  que les tampons restent intacts ;
- tranche locale non publiée ; `.Glib` et `.GdLib` restent une décision future
  pour 0.28.0, sans migration des artefacts 0.27.

### English

- recursively validate callback signatures: reject by-value `void` parameters,
  allow at most four parameters or three when returning an aggregate by value,
  and preserve nested-error precedence;
- validate function parameters, reject array references and reference-returning
  definitions, and include implicit receivers in method/constructor arity limits;
- add bilingual diagnostics 101–106 without changing public ABI layouts;
- reread return types preceding `operator` and resolve free operators on
  structures and unions as well as classes;
- add 42 bilingual valid corpora, 66 differential rejections (443 total), two
  emission corpora and four emission refusals checking untouched output buffers;
- keep this local development unpublished and the `.Glib` / `.GdLib` migration
  planned for 0.28.0, not applied to 0.27 artifacts.

## Types nommés — développement après Gs++ 0.27.0-alpha.9 — 2026-09-21

### Français

- résolution contextuelle des types nommés dans les signatures imbriquées,
  paramètres, retours, champs, variables et conversions relus depuis la source ;
- copie privée des nœuds typés : l'AST de l'appelant reste intact et les
  symboles/résolutions utilisent les empreintes sémantiques canoniques ;
- respect de la priorité du nom complet écrit puis du nom relatif à l'espace
  effectif du bootstrap, sans confondre les homonymes d'espaces distincts ;
- diagnostic bilingue 100 `TypeNommeIntrouvable` / `UnknownNamedType` ;
  maintien du diagnostic 99 pour une cible de conversion inconnue ;
- appel indirect d'un champ callback lorsqu'aucune méthode ne correspond,
  avec conservation des contrôles de visibilité, d'arité et de qualification ;
- 32 corpus valides français/anglais, 42 refus différentiels supplémentaires
  (377 au total), deux corpus d'émission et contrôles d'immuabilité de l'AST ;
- décision **prévue pour 0.28.0, non implémentée** : remplacer `.GsA` par `.Glib`
  pour les bibliothèques statiques, réserver `.GdLib` aux bibliothèques dynamiques
  si ce support est introduit, conserver `.GsE` pour les exécutables.

### English

- resolve named types contextually inside nested signatures and source-backed
  parameters, returns, fields, variables and casts;
- keep typed nodes in a private copy, preserving the caller's AST while using
  canonical semantic hashes for symbols and resolutions;
- match the bootstrap's written-full-name then contextual-relative-name lookup,
  keeping identically spelled types from different namespaces distinct;
- add bilingual diagnostic 100, `UnknownNamedType`, retaining diagnostic 99 for
  unknown cast targets;
- resolve callback field calls when no method matches, retaining visibility,
  arity and qualification checks;
- add 32 bilingual valid corpora, 42 differential rejections (377 total), two
  emission corpora and input-AST immutability checks;
- record a **planned, unimplemented 0.28.0 decision**: use `.Glib` instead of
  `.GsA` for static libraries, reserve `.GdLib` for dynamic libraries if supported,
  and retain `.GsE` for executables. This remains local development, not a release.

## Types composés — développement après Gs++ 0.27.0-alpha.9 — 2026-09-21

### Français

- index privé des signatures de fonctions et des indirections profondes,
  conservant les références et qualificatifs sans modifier l'AST ni l'ABI publics ;
- références de callbacks, adresses et déréférencements, signatures imbriquées,
  appels indirects et sélection de surcharges alignés sur les cas du bootstrap ;
- distinction entre un pointeur de fonction et un pointeur vers son emplacement
  lors des conversions ; maintien des refus de signatures incompatibles ;
- types d'éléments et indexations des tableaux de pointeurs sans limite de
  recherche fixée à deux ou quatre indirections ; tailles et alignements globaux
  comparés au bootstrap, y compris dans les structures ;
- 30 corpus valides français/anglais, 36 refus différentiels supplémentaires
  (335 au total) et deux corpus d'émission ; cette tranche reste locale et ne
  remplace pas la publication alpha.9 ni ne clôt le frontend 0.27.

### English

- privately index function signatures and deep pointer types while preserving
  references and qualifiers without changing the public AST or ABI;
- cover callback references, address/dereference operations, nested signatures,
  indirect calls and overload selection against the bootstrap;
- distinguish function pointers from pointers to their storage during casts,
  retaining incompatible-signature rejection;
- recover pointer-array element and indexing types without the previous fixed
  two/four-indirection search limits; compare global sizes and alignments with
  the bootstrap, including structure fields;
- add 30 bilingual valid corpora, 36 differential rejection cases (335 total)
  and two emission corpora; this local development does not replace the public
  alpha.9 release or mark the 0.27 frontend complete.

## Développement après Gs++ 0.27.0-alpha.9 — 2026-09-20

### Français

- unification des adaptations implicites de constantes entières : expressions
  composées, qualificatifs scalaires et contrôles de plage pour les surcharges,
  opérateurs intrinsèques, initialiseurs et appels indirects ;
- conservation des types adaptés dans l'arène privée, sans modifier l'AST ni
  l'ABI publique, et comparaison des octets globaux au bootstrap ;
- calcul des énumérateurs dans l'ordre avant leur utilisation par les appels ;
  conservation des erreurs de calcul pendant la sélection des surcharges ;
- correction des références locales de classe traitées à tort comme des objets
  à construire ; couverture des restrictions de qualification et d'héritage ;
- 42 refus différentiels français/anglais supplémentaires, soit 299 au total,
  36 corpus valides avec contrôles de sélection et deux corpus d'émission ;
- cette tranche de développement ne remplace pas la publication alpha.9 et ne
  déclare pas le frontend 0.27 complet.

### English

- unify implicit integer constant adaptation across compound expressions,
  scalar qualifiers, overload resolution, built-in operators and initializers;
- preserve adapted operand types in the private arena and compare emitted
  global bytes with the bootstrap without changing the public AST or ABI;
- evaluate enumerators in order before call resolution and preserve arithmetic
  diagnostics when considering overload candidates;
- stop treating local class references as objects requiring construction;
  cover qualification and inheritance restrictions;
- add 42 bilingual rejection cases (299 total), 36 valid semantic corpora and
  two global-emission corpora; this is development after alpha.9, not a new
  release or a claim of frontend completion.

## Gs++ 0.27.0-alpha.9 — 2026-09-20

### Français

- publication des avancées du frontend depuis alpha.8 : contraintes des
  expressions indirectes et des opérateurs, plans de durée de vie, références,
  affectations, retours, constantes, globales et émission de leurs données ;
- validation des conversions explicites `convertir` / `cast` : catégories
  scalaires, séparation pointeurs/entiers, identité des signatures de fonction,
  types nommés et plages des constantes, y compris conversions imbriquées,
  énumérations et branches à court-circuit ;
- conservation des qualificatifs de pointeurs et suppression du caractère
  référence sur le résultat d’une conversion ; les appels indirects à travers
  un pointeur de fonction converti retrouvent sa signature dans la source ;
- diagnostics bilingues 94 à 99 et 54 refus différentiels supplémentaires,
  portant le total à 257 corpus négatifs, sans modifier l’ABI publique existante ;
- version centralisée dans `VERSION`, métadonnées et paquets alpha.9 ; formats
  GsObj/GsA/GsE 1.0, ABI machine 1 et préfixe `GalacticShrine::GsPP::` conservés ;
- inclusion des quatre interfaces publiques du frontend dans les paquets,
  sous `share/GsPlusPlus/AutoHebergement`, à côté des bibliothèques livrées ;
- validation Windows 4/4, GNU/Linux 5/5, conformité 20/20, smoke 4/4 sur les
  deux chaînes, puis compilation et exécution depuis les paquets extraits ;
- le frontend reste partiel ; backend auto-hébergé, sorties natives multi-cibles
  et SDK de destination ne sont pas livrés par cette alpha.

### English

- release the frontend work completed since alpha.8: indirect expressions and
  operators, lifetime plans, references, assignments, returns, constants, global
  declarations, and in-memory global data/relocation emission;
- validate explicit casts, including scalar categories, pointer/integer
  separation, function signatures, named targets, constant ranges, nested
  conversions, enum values, and short-circuited branches;
- preserve pointer qualifiers, make cast results values rather than references,
  and recover cast function-pointer signatures for subsequent indirect calls;
- add bilingual diagnostics 94–99 and 54 differential rejection cases, for
  257 negative corpora; retain existing public ABI layouts, formats 1.0 and ABI 1;
- ship all four frontend interfaces under `share/GsPlusPlus/AutoHebergement`;
- pass Windows 4/4 and GNU/Linux 5/5 tests, 20/20 conformance and 4/4 smoke
  scenarios on each toolchain, plus extracted-package compilation/execution;
- keep the frontend marked partial: a self-hosted backend, native multi-target
  outputs, and destination SDKs remain future work.

## Développement après Gs++ 0.27.0-alpha.8 — 2026-09-20

- ajout de l’API `EmettreGlobales` / `EmitGlobals` dans `Frontend.GsE` :
  disposition indépendante des zones données/zéro, octets little-endian des
  constantes et agrégats, remplissage nul des éléments omis et des alignements,
  relocalisations absolues 64 bits vers des fonctions définies ou importées ;
- contrat public dédié avec tampons fournis par l’appelant, interrogation des
  besoins et diagnostics 91 à 93, sans écriture partielle en cas de capacité
  insuffisante ou d’erreur sémantique ; l’ABI des requêtes existantes reste
  inchangée, ainsi que les formats 1.0 et l’ABI machine 1 ;
- correction de la résolution des énumérateurs relatifs à l’espace courant,
  de la comparaison des types nommés simples/qualifiés et de la validation des
  initialiseurs de tableaux de pointeurs de fonction ;
- comparaison différentielle des données, dispositions et relocalisations sur
  cinq corpus, dont une paire complète français/anglais ; huit nouveaux refus positionnés
  portent la matrice sémantique à 203 corpus, avec contrôles supplémentaires
  des capacités, sentinelles, erreurs, déterminisme et débordements de section ;
- conservation des ajouts locaux de documentation, de licence et de mise en
  forme ; ajustement du contrôle `@Paramètre(type: nom)` pour accepter les
  types qualifiés tels que `GalacticShrine::GsPP::Hebergee::VueTexte` ;
- validation locale 4/4 sous Visual Studio 2026 et 5/5 sous GNU/Linux ; les
  deux chaînes produisent le même `Frontend.GsE` de 328 270 octets, 75 exports,
  accepté par `gseverifier`, SHA-256
  `1bf0c652b7cd6d51c8434cbc7d8fc2a21da00385dbf26fd3965426a3a5a70d5d` ;
- cette tranche n’écrit pas encore de fichier objet auto-hébergé et ne livre
  pas les cibles natives ni SDK prévus par la décision multi-cible.

## Décision produit — compilation multi-cible — 2026-09-19

- ajout au plan produit d'une cible native par défaut, d'une sélection
  explicite en ligne de commande ou dans le projet XML, et de la compilation
  croisée lorsque les composants de la destination sont disponibles ;
- premières cibles prévues : Windows PE, GNU/Linux ELF et Sanctuaire SE /
  ShrineOS GsE sur x86-64, avec ABI, SDK et validation propres à chaque cible ;
- intégration de ces exigences aux jalons 0.28 et 0.29 et aux critères de
  sortie 1.0 ; alignement des README français/anglais et des contrats de
  projets et d'ABI sur la distinction entre hôte et cible ;
- décision documentaire uniquement : les nouvelles options, attributs XML,
  sorties natives et SDK restent à implémenter.

## Développement après Gs++ 0.27.0-alpha.8 — 2026-09-15

- ajout, dans la passe sémantique auto-hébergée, de l’évaluation numérique des
  littéraux entiers et booléens, constantes d’énumération, opérateurs unaires
  et binaires, décalages, comparaisons, courts-circuits et conversions ;
- calcul privé des valeurs d’énumération explicites et implicites dans l’ordre
  source, avec contrôle de la plage `entier32`, du débordement de la valeur
  suivante et refus des initialiseurs non entiers ou non constants ;
- contrôle de la plage signée ou non signée des constantes affectées aux
  initialiseurs globaux scalaires ou imbriqués dans un agrégat, et diagnostic
  dédié de la division ou du modulo constant par zéro ;
- ajout des diagnostics sémantiques 85 à 90 et de vingt refus différentiels
  bilingues, portant le total à cent quatre-vingt-quinze corpus dont le code,
  la ligne et la colonne correspondent au bootstrap ;
- maintien sans modification de l’ABI publique de l’AST et de la passe
  sémantique : les valeurs calculées restent conservées dans l’arène privée ;
- validation locale 4/4 sous Visual Studio 2026 et 5/5 sous GNU/Linux ; les
  deux chaînes reconstruisent le même `Frontend.GsE` GsE 1.0 de 317 022 octets
  et 73 exports, accepté par `gseverifier`, dont le SHA-256 est
  `9446947bb60908d8a7df57b53e8397de3b387f15b1cff7c881bd7a7d0f22b281`.

## Développement après Gs++ 0.27.0-alpha.8 — 2026-08-30

- validation récursive de la forme des initialiseurs globaux : liste exigée
  pour les structures et unions, cible de fonction directe pour les pointeurs
  de fonction, refus des pointeurs de données initialisés et constance
  structurelle des feuilles scalaires, conversions et agrégats imbriqués ;
- résolution des constantes d’énumération qualifiées dans le frontend
  auto-hébergé et conservation de leur type d’énumération ;
- ajout des diagnostics sémantiques 81 à 84 et de dix refus différentiels
  bilingues, portant le total à cent soixante-quinze, avec un corpus positif
  pour l’arithmétique constante, les conversions, agrégats, énumérateurs et
  pointeurs de fonction globaux autorisés ;
- alignement des contraintes structurelles des variables globales sur le
  bootstrap : refus des objets de classe, références, types `vide`, imports à
  la fois publics et externes et constantes non initialisées ;
- ajout des diagnostics sémantiques 76 à 80 et de dix refus différentiels
  bilingues portant le total à cent soixante-cinq, avec un corpus positif pour
  les pointeurs vers classes, constantes initialisées, imports constants et
  pointeurs de fonction autorisés ;
- validation locale 4/4 sous Visual Studio 2026 et 5/5 sous GNU/Linux ; les
  deux chaînes reconstruisent le même `Frontend.GsE` GsE 1.0 de 295 646 octets
  et 73 exports, dont le SHA-256 est
  `03422775cda6395fa57d00eeaf0b75ed96394b0ceb971731cf51d936bd2b3a9c` ;
- centralisation de la version technique du produit dans le fichier racine
  `VERSION`, désormais lu par CMake et propagé aux bannières, métadonnées GsE,
  tests, benchmarks et paquets sans duplication dans le code ;
- remplacement des anciennes métadonnées `shrine-x86_64-v2` par la cible
  autonome `gspp-x86_64` et l’identifiant ABI canonique
  `GsAbi:x64-ms-v1`, sans modifier les formats 1.0 ni l’ABI binaire 1 ;
- alignement des liaisons de références locales, directes et indirectes sur le
  bootstrap : valeur gauche obligatoire, compatibilité exacte ou par héritage
  et interdiction de retirer `constante` / `const` ;
- validation auto-hébergée des initialiseurs scalaires, des cibles
  d’affectation, des valeurs constantes, des copies de tableaux, des types
  affectés et des valeurs de retour ;
- extension de l’adaptation des arguments littéraux aux formes unaires signées
  et ajout des diagnostics sémantiques 69 à 75 sans modifier l’ABI ni les codes
  antérieurs ;
- ajout de dix-huit refus différentiels bilingues portant le total à cent
  cinquante-cinq, avec validation locale 4/4 sous Visual Studio 2026 et 5/5
  sous GNU/Linux de l’image `Frontend.GsE` identique de 285 537 octets et 73
  exports ; son SHA-256 est
  `675c0579adc26431fc25ce846a390c2d68351fadc4c218d0148906333a8a5d59` ;
- nettoyage de la documentation publiée : conservation des contrats Markdown
  courants, de la dernière matrice de publication et d’une note de publication
  canonique, les anciens rapports restant disponibles dans l’historique Git ;
- validation auto-hébergée des six opérateurs unaires et des dix-huit
  opérateurs binaires intrinsèques, après résolution prioritaire des surcharges
  membres ou libres ;
- alignement du type de résultat non qualifié et de l’adaptation des littéraux
  entiers placés à gauche ou à droite, y compris la borne signée minimale de
  `entier64` ;
- prise en compte des entiers, booléens, pointeurs ordinaires, pointeurs de
  fonction et tableaux dans la matrice scalaire, arithmétique et de
  comparaison ;
- ajout des diagnostics sémantiques 60 à 68 sans modifier les codes précédents
  ni l’ABI, puis de dix-huit corpus négatifs bilingues portant le total à cent
  trente-sept ;
- validation locale 4/4 sous Visual Studio 2026 et 5/5 sous GNU/Linux 11.4,
  avec une image `Frontend.GsE` GsE 1.0 de 281 169 octets et 73 exports,
  identique bit à bit entre les deux chaînes, dont le SHA-256 est
  `fcc65ad812685ba84c468faf47515427efb2a49b13580a7388d8ee2218027dcf` ;

## Développement après Gs++ 0.27.0-alpha.8 — 2026-08-29

- validation auto-hébergée des contraintes de valeur gauche de `&`, avec
  conservation du cas particulier de l’adresse d’une fonction et refus des
  tableaux complets comme dans le bootstrap ;
- refus positionné du déréférencement d’une valeur non pointeur, des cibles non
  indexables, des indices non entiers, de l’indexation de `vide*` et de
  l’indexation d’un pointeur de fonction pur ;
- reconstruction privée de l’arité et des paramètres d’un pointeur de fonction
  afin de contrôler les appels indirects, y compris les paramètres par
  référence et les callbacks imbriqués ou obtenus par déréférencement ;
- prise en charge de l’appel explicite `(&Fonction)(...)` sans ajouter de champ
  aux nœuds, symboles, résolutions, résultats ou requêtes de l’ABI publique ;
- ajout des diagnostics sémantiques 47 à 55, sans renuméroter les diagnostics
  existants ni modifier les tailles ABI ;
- extension du corpus positif bilingue aux références, adresses explicites de
  fonctions et pointeurs vers callbacks, puis ajout de vingt-quatre corpus
  négatifs différentiels portant le total à cent huit ;
- représentation compacte des opérateurs libres dans l’AST auto-hébergé et
  alignement du bootstrap afin d’identifier aussi ces déclarations comme des
  opérateurs véritables ;
- sélection typée des opérateurs libres binaires et unaires lorsque aucun
  opérateur membre prioritaire n’existe, avec classement des littéraux,
  héritages et références `constante` / `const` ;
- ajout du diagnostic sémantique 59 pour les arités invalides des opérateurs
  membres ou libres, puis de huit corpus négatifs différentiels portant le
  total à cent dix-neuf ;
- validation locale 4/4 sous Visual Studio 2026 et 5/5 sous GNU/Linux, avec une
  image `Frontend.GsE` GsE 1.0 de 275 345 octets, identique bit à bit entre les
  deux chaînes, dont le SHA-256 est
  `f3762720abf876c24d0a4e2da0a81f75026425b6f2048c84bfe1449a0b7b1963`.

## Compilateur Gs++ 0.27.0-alpha.8 — 2026-08-29

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
- ajout des genres AST compacts `InitialiseurConstructeurDelegue`,
  `InitialiseurConstructeurBase` et `InitialiseurChampConstructeur`, avec les
  arguments rattachés à leur initialiseur plutôt qu’aplatis sous la fonction ;
- résolution typée des délégations `soi(...)` / `this(...)` et des
  constructeurs de base explicites `parent(...)` / `super(...)`, avec contrôle
  d’accès et détection des délégations directes ou cycliques ;
- résolution des initialiseurs vers les champs directs canoniques, y compris
  au travers d’un alias, avec refus des champs inconnus, doublons et ordres de
  déclaration inversés ;
- ajout des diagnostics sémantiques 30 à 35 et de huit corpus négatifs
  différentiels, portant leur total à quarante et un ;
- ajout des drapeaux de résolution `DelegationConstructeur`,
  `InitialisationBase` et `InitialisationChamp`, sans agrandir les nœuds,
  symboles, résolutions, résultats ou requêtes ABI ;
- regroupement des étapes auto-hébergées dans l’unique exécutable
  `Frontend.GsE`, qui exporte le classificateur, le lexeur, l’analyseur
  syntaxique et l’analyseur sémantique ;
- conservation des quatre `.GsObj` de construction comme modules internes et
  retrait des quatre images GsE spécialisées de la construction, de
  l’installation et des paquets ;
- validation de l’arité et du type connu des initialiseurs de champs
  scalaires, avec adaptation des littéraux entiers représentables et retrait
  des qualificatifs de valeur comme dans le bootstrap ;
- sélection surchargée et contrôle d’accès du constructeur des champs objets
  de classe directs, avec une résolution distincte du champ et de son
  constructeur ;
- résolution du constructeur sans argument de la base directe lorsque le
  constructeur dérivé n’utilise ni `parent(...)` ni une délégation `soi(...)` ;
- ajout des diagnostics `AriteInitialiseurChampInvalide` et
  `TypeInitialiseurChampIncompatible`, ainsi que de huit corpus négatifs
  différentiels, portant leur total à quarante-neuf ;
- reconstruction sémantique des dimensions de tableaux depuis la source sans
  agrandir l’AST compact, afin de retrouver le type final des éléments natifs
  ou nommés ;
- sélection du constructeur uniforme des tableaux locaux et des tableaux de
  champs objets, ainsi que construction implicite des champs objets omis de la
  liste du constructeur ;
- application des valeurs de champs par défaut lorsqu’aucun initialiseur
  explicite ne les remplace, validation de leurs types scalaires, de la forme
  agrégée des tableaux et de l’initialisation obligatoire des champs constants ;
- ajout des diagnostics de valeurs par défaut d’objets classes, de classe sans
  constructeur, de champ constant non initialisé et de tableau non agrégé,
  avec quatorze corpus négatifs supplémentaires portant le total à
  soixante-trois ;
- validation récursive des agrégats selon chaque dimension de tableau, avec
  contrôle de capacité à tous les niveaux et vérification du type connu de
  chaque feuille ;
- application du même contrat aux tableaux globaux et locaux, aux structures,
  unions, agrégats scalaires, valeurs de champs par défaut et initialiseurs de
  champs explicites, sans modifier l’ABI publique ;
- ajout des diagnostics 42 à 46 pour les dépassements de tableaux, structures
  ou scalaires, les éléments incompatibles et les tableaux non agrégés, avec
  quatorze corpus négatifs supplémentaires portant le total à
  soixante-dix-sept ;
- ajout d’un cache interne des cibles de résolution, alloué dans l’arène de la
  passe sans modifier l’AST ni les structures ABI publiques ;
- remplacement des trois parcours séparés des références, membres et
  opérateurs par un parcours descendant unique de l’AST compact, afin de
  résoudre les enfants avant leurs expressions parentes ;
- propagation du type de retour des fonctions, méthodes et opérateurs membres
  sélectionnés, ainsi que du type booléen des comparaisons et opérateurs
  logiques, jusque dans le classement des surcharges et les feuilles
  d’agrégats ;
- ajout d’un corpus positif bilingue combinant appels de méthodes, surcharges
  libres et opérateurs membres imbriqués, puis de trois corpus négatifs
  différentiels portant le total à quatre-vingts ;
- extension du hachage compact des types aux pointeurs de fonction, y compris
  les signatures imbriquées fermées par `>>`, sans modifier le nœud AST ni les
  structures sémantiques publiques ;
- propagation récursive du type d’une indexation de tableau ou de pointeur, de
  l’adresse `&`, du déréférencement `*` et du retour des appels indirects ;
- reconstruction privée des signatures de callbacks depuis les jetons de la
  source afin de typer aussi un tableau de callbacks et un callback retournant
  un autre callback ;
- ajout d’un corpus positif bilingue couvrant cinq compositions de ces
  expressions et de quatre refus différentiels supplémentaires, portant le
  total à quatre-vingt-quatre ;
- correction des chemins automatiques du banc de mesure vers les constructions
  et sessions permanentes situées dans `../Construction` ;
- correction du graphe CMake afin que toute modification des objets syntaxique
  ou sémantique force réellement la nouvelle liaison de `Frontend.GsE` ;
- validation complète 4/4 avec Visual Studio 2026 et 5/5 sous GNU/Linux,
  conformité 20/20 et quatre scénarios de benchmark smoke réussis sur chaque
  chaîne, avec `Frontend.GsE` identique bit à bit, valide au format GsE 1.0,
  de 231 809 octets et de SHA-256
  `e798a3fae8903a1788d66c0c2ef4187a64bd136d9a99131d170044a8a65c301a`.

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
