# Compilateur Gs++ 0.19.0 — référence historique

> La référence courante est
> [`REFERENCE-GS-PLUS-PLUS-0.22.md`](REFERENCE-GS-PLUS-PLUS-0.22.md). Ce
> document conserve le contrat exact du jalon 0.19 avant l’ajout de
> `parent/super`.

## Portée

Gs++ 0.19.0 est le compilateur système natif de Galactic-Shrine et de
Sanctuaire SE. Son bootstrap reste principalement écrit en C++, mais le
langage, ses formats natifs et son backend AMD64 sont indépendants du langage
C++ source.

Cette version conserve tout le modèle objet 0.18 et ajoute l’héritage simple
public, le remplacement explicite des virtuels et les chaînes de durée de vie
des bases. Le Markdown est la documentation principale ; les `.docx`
éventuels ne sont que des copies de consultation.

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

`GSO:0`, `GSOBJ\0`, GsE 2.0 et les champs ABI 2 sont d’anciennes bases locales
refusées. Le passage à 0.19 ne renumérote aucun format et aucun champ ABI.

## État fonctionnel

| Domaine | État en 0.19.0 |
|---|---|
| Lexeur/parser bilingue FR/EN | validé |
| Types système, tableaux, unions, énumérations | validé |
| Pointeurs et pointeurs de fonction | validé |
| Valeurs structurées et agrégats | validé |
| Compilation séparée GsObj/GsA/GsE | validé |
| Classes, visibilité, constructeurs/destructeurs | validé |
| Références locales et de paramètres | validé |
| Surcharge et RAII local | validé |
| Virtuels locaux | validé |
| Héritage simple public | validé en 0.19.0 |
| Accès protégé hérité | validé en 0.19.0 |
| `remplacer/override` et dispatch par base | validé en 0.19.0 |
| Chaînes automatiques base/dérivée | validé en 0.19.0 |
| Héritage multiple/virtuel et RTTI | prévu |
| Auto-hébergement complet | partiel, non revendiqué |

## Exemple 0.19

```gspp
classe Base
{
    protégée: entier32 Valeur;
    publique:
        constructeur() { soi.Valeur = 40; }
        virtuel entier32 Lire() { retourner soi.Valeur; }
        virtuel destructeur() {}
};

classe Derivee : publique Base
{
    privée: entier32 Bonus;
    publique:
        constructeur() { soi.Bonus = 2; }
        remplacer entier32 Lire()
        { retourner soi.Valeur + soi.Bonus; }
        remplacer destructeur() {}
};

publique entier32 Principal()
{
    Derivee valeur();
    Base& base = valeur;
    retourner base.Lire();
}
```

L’appel retourne `42`. La référence conserve l’adresse du sous-objet de base au
décalage zéro et l’appel virtuel emploie l’implémentation de `Derivee`.

## Disposition et ABI objet

Une classe dérivée commence par sa base complète. Si la base est polymorphe,
son pointeur de table est réutilisé ; les remplacements gardent leurs indices
et les nouveaux virtuels sont ajoutés à la fin. Si une dérivée d’une base non
polymorphe introduit le premier virtuel, le nouveau pointeur de table est placé
après la base afin de conserver la conversion d’adresse au décalage zéro.

La signature ABI récursive encode la base, les tailles/alignements, le décalage
de table et le fournisseur de chaque emplacement. Un désaccord entre une
interface et une implémentation est refusé lors de l’édition de liens.

Le contrat complet est décrit dans
[`HERITAGE_GS_PLUS_PLUS_0.19.md`](HERITAGE_GS_PLUS_PLUS_0.19.md). Le contrat
0.18 antérieur reste documenté dans
[`MODELE_OBJET_GS_PLUS_PLUS_0.18.md`](MODELE_OBJET_GS_PLUS_PLUS_0.18.md).

## Outils

- `gsppc` compile les sources, interfaces, objets, bibliothèques, projets et
  solutions ;
- `gseverifier` vérifie la structure et les contrats W^X d’un GsE ;
- `gsechargeur` et `gseload` chargent un GsE, résolvent ses imports et peuvent
  exécuter son point d’entrée ;
- `GsSysteme.GsA` fournit les primitives freestanding ;
- `GsHebergee.GsA` fournit le socle à stockage explicite du bootstrap hébergé ;
- `ClassificateurMotsCles.GsE` constitue la preuve actuelle
  d’auto-hébergement partiel.

`gsppc --version` et `gsechargeur --version` annoncent tous deux 0.19.0.

## Construction et validation

Windows/MSVC :

```bat
cmake -S . -B Construction/Validation-GSPP-0190-MSVC ^
  -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build Construction/Validation-GSPP-0190-MSVC ^
  --config Release --target espace_travail --parallel
ctest --test-dir Construction/Validation-GSPP-0190-MSVC ^
  -C Release --output-on-failure
```

GNU/WSL :

```bash
cmake -S . -B Construction/Validation-GSPP-0190-GNU \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build Construction/Validation-GSPP-0190-GNU \
  --target espace_travail -j2
ctest --test-dir Construction/Validation-GSPP-0190-GNU \
  --output-on-failure
```

La validation du 16 août 2026 obtient 4/4 sous MSVC, 5/5 sous GNU/WSL et un
démarrage QEMU/OVMF réel réussi. Les détails et empreintes sont dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.19.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.19.0.md).

## Documentation principale

- cette référence conserve l’état historique du jalon 0.19 ;
- `HERITAGE_GS_PLUS_PLUS_0.19.md` fixe le nouveau contrat objet ;
- `COMPILATION_SEPAREE_GS_PLUS_PLUS_0.13.md` et `FORMAT_GSE_1.0.md` décrivent
  les conteneurs GsObj/GsA/GsE et leur base canonique 1.0 ;
- `Validations/VALIDATION-GS-PLUS-PLUS-0.17.1.md` consigne la migration
  normative vers `GSOBJ:0` et l’ABI 1 ;
- `FEUILLE_DE_ROUTE_GS_PLUS_PLUS.md` sépare les jalons terminés et prévus.

## Limites courantes

La 0.19 ne fournit pas l’héritage multiple/virtuel, les arguments explicites de
constructeurs de base, les virtuels purs, la RTTI, les conversions descendantes,
les références de champ/globale/tableau/retour, la synthèse récursive des
destructeurs de champs, les constructeurs globaux ni les exceptions. Elle ne
revendique pas non plus l’auto-hébergement complet du compilateur.
