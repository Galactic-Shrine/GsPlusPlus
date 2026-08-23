# Compilateur Gs++ 0.20.0 — référence historique

> La référence courante est
> [`REFERENCE-GS-PLUS-PLUS-0.22.md`](REFERENCE-GS-PLUS-PLUS-0.22.md). Ce
> document conserve le contrat exact du jalon 0.20 avant l’ajout des
> initialisateurs de champs.

## Portée

Gs++ 0.20.0 est le compilateur système natif de Galactic-Shrine et de
Sanctuaire SE. Son bootstrap reste principalement écrit en C++, mais le
langage, ses formats natifs et son backend AMD64 sont indépendants du langage
C++ source.

Cette version conserve le modèle objet et l’héritage simple public de 0.19,
puis ajoute l’initialisation explicite de la base directe et l’appel direct de
l’implémentation héritée. Le Markdown est la documentation principale ; les
`.docx` éventuels ne sont que des copies de consultation.

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
refusées. Le passage à 0.20 ne renumérote aucun format et aucun champ ABI.

## État fonctionnel

| Domaine | État en 0.20.0 |
|---|---|
| Lexeur/parser bilingue FR/EN | validé |
| Types système, tableaux, unions, énumérations | validé |
| Pointeurs et pointeurs de fonction | validé |
| Valeurs structurées et agrégats | validé |
| Compilation séparée GsObj/GsA/GsE | validé |
| Classes, visibilité, surcharge et RAII local | validé |
| Virtuels et héritage simple public | validé |
| `remplacer/override` et dispatch par base | validé |
| `: parent(...)` / `: super(...)` | validé en 0.20.0 |
| `parent.Methode()` / `super.Method()` direct | validé en 0.20.0 |
| Chaînes construction/destruction | validé en 0.20.0 |
| Héritage multiple/virtuel et RTTI | prévu |
| Auto-hébergement complet | partiel, non revendiqué |

## Exemple 0.20

```gspp
classe Base
{
    protégée: entier32 Valeur;
    publique:
        constructeur(entier32 valeur) { soi.Valeur = valeur; }
        virtuel entier32 Lire() { retourner soi.Valeur; }
        virtuel destructeur() {}
};

classe Derivee : publique Base
{
    privée: entier32 Bonus;
    publique:
        constructeur(entier32 valeur, entier32 bonus)
            : parent(valeur)
        { soi.Bonus = bonus; }

        remplacer entier32 Lire()
        { retourner parent.Lire() + soi.Bonus; }

        remplacer destructeur() {}
};

publique entier32 Principal()
{
    Derivee valeur(40, 2);
    Base& vue = valeur;
    retourner vue.Lire();
}
```

L’appel retourne `42`. `parent.Lire()` est un appel direct vers `Base::Lire`
depuis le remplacement, tandis que `vue.Lire()` conserve le dispatch virtuel
et atteint `Derivee::Lire`.

## Construction, disposition et ABI objet

Une classe dérivée commence toujours par sa base complète. Les remplacements
virtuels conservent les indices hérités et les nouveaux virtuels sont ajoutés à
la fin. La signature ABI récursive encode la base, les tailles, les alignements,
le décalage de table et le fournisseur de chaque emplacement.

Le prologue de chaque constructeur déclaré appelle son constructeur de base,
installe les tables virtuelles des éventuelles étapes sans constructeur, puis
la table de la classe courante avant d’exécuter le corps. Sans initialiseur
explicite, la sélection sans argument reste automatique. Avec
`: parent(arguments)`, les arguments sont résolus contre le constructeur de la
base directe.

Le contrat 0.20 complet est décrit dans
[`INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md`](INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md).
Le socle d’héritage reste documenté dans
[`HERITAGE_GS_PLUS_PLUS_0.19.md`](HERITAGE_GS_PLUS_PLUS_0.19.md).

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

`gsppc --version` et `gsechargeur --version` annoncent tous deux 0.20.0.

## Construction et validation

Windows/MSVC :

```bat
cmake -S . -B Construction/Validation-GSPP-0200-MSVC ^
  -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build Construction/Validation-GSPP-0200-MSVC ^
  --config Release --target espace_travail --parallel
ctest --test-dir Construction/Validation-GSPP-0200-MSVC ^
  -C Release --output-on-failure
```

GNU/WSL :

```bash
cmake -S . -B Construction/Validation-GSPP-0200-GNU \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build Construction/Validation-GSPP-0200-GNU \
  --target espace_travail -j2
ctest --test-dir Construction/Validation-GSPP-0200-GNU \
  --output-on-failure
```

La validation du 16 août 2026 obtient 4/4 sous MSVC, 5/5 sous GNU/WSL et un
démarrage QEMU/OVMF réel réussi. Les détails et empreintes sont dans
[`Validations/VALIDATION-GS-PLUS-PLUS-0.20.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.20.0.md).

## Documentation principale

- cette référence conserve l’état historique du jalon 0.20 ;
- `INITIALISATION_PARENT_GS_PLUS_PLUS_0.20.md` fixe le nouveau contrat ;
- `HERITAGE_GS_PLUS_PLUS_0.19.md` fixe le socle d’héritage ;
- `COMPILATION_SEPAREE_GS_PLUS_PLUS_0.13.md` et `FORMAT_GSE_1.0.md` décrivent
  les conteneurs GsObj/GsA/GsE et leur base canonique 1.0 ;
- `Validations/VALIDATION-GS-PLUS-PLUS-0.17.1.md` consigne la migration
  normative vers `GSOBJ:0` et l’ABI 1 ;
- `FEUILLE_DE_ROUTE_GS_PLUS_PLUS.md` sépare les jalons terminés et prévus.

## Limites courantes

La 0.20 ne fournit pas l’héritage multiple/virtuel, plusieurs initialisateurs
de bases ou de champs, les virtuels purs, la RTTI, les conversions descendantes,
les références de champ/globale/tableau/retour, la synthèse récursive des
destructeurs de champs, les constructeurs globaux ni les exceptions. Elle ne
revendique pas non plus l’auto-hébergement complet du compilateur.
