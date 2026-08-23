# Bibliothèque hébergée Gs++ 0.26

**VALIDÉ — Gs++ 0.26.0 — 23 août 2026.**

## Objet du jalon

Gs++ 0.26.0 transforme `GsHebergee.GsA` en socle de propriété mémoire
suffisant pour commencer la migration du frontend du compilateur en Gs++ au
jalon 0.27. La bibliothèque 0.17 savait manipuler des vues, flux, vecteurs et
tables dont le stockage était fourni par l’appelant. La 0.26 ajoute les briques
propriétaires qui manquaient : chaînes UTF-8, tampons dynamiques, arène stable,
table de symboles dynamique, chemins et fichiers alloués.

Le compilateur complet reste écrit majoritairement en C++ dans ce jalon. La
0.26 livre la bibliothèque nécessaire à sa migration ; elle ne revendique pas
encore l’auto-hébergement du frontend, du backend ou de l’éditeur de liens.

## Frontière entre profils

`GsSysteme.GsA` reste entièrement freestanding : aucune allocation, aucun
fichier, aucun diagnostic d’hôte et aucun import `Gs::Hote` ne peut y entrer.

`GsHebergee.GsA` dépend uniquement des cinq imports explicitement déclarés :

```text
Gs::Hote::AllouerMemoire(naturel64)
Gs::Hote::LibererMemoire(octet*)
Gs::Hote::LireFichier(RequeteFichier*)
Gs::Hote::EcrireFichier(RequeteFichier*)
Gs::Hote::EmettreDiagnostic(Diagnostic)
```

Une allocation n’est jamais déclenchée par une construction implicite. Elle a
lieu seulement lors d’un appel explicite à une fonction de réserve,
d’affectation, d’ajout, d’arène, de table dynamique ou de chargement de fichier.

## Modèle d’erreur explicite

Les opérations qui peuvent échouer retournent `CodeErreurHebergee` :

| Code | Signification |
| ---: | --- |
| `Reussite` / `Success` | opération terminée |
| `ArgumentInvalide` / `InvalidArgument` | pointeur, taille ou alignement invalide |
| `AllocationEchouee` / `AllocationFailed` | l’hôte n’a pas fourni la mémoire |
| `CapaciteDepassee` / `CapacityExceeded` | débordement ou capacité impossible |
| `LectureEchouee` / `ReadFailed` | lecture de fichier refusée ou incohérente |
| `EcritureEchouee` / `WriteFailed` | écriture de fichier refusée |
| `CheminInvalide` / `InvalidPath` | chemin vide, contenant zéro ou invalide |
| `Utf8Invalide` / `InvalidUtf8` | séquence UTF-8 non canonique |

Aucune exception n’est générée. En cas d’échec d’allocation, les chaînes et
conteneurs conservent leur ancien contenu. Les fonctions de destruction sont
idempotentes et remettent les structures à zéro.

## Chaînes UTF-8 propriétaires

`ChaineUtf8` contient un pointeur, une taille en octets et une capacité utile.
Le contenu est toujours terminé par zéro sans inclure ce zéro dans `Taille`.

Les opérations prévues sont :

- initialisation et destruction explicites ;
- validation UTF-8 stricte, avec refus des surlongueurs, substituts UTF-16 et
  points de code supérieurs à `U+10FFFF` ;
- réserve transactionnelle ;
- affectation depuis `VueTexte` ;
- ajout d’une vue ou d’un octet ASCII ;
- production d’une vue non propriétaire.

Les vues restent valides jusqu’à la prochaine opération susceptible de
réallouer la chaîne ou jusqu’à sa destruction.

## Conteneurs dynamiques

La bibliothèque fournit :

- `VecteurOctetsDynamique` pour les sources, objets et sections binaires ;
- `VecteurNaturelsDynamique` pour les indices, positions et identifiants ;
- `TableSymbolesDynamique` pour les associations texte vers valeur entière ;
- `AreneMemoire` pour les jetons, nœuds d’AST et structures dont l’adresse doit
  rester stable.

