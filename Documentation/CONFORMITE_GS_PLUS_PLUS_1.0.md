# Conformité Gs++ 1.0

**NORMATIF — suite initiale livrée en 0.24, étendue en 0.27.0-alpha.1 et
revalidée localement par Gs++ 0.27.0-alpha.10.**

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

## Matrice obligatoire de Gs++ 0.27.0-alpha.10

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
| `CONF-HOST-001` | profil hébergé | primitives 0.26 et exactement cinq imports `GalacticShrine::GsPP::Hote` dans `GsHebergee.GsA` et le GsE de test |
| `CONF-HOST-002` | profil système | absence de tout import `GalacticShrine::GsPP::Hote` dans `GsSysteme.GsA` |
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

### Extension locale des contrôles de projets

Dans les sources de développement après alpha.10, `CONF-PROJ-001` vérifie aussi
le choix explicite de l'expansion Gs++ par `--expanseur-inclusions` /
`--include-expander`, sans ajouter d'exigence au manifeste : le total reste 20.
La matrice croise les modes séparé/agrégé, les vocabulaires XML français/anglais
et les mots-clés source français/anglais, soit **huit scénarios**.

Pour chacun, les deux alias sont utilisés sur le projet bibliothèque, le projet
exécutable et la solution : **48 comparaisons** d'objets, archive, image et carte
avec le bootstrap. Les journaux de solution et l'ordre des projets sont comparés ;
les exécutables sont vérifiés et retournent 42. Le chemin de l'image est relatif
au processus et contient des espaces, tandis que les chemins XML restent relatifs
aux projets. Les remplacements de sortie et de répertoire d'objets sont vérifiés
sur un projet indépendant.

**56 refus différentiels** couvrent directives, lexage d'un fichier inclus,
corps interdit dans une interface, sémantique, inclusion absente, nom virtuel
d'une interface racine et source physique absente : deux modes, deux langues de
diagnostics et deux entrées (projet/solution). Codes et messages sont identiques
au bootstrap, et les sorties préexistantes sont conservées pour ces refus avant
le premier objet. Quatre images invalides/absentes sont refusées avant la
construction. Un échec du second projet vérifie séparément l'arrêt de la
solution sans annulation du premier, puis la réussite d'une nouvelle invocation.

Ces vérifications ne certifient ni l'authenticité d'une image native, ni une
transaction globale de construction, ni les passes auto-hébergées suivantes.
Elles n'actualisent pas les paquets alpha.10 déjà publiés.

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
