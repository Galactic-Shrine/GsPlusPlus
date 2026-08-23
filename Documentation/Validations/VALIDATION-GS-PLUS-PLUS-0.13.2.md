# Validation du compilateur Gs++ 0.13.2

> Document historique : `GSOBJ\0` était la signature de ce jalon. Gs++ 0.17.1
> la remplace par `GSOBJ:0` et fixe GsObj/GsA/GsE 1.0 avec l’ABI 1.

Date : 22 juillet 2026.

## Objet de la révision

La version 0.13.2 rend le format objet natif immédiatement identifiable :

- octets 0 à 5 : signature exacte `GSOBJ\0`, soit `47 53 4f 42 4a 00` ;
- octets 6 et 7 : réservés et obligatoirement nuls ;
- octets 8 et 9 : version majeure 1 en petit-boutiste ;
- octets 10 et 11 : version mineure 0 en petit-boutiste ;
- taille totale de l’en-tête conservée à 112 octets.

Les champs suivants ont été déplacés ensemble afin de préserver leur
alignement naturel. Le lecteur refuse l’ancienne signature provisoire
`GSO\0`, une signature partielle, un octet réservé non nul et toute version
inconnue. Les objets `.GsObj` produits avec la version 0.13.1 doivent donc être
recompilés.

Cette modification n’affecte ni la syntaxe Gs++, ni les interfaces `.GsPPH` et
`.GsPlusPlusHeader`, ni les bibliothèques GsA, ni le format exécutable GsE 2.0,
ni le chargeur UEFI.

## Validation

La reconstruction complète valide :

- production de la signature binaire exacte `GSOBJ\0` ;
- présence des deux octets réservés nuls ;
- lecture de la version 1.0 aux nouveaux décalages ;
- refus d’un octet réservé non nul et d’une version inconnue ;
- compilation séparée, bibliothèques GsA et liaison multi-unités ;
- diagnostic des incompatibilités ABI et reproductibilité des objets ;
- projets `.GsPj`/`.GsProject`, solutions `.GsPs` et cartes de liens ;
- exécution native du programme lié avec le code de retour `44` ;
- tests unitaires, intégration, GsE, PE32+, UEFI statique et FAT32 ;
- AddressSanitizer et UndefinedBehaviorSanitizer ;
- reconstruction autonome depuis l’archive source.

Sanctuaire SE reste gelé sur sa référence 0.10.2, déjà validée en démarrage
UEFI réel. Cette révision concerne uniquement la chaîne de compilation Gs++.