Les vecteurs doublent leur capacité lorsque cela est possible et refusent tout
débordement de taille. La table conserve le sondage linéaire déterministe et
réalloue vers une capacité doublée avant une saturation excessive. Les clés de
la table restent des vues non propriétaires : leur stockage doit survivre à la
table, typiquement dans une chaîne, un fichier alloué ou une arène.

L’arène utilise des blocs chaînés qui ne sont jamais déplacés. Les pointeurs
déjà retournés restent donc stables jusqu’à `DetruireArene`. Les alignements
acceptés sont les puissances de deux comprises entre 1 et 8, correspondant aux
types du contrat ABI actuel.

## Chemins et fichiers

`JoindreCheminUtf8` assemble deux vues avec un séparateur fourni par l’appelant
et évite de dupliquer ce séparateur. Les fonctions de vue extraient le nom de
fichier et l’extension sans allocation. Les chemins doivent être du UTF-8
valide, non vides et sans octet nul intégré.

`InitialiserFichierAlloue` établit d’abord un propriétaire vide.
`ChargerFichierAlloue` utilise ensuite un protocole en deux appels : le premier demande
la taille à l’hôte, le second remplit le tampon alloué. Le résultat appartient à
l’appelant et doit être libéré par `LibererFichierAlloue`. Un fichier vide est
un succès avec une taille nulle. `SauverFichierResultat` fournit la variante à
code d’erreur de l’ancienne fonction booléenne. `ExtensionVue` suit la
convention de `std::filesystem::path::extension` pour les noms ordinaires et
inclut le point initial, par exemple `.GsPP`.

## Contrat de l’hôte

L’hôte doit respecter les règles suivantes :

- `AllouerMemoire(0)` peut retourner zéro ; la bibliothèque ne lui demande pas
  d’allocation nulle ;
- `LibererMemoire(0)` n’est jamais nécessaire mais doit rester sans danger ;
- la mémoire retournée est alignée au minimum sur 8 octets ;
- une requête de lecture avec `Donnees == 0` et `Capacite == 0` demande seulement
  la taille du fichier ;
- une lecture réussie ne peut annoncer une taille supérieure à la capacité ;
- tous les imports utilisent l’ABI Microsoft x64 canonique de Gs++.

## Formats et ABI

Le jalon ne modifie aucun conteneur natif :

| Contrat | Valeur conservée |
| --- | --- |
| Signature GsObj | `GSOBJ:0` suivie d’un octet nul |
| Signature GsA | `GSA:0` suivie de trois octets nuls |
| Signature GsE | `GSE:0` suivie de trois octets nuls |
| Formats GsObj, GsA et GsE | 1.0 |
| Champs ABI | 1 |
| Signature de liaison | `GsAbi:x64-ms-v1` |

Les nouvelles structures et fonctions possèdent leurs propres signatures ABI
inter-unités. Les productions locales antérieures doivent être reconstruites,
mais aucune migration de format n’est nécessaire.

## Couverture attendue avant validation

- chaînes ASCII et UTF-8 accentuées, terminaison nulle et ajout transactionnel ;
- refus d’une séquence UTF-8 surlongue ;
- croissance et lecture de vecteurs d’octets et de naturels ;
- croissance, mise à jour et recherche dans une table dynamique ;
- stabilité de pointeurs issus de plusieurs blocs d’arène ;
- jointure, nom et extension de chemin ;
- lecture allouée, écriture et libération sans fuite dans l’hôte de test ;
- échec d’allocation conservant l’ancien contenu ;
- présence exacte des cinq imports hébergés nécessaires ;
- absence de tout import `Gs::Hote` dans `GsSysteme.GsA` ;
- reconstruction déterministe de `GsHebergee.GsA` ;
- validation MSVC, GNU, benchmark et QEMU/OVMF.

Cette couverture réussit sous MSVC et GNU/WSL. Les benchmarks smoke et la
preuve QEMU/OVMF réussissent également ; les résultats et empreintes sont
consignés dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.26.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.26.0.md).
