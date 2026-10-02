# Notes de publication Gs++ / Gs++ release notes

## 0.27.0-alpha.10 — 2026-10-02

### Français

**Sources, constructions et paquets extraits validés.**
Cette préversion consolide les avancées depuis alpha.9,
dans l'unique image `Frontend.GsE` :

- adaptation implicite des constantes composées, types effectifs,
  qualifications et sélection des surcharges ;
- références et signatures imbriquées de callbacks, tableaux de pointeurs à
  indirections profondes et disposition des données globales ;
- résolution contextuelle des types nommés dans les signatures, homonymes,
  appels de callbacks stockés dans les champs et AST d'entrée préservé ;
- contraintes récursives de signatures, limites d'arité avec récepteur
  implicite et opérateurs libres sur structures/unions ;
- chaînes d'alias de champs, alias racines de types, fonctions et globales,
  validation des alias inutilisés et diagnostics bilingues 100–112 ;
- appels via alias de méthodes non liées, récepteur mutable `Classe&`
  explicite, visibilité des appels directs et signatures de callbacks ;
- 619 corpus négatifs comparés au bootstrap (257 dans alpha.9), avec code,
  ligne, colonne, capacités bornées et absence d'écriture partielle contrôlés ;
- construction native Visual Studio 2026 via `GsPlusPlus.slnx`, sans CMake,
  en complément de CMake ; version commune lue depuis `VERSION`.

Les formats `.GsObj`, `.GsA` et `.GsE` restent en **1.0**, avec ABI **1**,
signatures `GSOBJ:0`, `GSA:0`, `GSE:0`, et préfixe
`GalacticShrine::GsPP::`. Les structures publiques existantes restent compatibles.

Validation : CTest Visual Studio 2026 **5/5**, GNU/Linux **6/6**, validation
MSBuild native réussie, conformité **20/20** sur les trois constructions et
benchmark smoke **4/4** sous Windows/CMake et GNU/Linux. `Frontend.GsE` est
identique bit à bit entre les trois constructions : **366 719 octets**,
**75 exports**, **deux imports**.
Les deux paquets extraits réussissent **11/11 contrôles** : versions, SDK,
interfaces publiques, exemples, exécution d'alias de méthodes français/anglais,
bibliothèques système/hébergée et suite différentielle du frontend distribué.
Le détail est dans la
[matrice alpha.10](Documentation/Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.10.md).

**Limites :** frontend encore partiel, qualifications et contraintes restantes
d'héritage et des autres familles sémantiques à compléter. Le bootstrap C++
produit toujours le code machine et les fichiers finaux ; les sorties natives
Windows PE/Linux ELF et leurs SDK restent prévues, non implémentées.
La prise d'adresse d'une méthode privée/protégée conserve le comportement
actuel du bootstrap, distinct des contrôles de visibilité des appels directs.
`.GsA` reste l'extension des bibliothèques statiques ; `.Glib` et `.GdLib`
restent prévus pour 0.28.0.

### English

**Sources, local builds and extracted packages validated.**
This prerelease consolidates development since alpha.9 in the single
`Frontend.GsE` image:

- implicit compound-constant adaptations, effective types, qualifiers and
  overload selection;
- callback references and nested signatures, deeply indirect pointer arrays
  and global-data layouts;
- context-aware named types in signatures, same-name types, callback-field
  calls and preservation of the caller's input AST;
- recursive signature constraints, parameter limits including the implicit
  receiver and free operators on structures/unions;
- field-alias chains, root type/function/global aliases, validation of unused
  aliases and bilingual diagnostics 100–112;
- unbound method-alias calls with an explicit mutable `Class&` receiver,
  direct-call visibility checks and callback signatures;
- 619 negative bootstrap differential corpora (257 in alpha.9), checking
  code, line, column, bounded capacities and absence of partial writes;
- native Visual Studio 2026 builds through `GsPlusPlus.slnx`, without CMake,
  alongside CMake; both read the same product version from `VERSION`.

File formats remain **1.0**, ABI **1**, with unchanged signatures and existing
public ABI layouts. Local CTest passes **5/5** on Visual Studio 2026 and
**6/6** on GNU/Linux; native MSBuild validation passes, with **20/20**
conformance on all three builds and **4/4** smoke benchmark scenarios on
Windows/CMake and GNU/Linux. All three produce the same **366,719-byte**,
**75-export**, **two-import** frontend.
Both extracted packages pass **11/11 checks**, covering versions, SDK and
public interfaces, examples, French/English method-alias execution, system and
hosted libraries, and the distributed frontend differential suite.

**Limitations:** this is not a complete self-hosted compiler. Remaining semantic
families, qualifiers and inheritance constraints still need work; the C++ bootstrap
still generates machine code and final files. Native Windows PE/Linux ELF
outputs and destination SDKs remain planned, not delivered.
Taking private/protected method addresses preserves the current bootstrap
behavior, distinct from direct-call visibility checking. `.GsA` remains the
static-library extension; `.Glib` and `.GdLib` stay planned for 0.28.0.

## Archives / Assets

- `GsPlusPlus-0.27.0-alpha.10-Windows-x86_64.zip`
- `GsPlusPlus-0.27.0-alpha.10-Linux-x86_64.tar.gz`
- `SHA256SUMS.txt`

Les archives de diffusion sont produites par CPack depuis un export du commit
signé de publication, puis contrôlées après extraction dans des répertoires
neufs. Le manifeste SHA-256 externe est calculé après le dernier paquetage.
Distribution archives are produced by CPack from an export of the signed
release commit, then checked after extraction into fresh directories.
The external SHA-256 manifest is computed after the final packaging step.

Les anciennes notes et matrices restent dans l’historique Git et les releases
GitHub. / Previous notes and validation records remain in Git history and
GitHub releases.
