# Bibliothèque système de Gs++ 0.16

## Objectif

`GsSysteme.GsA` est la première bibliothèque standard freestanding écrite en
Gs++. Elle peut être liée à un noyau ou à une application native sans tas,
exception, constructeur global ni runtime implicite. Son interface canonique
est `GsPlusPlus/Bibliotheques/Systeme/Systeme.HGsPP`.

Les déclarations françaises se trouvent dans `Gs::Systeme`. Les alias anglais
correspondants sont disponibles dans `Gs::System` et désignent exactement les
mêmes symboles. Les types principaux sont :

```text
Gs::Systeme::VueOctets       / Gs::System::ByteView
Gs::Systeme::VueTexte        / Gs::System::TextView
Gs::Systeme::VerrouAtomique  / Gs::System::SpinLock
```

Les vues ne possèdent pas leur stockage. Leur utilisateur demeure responsable
de sa durée de vie et de sa validité.

## Modules

La bibliothèque GsA contient quatre objets indépendants :

| Module | Services |
|---|---|
| `Memoire.GsPP` | remplissage, copie, comparaison et égalité d’octets |
| `Vues.GsPP` | vues d’octets, vues de texte, sous-vues, longueur et recherche |
| `Bits.GsPP` | rotations, comptage de bits, inversion d’octets et alignement |
| `Atomiques.GsPP` | charges, stockages, échanges, ajout, comparaison-échange et verrou |

L’éditeur de liens n’extrait que les membres requis. Depuis la 0.16, une
fonction simplement déclarée dans une interface mais jamais référencée ne crée
plus un import artificiel.

## Opérateurs binaires

Le langage reconnaît désormais :

```text
~valeur
gauche & droite
gauche ^ droite
gauche | droite
valeur << distance
valeur >> distance
```

`&`, `^` et `|` exigent deux entiers de même type, après adaptation éventuelle
d’une constante. Les décalages acceptent un entier d’un autre type à droite et
conservent le type de gauche. Un décalage à droite est arithmétique pour un
entier signé et logique pour un entier non signé.

La distance est réduite modulo 8, 16, 32 ou 64 selon la largeur du type. Cette
règle est identique dans l’évaluation constante et dans le code x86-64.

L’ordre de priorité, du plus fort au plus faible, est : multiplication,
addition, décalage, comparaison, égalité, `&`, `^`, `|`, affectation.

## Atomiques x86-64

Les fonctions réservées de `Gs::Intrinseques` sont reconnues uniquement lors
d’un appel direct possédant le prototype exact. Le backend les remplace par :

| Primitive | Instruction principale |
|---|---|
| charge 32/64 | `mov` |
| stockage 32/64 | `mov` |
| échange 32/64 | `xchg` |
| ajout avec ancienne valeur | `lock xadd` |
| comparaison-échange | `lock cmpxchg` |
| barrière complète | `mfence` |
| attente active courtoise | `pause` |

Ces primitives ne produisent aucune relocalisation et n’apparaissent pas dans
la table d’imports du GsObj ou du GsE. Un prototype réservé incorrect est
refusé pendant l’analyse sémantique.

Les fonctions d’ajout, d’échange et de comparaison-échange retournent la
valeur précédente. `ComparerEchanger32/64` remplace la valeur uniquement si
elle est égale à `attendu`.

Le stockage doit être naturellement aligné sur quatre ou huit octets. La
version 0.16 cible exclusivement le modèle mémoire x86-64 ; une architecture
future devra fournir son propre backend pour conserver les mêmes contrats.

## Verrou léger

`VerrouAtomique` contient un mot `naturel32` volatile. Son cycle normal est :

```gspp
Gs::System::SpinLock verrou = {0};
Gs::System::InitializeLock(&verrou);
Gs::System::AcquireLock(&verrou);
// section critique
Gs::System::ReleaseLock(&verrou);
```

`TryAcquireLock` n’attend pas. `AcquireLock` utilise `pause` entre deux essais.
Le verrou n’est ni récursif ni équitable et ne doit pas être libéré par un
contexte qui ne le possède pas.

## Construction et liaison

```bash
make bibliotheque-systeme

Construction/Bin/gsppc GsPlusPlus/Bibliotheques/Systeme/Systeme.HGsPP Application.GsPP \
    --format gsobj -o Construction/Application.GsObj

Construction/Bin/gsppc Construction/Application.GsObj Construction/Artefacts/GsPlusPlus/Bibliotheques/Systeme/GsSysteme.GsA \
    --format gse --point-entree Principal -o Construction/Application.GsE
```

Le projet source `GsPlusPlus/Bibliotheques/Systeme/GsSysteme.GsPj` construit les quatre
GsObj puis l’archive déterministe.

## Contrats et limites

- `CopierMemoire` exige des plages valides et sans chevauchement ;
- une taille nulle ne lit ni n’écrit la mémoire ;
- une recherche absente retourne la taille de la vue ;
- une sous-vue est tronquée à la fin de la vue d’origine ;
- `AlignerSuperieur` retourne la valeur inchangée pour un alignement nul et
  suppose un alignement puissance de deux dans les autres cas ;
- les fonctions de texte utilisent des octets `caractère` et ne décodent pas
  encore les points de code UTF-8 ;
- aucune fonction de cette bibliothèque n’alloue ou ne libère de stockage.

La bibliothèque hébergée destinée aux fichiers, diagnostics et conteneurs du
compilateur appartient au jalon d’auto-hébergement 0.17. Elle restera séparée
du noyau freestanding.
