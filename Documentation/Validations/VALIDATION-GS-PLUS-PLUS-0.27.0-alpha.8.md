# Validation Gs++ 0.27.0-alpha.8

**VALIDÉ — Visual Studio 2026 et GNU/Linux — 29 août 2026.**

Cette preuve porte sur la tranche `0.27.0-alpha.8` du frontend auto-hébergé :
sélection typée des surcharges, membres et visibilité, constructeurs et
initialiseurs de sous-objets, tableaux et agrégats imbriqués, propagation des
types à travers les appels, opérateurs, indexations, adresses,
déréférencements et appels indirects. Elle ne déclare ni le frontend 0.27
complet ni Gs++ 1.0 stables.

## Périmètre vérifié

- compilation native du compilateur, du chargeur et du vérificateur ;
- construction des bibliothèques `GsSysteme.GsA` et `GsHebergee.GsA` ;
- construction et exécution réelle de l’image unifiée `Frontend.GsE` ;
- comparaison différentielle du lexeur, de l’AST et de l’analyse sémantique
  avec le bootstrap C++ ;
- quatre-vingt-quatre familles d’erreurs sémantiques positionnées ;
- formats GsObj/GsA/GsE 1.0, ABI 1 et signatures canoniques ;
- matrice de conformité portable 20/20 ;
- intégration GNU/Bash ;
- benchmark smoke à quatre scénarios ;
- reproductibilité de `Frontend.GsE` entre les deux chaînes ;
- création, extraction et utilisation des paquets Windows et Linux.

## Environnements

| Chaîne | Environnement | Résultat CTest |
| --- | --- | ---: |
| MSVC | Visual Studio 2026, MSBuild 18.9.1, SDK Windows 10.0.26100.0, CMake 4.4.0, x64 Release | 4/4 |
| GNU | Ubuntu WSL, GCC 11.4.0, Ninja 1.10.1, CMake 3.22.1, x86-64 Release | 5/5 |

La cinquième épreuve GNU est l’intégration Bash, volontairement absente de la
matrice Windows.

## Frontend sémantique alpha.8

La passe sémantique consomme l’AST compact sans modifier son contrat public.
Elle résout les types déjà déterminables des références, membres, surcharges,
constructeurs, initialiseurs et agrégats. Un cache privé des cibles permet de
propager les types des enfants vers les expressions parentes.

La description compacte des types couvre aussi les pointeurs de fonction. Le
frontend peut ainsi typer une indexation de tableau ou de pointeur, `&`, `*`,
un appel indirect, un tableau de callbacks et un callback retournant un autre
callback. Les signatures sont reconstruites dans l’arène privée de la passe et
n’ajoutent aucun champ à l’ABI publique.

Les corpus français et anglais sont comparés au bootstrap. Pour chaque refus,
le code, la ligne et la colonne sont identiques. Les requêtes de capacité, les
sorties partielles, les requêtes invalides et l’équilibre de l’arène sont aussi
contrôlés.

| Contrat ABI | Taille |
| --- | ---: |
| `NoeudDeclaration` | 64 octets |
| `SymboleSemantique` | 48 octets |
| `ResolutionSemantique` | 32 octets |
| `ResultatAnalyseSemantique` | 56 octets |
| `RequeteAnalyseSemantique` | 120 octets |

## Image unifiée et reproductibilité

Les quatre objets de construction suivants restent internes :

- `ClassificateurMotsCles.GsObj` ;
- `Lexeur.GsObj` ;
- `AnalyseurDeclarations.GsObj` ;
- `AnalyseurSemantique.GsObj`.

Ils sont liés dans l’unique image installée `Frontend.GsE`. Les fichiers MSVC
et GNU/Linux sont identiques bit à bit :

| Image | Taille | SHA-256 |
| --- | ---: | --- |
| `Frontend.GsE` | 231 809 | `2f5df5a749361ff723a5738e7dcfeec45bfee7e73436c0ea9d03fa8a4d0a749f` |

`gseverifier` décrit l’image comme GsE 1.0 avec trois segments, huit sections,
deux imports et soixante-treize exports. Les imports sont uniquement
`GalacticShrine::GsPP::Hote::AllouerMemoire` et
`GalacticShrine::GsPP::Hote::LibererMemoire`.

## Conformité et benchmarks

| Contrôle | Windows | GNU/Linux |
| --- | ---: | ---: |
| CTest | 4/4 | 5/5 |
| Conformité portable | 20/20 | 20/20 |
| Benchmark smoke | 4/4 | 4/4 |
| État | réussi | réussi |

Les rapports JSON annoncent `Gs++ 0.27.0-alpha.8`, zéro échec et les formats
`.GsObj`, `.GsA` et `.GsE` en version 1.0 avec ABI 1.

Les sessions de prépublication sont :

- Windows : `20260828T221504.529891Z-fcec2b05` ;
- GNU/Linux : `20260828T221614.809141Z-3a2d068a`.

## Paquets validés

Les archives finales sont :

- `GsPlusPlus-0.27.0-alpha.8-Windows-x86_64.zip` ;
- `GsPlusPlus-0.27.0-alpha.8-Linux-x86_64.tar.gz`.

Chaque archive est produite depuis le commit de publication dans un arbre
propre, puis extraite dans un dossier neuf. Depuis chaque paquet :

- `gsppc --version` annonce `Gs++ Compiler 0.27.0-alpha.8` ;
- `gsechargeur --version` annonce `Chargeur GsE 0.27.0-alpha.8` ;
- `gseverifier` accepte le `Frontend.GsE` livré et retrouve ses 73 exports ;
- `gsppc` compile l’exemple `Bonjour.Gs++` en GsObj ;
- la documentation et les interfaces correspondent au commit publié.

Les empreintes finales ne sont volontairement pas incorporées dans le présent
document, lui-même inclus dans les archives. Le fichier externe
`SHA256SUMS.txt`, généré après le dernier paquetage, constitue la source de
vérité. Les archives et ce manifeste sont consolidés sous
`Construction/GsPlusPlus-0.27.0-alpha.8/Packages/`.

## Limites conservées

- frontend auto-hébergé encore partiel ;
- contraintes complètes des valeurs adressables et indices encore à valider ;
- arité et paramètres de tous les appels indirects encore à couvrir ;
- opérateurs libres et combinaisons intrinsèques exhaustives à migrer ;
- conversions, qualifications et liaisons de références à compléter ;
- plans complets de construction, destruction et durée de vie à migrer ;
- cible de génération actuelle limitée à x86-64 et à la signature
  `GsAbi:x64-ms-v1`.
