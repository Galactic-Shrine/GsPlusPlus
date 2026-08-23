# Compilateur Gs++ 0.22.1 — référence historique

## Statut

Gs++ 0.22.1 est la référence historique qui a figé l’inventaire et les formats
avant le jalon 0.23. Elle conserve la durée de vie récursive des champs objets classes
ajoutée en 0.22.0 et applique l’inventaire consolidé des extensions ainsi que
les signatures natives GsA/GsE définitives avant publication.

La référence courante est
[`REFERENCE-GS-PLUS-PLUS-0.26.md`](REFERENCE-GS-PLUS-PLUS-0.26.md).

La documentation normative est maintenue en Markdown. Les éventuels documents
bureautiques sont des copies secondaires et ne remplacent pas les fichiers
`.md`.

## Contrats binaires canoniques

| Élément | Valeur courante |
|---|---:|
| Signature GsObj | `GSOBJ:0` suivie d’un octet nul |
| Signature GsA | `GSA:0` suivie de trois octets nuls |
| Signature GsE | `GSE:0` suivie de trois octets nuls |
| Version GsObj | 1.0 |
| Version GsA | 1.0 |
| Version GsE | 1.0 |
| ABI d’objet | 1 |
| ABI d’archive | 1 |
| ABI d’en-tête et d’import GsE | 1 |
| Signature de liaison | `GsAbi:x64-ms-v1` |

Gs++ 0.22.1 ne renumérote aucun format : GsObj, GsA et GsE restent en 1.0,
avec ABI 1. Les anciennes signatures locales `GSO:0`, `GSOBJ\0`, `GSA\0` et
`GSE\0` ne sont pas acceptées. Les projets étant locaux, leurs objets,
archives et exécutables sont reconstruits sans couche de compatibilité.

## Extensions courantes

| Rôle | Extensions |
| --- | --- |
| Sources Gs++ | `.Gs++`, `.GsPP`, `.GsPlusPlus` |
| Interfaces Gs++ | `.HGs++`, `.HGsPP`, `.HeaderGsPlusPlus` |
| Projets et solutions | `.GsPj`, `.GsProject`, `.GsPs` |
| Production native | `.GsObj`, `.GsA`, `.GsE` |

`.GsPH`, `.GsO`, `.GsPPH` et `.GsPlusPlusHeader` sont refusées. Les extensions
source Gs# sont réservées au compilateur Gs# et ne sont jamais analysées
silencieusement par `gsppc`. Gs# ne possède aucun fichier d’en-tête.

## Périmètre validé

| Domaine | État en 0.22.1 |
|---|---|
| Alias applicatifs et noms qualifiés | validé |
| Types système, tableaux, énumérations et unions | validé |
| Compilation séparée, projets, solutions et GsA | validé |
| Pointeurs de fonction et ABI structurée | validé |
| Bibliothèques système et hébergée | validé |
| Auto-hébergement partiel du classificateur | validé |
| Classes, visibilité, références et surcharges | validé |
| RAII local et méthodes virtuelles | validé |
| Héritage simple public et `remplacer/override` | validé |
| `parent/super` dans les constructeurs et appels directs | validé |
| Initialisateurs de champs non classes | validé |
| Champs objets classes explicites et implicites | validé en 0.22.0 |
| Construction récursive à travers une classe sans constructeur | validé en 0.22.0 |
| Destruction récursive sur sorties normales et anticipées | validé en 0.22.0 |
| Table virtuelle d’un champ imbriqué sans constructeur | validé en 0.22.0 |
| Extensions Gs++ nouvelles et rejet des obsolètes | validé en 0.22.1 |
| Signatures `GSA:0` et `GSE:0`, formats 1.0, ABI 1 | validé en 0.22.1 |

## Exemple courant

```gspp
classe Capteur
{
    publique:
        entier32 Valeur;
        constructeur(entier32 valeur) { soi.Valeur = valeur; }
        destructeur() {}
};

classe Journal
{
    publique:
        constructeur() {}
        destructeur() {}
};

classe Systeme
{
    Capteur Principal;
    Journal Trace;

    publique:
        constructeur(entier32 valeur)
            : Principal(valeur)
        {}
};
```

`Principal` est construit explicitement. `Trace`, omis de la liste, sélectionne
automatiquement son constructeur sans argument. À la fin de vie de `Systeme`,
`Trace` est détruit avant `Principal`.

Le contrat détaillé se trouve dans
[`CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md`](CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md).

## Ordres normatifs

Construction d’un objet :

1. base de la racine vers la dérivée ;
2. table virtuelle de la classe courante ;
3. champs directs dans l’ordre de déclaration ;
4. corps du constructeur.

Destruction d’un objet :

1. corps du destructeur le plus dérivé ;
2. champs directs en ordre inverse ;
3. base directe, puis remontée jusqu’à la racine.

Chaque champ classe applique récursivement les mêmes règles. Les plans portent
des décalages relatifs ; les appels reçoivent donc l’adresse exacte du
sous-objet et non l’adresse du conteneur.

## Outils

`gsppc --version` annonce `Gs++ Compiler 0.22.1`.

`gsechargeur --version` annonce `Chargeur GsE 0.22.1`.

Les sorties GsE continuent d’annoncer le format `GsE 1.0`. La version du
compilateur et la version du conteneur ne doivent pas être confondues.

## Construction et validation

### Windows/MSVC

```powershell
cmake -S . -B Construction/Validation-GSPP-0221-MSVC `
  -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build Construction/Validation-GSPP-0221-MSVC `
  --config Release --target preparer_tests --parallel
ctest --test-dir Construction/Validation-GSPP-0221-MSVC `
  -C Release --output-on-failure
```

### GNU/WSL

```bash
cmake -S . -B Construction/Validation-GSPP-0221-GNU \
  -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build Construction/Validation-GSPP-0221-GNU --parallel
cmake --build Construction/Validation-GSPP-0221-GNU \
  --target preparer_tests --parallel
ctest --test-dir Construction/Validation-GSPP-0221-GNU --output-on-failure
```

La preuve datée et les résultats QEMU sont dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.22.1.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.22.1.md).

## Documents normatifs associés

- [`COMPILATION_SEPAREE_GS_PLUS_PLUS_0.13.md`](COMPILATION_SEPAREE_GS_PLUS_PLUS_0.13.md) — objets GsObj et archives GsA ;
- [`FORMAT_GSA_1.0.md`](FORMAT_GSA_1.0.md) — bibliothèque GsA 1.0 ;
- [`FORMAT_GSE_1.0.md`](FORMAT_GSE_1.0.md) — exécutable GsE 1.0 ;
- [`MODELE_OBJET_GS_PLUS_PLUS_0.18.md`](MODELE_OBJET_GS_PLUS_PLUS_0.18.md) — modèle objet initial ;
- [`HERITAGE_GS_PLUS_PLUS_0.19.md`](HERITAGE_GS_PLUS_PLUS_0.19.md) — héritage simple ;
- [`INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md`](INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md) — base et appels parent ;
- [`INITIALISEURS_CHAMPS_GS_PLUS_PLUS_0.21.md`](INITIALISEURS_CHAMPS_GS_PLUS_PLUS_0.21.md) — champs non classes ;
- [`CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md`](CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md) — durée de vie récursive.

## Limites courantes

Gs++ 0.22.1 ne fournit pas encore les tableaux de classes, les constructeurs
délégués, les valeurs par défaut au point de déclaration, les constructeurs
globaux, l’héritage multiple/virtuel, la RTTI, les virtuels purs ou les
exceptions.
