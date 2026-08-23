# Conformité Gs++ 1.0

**NORMATIF — suite initiale livrée en 0.24, étendue en 0.27.0-alpha.1 et
revalidée par Gs++ 0.27.0-alpha.3.**

La conformité distingue le contrat produit des tests de développement. Une
construction Gs++ n’est pas déclarée conforme parce qu’elle compile : elle
doit exécuter la suite portable, produire un rapport JSON complet et réussir
chaque exigence obligatoire.

## Sources de vérité

- manifeste :
  [`../Tests/Conformite/conformite.json`](../Tests/Conformite/conformite.json) ;
- exécuteur portable :
  [`../Tests/Conformite/executer_conformite.py`](../Tests/Conformite/executer_conformite.py) ;
- corpus minimal :
  [`../Tests/Conformite/Corpus`](../Tests/Conformite/Corpus) ;
- rapport de construction :
  `Construction/<générateur>/Release/Tests/GsPlusPlus/Conformite/rapport.json`.

Le manifeste est lisible par machine. Le présent document explique la portée
des exigences ; il ne remplace ni le manifeste ni les contrats de format.

## Matrice obligatoire de Gs++ 0.27.0-alpha.3

| Identifiant | Domaine | Preuve |
| --- | --- | --- |
| `CONF-CLI-001` | interface | versions exactes de `gsppc` et `gsechargeur` |
| `CONF-EXT-001` | extensions | compilation de `.Gs++`, `.GsPP` et `.GsPlusPlus` |
| `CONF-EXT-002` | extensions | compilation de `.HGs++`, `.HGsPP` et `.HeaderGsPlusPlus` |
| `CONF-FMT-001` | GsObj | signature `GSOBJ:0`, en-tête 112, version 1.0, ABI 1 |
| `CONF-FMT-002` | GsA | signature `GSA:0`, en-tête 32, version 1.0, ABI 1 |
| `CONF-FMT-003` | GsE | signature `GSE:0`, en-tête 112, version 1.0, ABI 1, validation et exécution |
| `CONF-ABI-001` | ABI | présence de `GsAbi:x64-ms-v1` dans GsObj |
| `CONF-LANG-001` | langage | programmes français et anglais retournant tous deux 24 |
| `CONF-LIFE-001` | durée de vie | valeurs par défaut, délégation, tableaux avec arguments, construction `123`, destruction `321` et retour 25 |
| `CONF-HOST-001` | profil hébergé | primitives 0.26 et exactement cinq imports `Gs::Hote` dans `GsHebergee.GsA` et le GsE de test |
| `CONF-HOST-002` | profil système | absence de tout import `Gs::Hote` dans `GsSysteme.GsA` |
| `CONF-DET-001` | reproductibilité | égalité binaire de deux GsObj, GsA et GsE produits à entrées identiques |
| `CONF-PROJ-001` | projets | construction et exécution d’une solution XML 1.0 contenant des projets français et anglais |
| `CONF-NEG-001` | refus | refus de la sortie obsolète `.GsO` |
| `CONF-NEG-002` | refus | refus de l’interface obsolète `.GsPPH` |
| `CONF-NEG-003` | routage | refus explicite de `.Gs#`, réservé au compilateur Gs# futur |
| `CONF-NEG-004` | robustesse | refus d’un GsE tronqué par `gseverifier` |
| `CONF-NEG-005` | durée de vie | refus d’un objet de classe global sans runtime caché |
| `CONF-NEG-006` | durée de vie | refus d’un cycle de délégation entre constructeurs |
| `CONF-NEG-007` | projets | refus de l’ancien format texte `clé = valeur` |

Les fichiers Gs# ne possèdent aucun fichier d’en-tête et ne sont jamais
interprétés par `gsppc`.

## Exécution par CMake

La suite est enregistrée dans CTest sous le nom `gspp_conformite`. Elle utilise
uniquement Python 3 et les trois outils construits dans la même configuration :
`gsppc`, `gseverifier` et `gsechargeur`.

Depuis la racine du dépôt Gs++ sous Windows :

```powershell
cmake --preset windows-release
cmake --build --preset windows-release --target preparer_tests
ctest --preset windows-release -R gspp_conformite
```

Sous GNU/WSL :

```bash
cmake --preset linux-release
cmake --build --preset linux-release --target preparer_tests
ctest --preset linux-release -R gspp_conformite
```

Un succès CTest sans rapport JSON est incomplet. Le rapport doit annoncer
`etat: réussi`, vingt cas, vingt réussites et zéro échec.

## Portée et limites

La matrice 0.27 alpha verrouille l’identité du produit, ses extensions, ses formats,
son ABI minimale, son bilinguisme de base, sa reproductibilité locale, les
fondations de la durée de vie 0.25, la frontière entre bibliothèques système et
hébergée ainsi que ses refus essentiels. Elle ne prétend pas encore couvrir
toutes les productions du langage.

Les tests unitaires, le scénario d’intégration GNU, l’auto-hébergement partiel
et les benchmarks restent donc obligatoires en plus de cette suite. La preuve
Sanctuaire SE/QEMU appartient à l’intégration de ShrineOS et n’est pas une
dépendance ni une barrière de publication du produit Gs++ autonome. Les jalons
suivants étendront le manifeste sans changer silencieusement le sens des
identifiants existants.

## Règle de publication

Une version candidate échoue à la conformité si un seul cas obligatoire
échoue, si un cas du manifeste n’est pas exécuté, si la version publiée par les
outils diverge du manifeste ou si les preuves ont été produites par des
binaires d’une autre construction.

La conformité sur MSVC et GNU est nécessaire pour publier Gs++ autonome. Une
preuve QEMU peut démontrer séparément la compatibilité avec Sanctuaire SE, mais
elle ne requiert aucune inclusion de `Noyau.GsE` dans Gs++.
