# Compilateur Gs++ 0.25.0 — référence historique

> La référence courante est
> [`REFERENCE-GS-PLUS-PLUS-0.26.md`](REFERENCE-GS-PLUS-PLUS-0.26.md).

## Statut

Gs++ 0.25.0 était le compilateur système natif courant de Galactic-Shrine et de
Sanctuaire SE. Il conserve le contrat candidat Gs++ 1.0 établi en 0.24 et
finalise l’initialisation déterministe des champs et des tableaux d’objets sans
introduire de runtime caché.

La documentation normative est maintenue d’abord en Markdown. Les éventuels
documents bureautiques sont des copies secondaires de consultation.

## Contrats binaires canoniques

| Élément | Valeur courante |
| --- | ---: |
| Signature GsObj | `GSOBJ:0` suivie d’un octet nul |
| Signature GsA | `GSA:0` suivie de trois octets nuls |
| Signature GsE | `GSE:0` suivie de trois octets nuls |
| Version GsObj | 1.0 |
| Version GsA | 1.0 |
| Version GsE | 1.0 |
| ABI d’objet, d’archive et de GsE | 1 |
| Signature de liaison | `GsAbi:x64-ms-v1` |

La version `0.25.0` est celle du compilateur, pas celle des conteneurs. Les
formats restent en `1.0` et aucune compatibilité avec les anciennes signatures
locales `GSO:0`, `GSOBJ\0`, `GSA\0` ou `GSE\0` n’est ajoutée.

## Extensions courantes

| Rôle | Extensions |
| --- | --- |
| Sources Gs++ | `.Gs++`, `.GsPP`, `.GsPlusPlus` |
| Interfaces Gs++ | `.HGs++`, `.HGsPP`, `.HeaderGsPlusPlus` |
| Sources Gs# réservées | `.Gs#`, `.GsS`, `.GsSharp` |
| Projets et solutions | `.GsPj`, `.GsProject`, `.GsPs` |
| Production native | `.GsObj`, `.GsA`, `.GsE` |

`.GsPH`, `.GsO`, `.GsPPH` et `.GsPlusPlusHeader` sont obsolètes et refusées.
Gs# suit le modèle C# et ne possède aucun fichier d’en-tête.

## Périmètre validé

| Domaine | État |
| --- | --- |
| Alias, types système, structures et tableaux fixes | validé |
| Compilation séparée, projets, GsA et édition de liens | validé |
| Pointeurs de fonction et ABI structurée | validé |
| Bibliothèques système/hébergée et auto-hébergement partiel | validé |
| Classes, références, visibilité, surcharges et RAII | validé |
| Héritage simple public et `remplacer/override` | validé |
| `parent/super` et initialisateurs de champs | validé |
| Champs objets classes directs et récursifs | validé en 0.22.0 |
| Extensions et signatures définitives | validé en 0.22.1 |
| Tableaux de champs objets classes multidimensionnels | validé en 0.23.0 |
| Tableaux locaux d’objets classes | validé en 0.23.0 |
| Construction croissante et destruction inverse par élément | validé en 0.23.0 |
| Valeurs par défaut des champs de classes | validé en 0.25.0 |
| Délégation `soi(arguments)`/`this(arguments)` | validé en 0.25.0 |
| Arguments uniformes des tableaux d’objets classes | validé en 0.25.0 |
| Objets de classe globaux | exclus normativement en 0.25.0 |

## Exemple courant

```gspp
publique entier32 Compteur;

publique entier32 ProchaineValeur()
{
    Compteur = Compteur + 1;
    retourner Compteur;
}

classe Element
{
    publique:
        entier32 Valeur;
        constructeur(entier32 valeur) { soi.Valeur = valeur; }
        destructeur() {}
};

classe Systeme
{
    publique:
        entier32 Capacite = 7;
        Element Capteurs[3];

        constructeur() : Capteurs(ProchaineValeur()) {}
        constructeur(entier32 capacite) : soi()
        {
            soi.Capacite = capacite;
        }
};
```

