# Validation du compilateur Gs++ 0.13.1

> Document historique : les signatures et numéros de formats indiqués décrivent
> ce jalon local. Gs++ 0.17.1 fixe `GSOBJ:0`, GsObj/GsA/GsE 1.0 et ABI 1.

Date : 22 juillet 2026.

## Objet de la révision

La version 0.13.1 ajuste uniquement les conventions de fichiers de la
compilation séparée :

- les interfaces courtes utilisent `.GsPPH` ;
- les interfaces longues conservent `.GsPlusPlusHeader` ;
- les objets natifs utilisent `.GsObj` ;
- l’option de sortie canonique devient `--format gsobj`.

Le format binaire objet reste **GsO 1.0**, avec la signature `GSO\0`, les mêmes
tables, les mêmes contrôles ABI et la même compatibilité avec les bibliothèques
`GsA 1.0`. Cette révision ne modifie donc ni le code machine généré, ni GsE 2.0,
ni le chargeur UEFI.

## Validation

La reconstruction complète valide :

- reconnaissance de `.GsPPH` et `.GsPlusPlusHeader` comme interfaces ;
- production et relecture d’objets `.GsObj` ;
- création d’une bibliothèque GsA à partir de plusieurs `.GsObj` ;
- liaison d’un `.GsObj` avec une GsA ;
- diagnostic ABI mentionnant le fichier `.GsPPH` fautif ;
- projets `.GsPj` et `.GsProject` produisant des membres `.GsObj` ;
- solution `.GsPs` et exécution native avec code de retour `44` ;
- tests unitaires, intégration, GsE, PE32+, UEFI statique et FAT32 ;
- AddressSanitizer et UndefinedBehaviorSanitizer ;
- reproductibilité bit à bit de l’archive source et des sorties.

Sanctuaire SE reste gelé sur la référence 0.10.2 déjà validée en démarrage UEFI
réel. Aucun nouveau démarrage QEMU n’est revendiqué pour cette révision de
nomenclature.
