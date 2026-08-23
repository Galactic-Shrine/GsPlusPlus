# Validation de Gs++ 0.27.0-alpha.1

**ALPHA VALIDÉE POUR PUBLICATION — 23 août 2026.**

Cette preuve porte sur la première préversion publique autonome de Gs++. Elle
ne déclare ni Gs++ 0.27 final ni le frontend auto-hébergé complet comme prêts
pour la production.

## Périmètre livré

- outils natifs `gsppc`, `gseverifier`, `gsechargeur` et `gseload` ;
- bibliothèques `GsSysteme.GsA` et `GsHebergee.GsA` ;
- classificateur de mots-clés et lexeur auto-hébergés ;
- formats GsObj, GsA et GsE 1.0, ABI 1 ;
- projets `.GsPj`/`.GsProject` et solutions `.GsPs` au format XML 1.0 ;
- tests, conformité, exemples, SDK et documentation Markdown.

`SanctuaireSE`, ShrineOS et `Noyau.GsE` sont volontairement absents du dépôt,
du graphe CMake autonome et des paquets. Leur intégration pourra consommer les
artefacts Gs++ comme un produit externe, mais Gs++ ne les consomme pas.

## Chaînes validées

### Windows

- générateur : Visual Studio 2026 x64 avec CMake 4.4.0 ;
- compilateur : MSVC 19.51.36256.0, outils v145 ;
- configuration : Release ;
- construction autonome depuis `GsPlusPlus/CMakeLists.txt` ;
- CTest : 3/3 réussis ;
- conformité : 20/20 réussie ;
- benchmark smoke : 4/4 scénarios réussis.

### GNU/Linux sous WSL2

- compilateur : GNU C++ 11.4.0 ;
- générateur : Ninja, CMake 3.22.1 ;
- configuration : Release dans le système de fichiers Linux `/home` ;
- CTest : 4/4 réussis, y compris l’intégration GNU ;
- conformité : 20/20 réussie ;
- benchmark smoke : 4/4 scénarios réussis.

La configuration Linux a été contrôlée sans aucun fichier ou cible contenant
`Sanctuaire` ou `Noyau`.

## Preuve reproductible du lexeur

L’image `Lexeur.GsE` produite indépendamment par MSVC et GNU est identique :

```text
taille  : 40 897 octets
SHA-256 : 583a2a8c4b31fc742548e54831df51dd6d7baea2a52a72d28c4bf5072a32aa0a
```

La conformité vérifie également les signatures `GSOBJ:0`, `GSA:0` et `GSE:0`,
les versions de format 1.0 et les champs ABI à 1.

## XML 1.0

`CONF-PROJ-001` construit une solution comprenant un projet français `.GsPj`
et un projet anglais `.GsProject`, exécute le GsE obtenu et vérifie son code de
retour 44. `CONF-NEG-007` prouve que l’ancien format `clé = valeur` est refusé.
Le contrat complet est
[`../FORMAT_PROJETS_GS_PLUS_PLUS_1.0.md`](../FORMAT_PROJETS_GS_PLUS_PLUS_1.0.md).

## Limites explicites de l’alpha

- le parseur et l’analyse sémantique auto-hébergés ne sont pas encore livrés ;
- l’interface en ligne de commande et le schéma XML peuvent encore évoluer
  avant Gs++ 1.0, avec une nouvelle version de schéma si nécessaire ;
- le profil de génération natif reste x86-64 avec ABI
  `GsAbi:x64-ms-v1` ;
- aucune promesse de compatibilité binaire avec une préversion ultérieure n’est
  faite au-delà des formats 1.0 explicitement documentés.

Ces limites justifient le canal GitHub « pre-release » et le suffixe
`alpha.1`.