L’argument de `Capteurs` est réévalué pour chaque élément : les trois valeurs
sont donc `1`, `2`, puis `3`. `Systeme(9)` délègue d’abord à `Systeme()`, qui
initialise la valeur par défaut et le tableau, puis remplace `Capacite` dans le
corps délégant. Les éléments sont détruits dans l’ordre inverse.

Le contrat consolidé est
[`SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md`](SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md)
et sa preuve portable est définie dans
[`CONFORMITE_GS_PLUS_PLUS_1.0.md`](CONFORMITE_GS_PLUS_PLUS_1.0.md).

## Outils

- `gsppc --version` annonce `Gs++ Compiler 0.25.0` ;
- `gsechargeur --version` annonce `Chargeur GsE 0.25.0` ;
- le vérificateur et le chargeur continuent d’annoncer `GsE 1.0` pour les
  images produites.

## Construction et validation

### Windows/MSVC

```powershell
cmake --preset release
cmake --build --preset release --target espace_travail --parallel
ctest --preset release --output-on-failure
```

La construction permanente est conservée dans
`Construction/VisualStudio/Release`.

### GNU/WSL

```bash
cmake --preset release-ninja
cmake --build --preset release-ninja --target espace_travail --parallel
ctest --preset release-ninja --output-on-failure
```

La construction permanente est conservée dans `Construction/Ninja/Release`.

La preuve datée, les résultats d’intégration et le démarrage QEMU/OVMF sont
dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.25.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.25.0.md).

## Documents normatifs associés

- [`COMPILATION_SEPAREE_GS_PLUS_PLUS_0.13.md`](COMPILATION_SEPAREE_GS_PLUS_PLUS_0.13.md) — objets et archives ;
- [`FORMAT_GSA_1.0.md`](FORMAT_GSA_1.0.md) — bibliothèque native ;
- [`FORMAT_GSE_1.0.md`](FORMAT_GSE_1.0.md) — exécutable natif ;
- [`MODELE_OBJET_GS_PLUS_PLUS_0.18.md`](MODELE_OBJET_GS_PLUS_PLUS_0.18.md) — modèle objet ;
- [`HERITAGE_GS_PLUS_PLUS_0.19.md`](HERITAGE_GS_PLUS_PLUS_0.19.md) — héritage simple ;
- [`INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md`](INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md) — initialisation de la base ;
- [`INITIALISEURS_CHAMPS_GS_PLUS_PLUS_0.21.md`](INITIALISEURS_CHAMPS_GS_PLUS_PLUS_0.21.md) — champs non classes ;
- [`CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md`](CHAMPS_OBJETS_CLASSES_GS_PLUS_PLUS_0.22.md) — champs objets directs ;
- [`TABLEAUX_OBJETS_CLASSES_GS_PLUS_PLUS_0.23.md`](TABLEAUX_OBJETS_CLASSES_GS_PLUS_PLUS_0.23.md) — tableaux d’objets classes ;
- [`INITIALISATION_DUREE_VIE_GS_PLUS_PLUS_0.25.md`](INITIALISATION_DUREE_VIE_GS_PLUS_PLUS_0.25.md) — valeurs par défaut, délégation, tableaux et globales ;
- [`FORMAT_GSOBJ_1.0.md`](FORMAT_GSOBJ_1.0.md) — format objet natif ;
- [`ABI_GS_PLUS_PLUS_X64_MS_V1.md`](ABI_GS_PLUS_PLUS_X64_MS_V1.md) — ABI native ;
- [`PROFILS_GS_PLUS_PLUS_1.0.md`](PROFILS_GS_PLUS_PLUS_1.0.md) — profils
  freestanding et hébergé.

## Limites courantes

Gs++ 0.25.0 ne fournit pas les arguments ou agrégats distincts par élément
d’un tableau de classes, la copie implicite de tableaux d’objets, l’héritage
multiple/virtuel, la RTTI, les virtuels purs ou les exceptions. Les objets de
classe globaux sont exclus du contrat 1.0 courant afin de conserver un profil
freestanding sans initialisation ni destruction cachées.
