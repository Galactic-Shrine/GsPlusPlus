# Compilateur Gs++ 0.21.0 — référence historique

> La référence courante est
> [`REFERENCE-GS-PLUS-PLUS-0.22.md`](REFERENCE-GS-PLUS-PLUS-0.22.md). Ce
> document conserve le périmètre validé du jalon 0.21.

## Portée

Gs++ 0.21.0 est le compilateur système natif de Galactic-Shrine et de
Sanctuaire SE. Son bootstrap reste principalement écrit en C++, mais le
langage, ses formats natifs et son backend AMD64 sont indépendants du langage
C++ source.

Cette version conserve l’héritage simple, `parent/super` et les prologues de
construction de 0.20, puis étend la liste aux champs directs. Le Markdown est
la documentation principale ; les `.docx` éventuels ne sont que des copies de
consultation.

## Contrats binaires courants

| Contrat | Valeur canonique |
|---|---|
| Signature GsObj | sept octets `GSOBJ:0`, puis un octet réservé nul |
| GsObj | format 1.0, AMD64, ABI 1 |
| GsA | format 1.0 |
| GsE | format 1.0, AMD64, ABI d’import 1 |
| ABI textuelle | `GsAbi:x64-ms-v1:` |
| Convention d’appel | Microsoft x64 |
| Compatibilité ancienne | aucune ; reconstruction obligatoire |

`GSO:0`, `GSOBJ\0`, GsE 2.0 et les champs ABI 2 restent refusés. Le passage à
0.21 ne renumérote aucun format et aucun champ ABI.

## État fonctionnel

| Domaine | État en 0.21.0 |
|---|---|
| Lexeur/parser bilingue FR/EN | validé |
| Types système, tableaux, unions, énumérations | validé |
| Pointeurs et pointeurs de fonction | validé |
| Valeurs structurées et agrégats | validé |
| Compilation séparée GsObj/GsA/GsE | validé |
| Classes, visibilité, surcharge et RAII local | validé |
| Virtuels et héritage simple public | validé |
| `remplacer/override` et dispatch par base | validé |
| `parent/super` pour la base directe | validé |
| Initialisateurs de champs directs | validé en 0.21.0 |
| Champs constants, pointeurs, structures et tableaux | validé en 0.21.0 |
| Champs objets classes récursifs | prévu |
| Héritage multiple/virtuel et RTTI | prévu |
| Auto-hébergement complet | partiel, non revendiqué |

## Exemple 0.21

```gspp
structure Point
{
    entier32 X;
    entier32 Y;
};

classe Derivee : publique Base
{
    privée:
        constante entier32 Bonus;
        Point Position;

    publique:
        constructeur(entier32 valeur, entier32 bonus)
            : parent(valeur), Bonus(bonus), Position({2, 3})
        {
        }

        remplacer entier32 Lire()
        {
            retourner parent.Lire() + soi.Bonus
                + soi.Position.X + soi.Position.Y;
        }
};
```

La base est construite avant `Bonus` et `Position`. Les champs sont initialisés
avant le corps, et `parent.Lire()` reste un appel direct vers l’implémentation
héritée.

## Construction, disposition et ABI objet

Le sous-objet de base reste au décalage zéro. Les champs propres conservent la
disposition déjà encodée dans la signature ABI. La liste ne change pas cette
disposition : elle fournit seulement les valeurs écrites par le constructeur.

`parent/super` doit être la première entrée, puis les champs directs uniques
suivent l’ordre de leur déclaration. Les champs non listés conservent leur mise
à zéro. Les champs objets classes sont encore refusés, car leur destruction
récursive n’est pas définie dans ce jalon.

Le contrat complet est décrit dans
[`INITIALISEURS_CHAMPS_GS_PLUS_PLUS_0.21.md`](INITIALISEURS_CHAMPS_GS_PLUS_PLUS_0.21.md).
Le contrat de la base directe reste dans
[`INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md`](INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md).

## Outils

- `gsppc` compile sources, interfaces, objets, bibliothèques, projets et
  solutions ;
- `gseverifier` vérifie la structure et W^X d’un GsE ;
- `gsechargeur` et `gseload` chargent et peuvent exécuter un GsE ;
- `GsSysteme.GsA` fournit les primitives freestanding ;
- `GsHebergee.GsA` fournit le socle hébergé à stockage explicite ;
- `ClassificateurMotsCles.GsE` constitue la preuve actuelle
  d’auto-hébergement partiel.

`gsppc --version` et `gsechargeur --version` annoncent tous deux 0.21.0.

## Construction et validation

Windows/MSVC :

```bat
cmake -S . -B Construction/Validation-GSPP-0210-MSVC ^
  -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build Construction/Validation-GSPP-0210-MSVC ^
  --config Release --target espace_travail --parallel
ctest --test-dir Construction/Validation-GSPP-0210-MSVC ^
  -C Release --output-on-failure
```

GNU/WSL :

```bash
cmake -S . -B Construction/Validation-GSPP-0210-GNU \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build Construction/Validation-GSPP-0210-GNU \
  --target espace_travail -j2
ctest --test-dir Construction/Validation-GSPP-0210-GNU \
  --output-on-failure
```

La validation du 16 août 2026 obtient 4/4 sous MSVC, 5/5 sous GNU/WSL et un
démarrage QEMU/OVMF réel réussi. Les détails se trouvent dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.21.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.21.0.md).

## Documentation principale

- cette référence donne l’état courant ;
- `INITIALISEURS_CHAMPS_GS_PLUS_PLUS_0.21.md` fixe le nouveau contrat ;
- `INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md` décrit la base du prologue ;
- `HERITAGE_GS_PLUS_PLUS_0.19.md` décrit le socle d’héritage ;
- `COMPILATION_SEPAREE_GS_PLUS_PLUS_0.13.md` et `FORMAT_GSE_1.0.md` décrivent
  les conteneurs natifs ;
- `FEUILLE_DE_ROUTE_GS_PLUS_PLUS.md` sépare les jalons terminés et prévus.

## Limites courantes

La 0.21 ne fournit pas la durée de vie récursive des champs objets classes, les
valeurs par défaut au point de déclaration, la délégation entre constructeurs,
l’héritage multiple/virtuel, les virtuels purs, la RTTI, les conversions
descendantes, les références de champ/globale/tableau/retour, les constructeurs
globaux ni les exceptions. Elle ne revendique pas non plus l’auto-hébergement
complet du compilateur.
