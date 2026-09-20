# Notes de publication Gs++ / Gs++ release notes

## 0.27.0-alpha.9 — 2026-09-20

### Français

Cette préversion consolide les avancées du frontend auto-hébergé depuis
alpha.8, dans l’unique image `Frontend.GsE` :

- validation des expressions indirectes, opérateurs, références, affectations
  et retours ; plans ordonnés de construction, destruction et tables virtuelles ;
- contraintes des globales, évaluation des constantes et énumérations,
  émission en mémoire des données globales et relocalisations de fonctions ;
- validation des conversions explicites, catégories scalaires, signatures de
  fonctions, qualificatifs et plages constantes, y compris formes imbriquées ;
- 257 corpus négatifs comparés au bootstrap, dont 54 nouveaux dans cette tranche ;
- version centralisée dans `VERSION` et quatre interfaces publiques du frontend
  désormais incluses sous `share/GsPlusPlus/AutoHebergement`.

Les formats `.GsObj`, `.GsA` et `.GsE` restent en **1.0**, avec ABI **1**,
signatures `GSOBJ:0`, `GSA:0`, `GSE:0`, et préfixe
`GalacticShrine::GsPP::`. Les structures publiques existantes restent compatibles.

Validation : Visual Studio 2026 **4/4**, GNU/Linux **5/5**, conformité **20/20**
et benchmark smoke **4/4** sur chaque chaîne. `Frontend.GsE` est identique bit à
bit entre les deux constructions : **334 318 octets**, **75 exports**.
Le détail est dans la
[matrice alpha.9](Documentation/Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.9.md).

**Limites :** frontend encore partiel, conversions implicites composées et
autres familles sémantiques à compléter. Le bootstrap C++ produit toujours le
code machine et les fichiers finaux. Cette publication ne fournit pas encore
les sorties natives Windows PE/Linux ELF ni leurs SDK prévus dans la feuille
de route.

### English

This prerelease consolidates self-hosted frontend development since alpha.8
in the single `Frontend.GsE` image:

- indirect expressions, operators, references, assignments and returns;
  ordered construction, destruction and virtual-table plans;
- global declaration checks, numeric constants and enum evaluation,
  in-memory global data and function relocation emission;
- explicit cast validation, scalar categories, function signatures,
  qualifiers and constant ranges, including nested casts;
- 257 negative differential corpora, including 54 new cases in this tranche;
- centralized product version and all four public frontend interfaces
  shipped under `share/GsPlusPlus/AutoHebergement`.

File formats remain **1.0**, ABI **1**, with unchanged signatures and existing
public ABI layouts. Local validation passes **4/4** on Visual Studio 2026,
**5/5** on GNU/Linux, **20/20** conformance and **4/4** smoke benchmark scenarios
on each toolchain. Both produce the same **334,318-byte**, **75-export** frontend.

**Limitations:** this is not a complete self-hosted compiler. Remaining semantic
families and compound implicit conversions still need work; the C++ bootstrap
still generates machine code and final files. Native Windows PE/Linux ELF
outputs and destination SDKs remain planned, not delivered.

## Archives / Assets

- `GsPlusPlus-0.27.0-alpha.9-Windows-x86_64.zip`
- `GsPlusPlus-0.27.0-alpha.9-Linux-x86_64.tar.gz`
- `SHA256SUMS.txt`

Les anciennes notes et matrices restent dans l’historique Git et les releases
GitHub. / Previous notes and validation records remain in Git history and
GitHub releases.
