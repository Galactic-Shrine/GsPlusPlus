# SDK Gs++

Le SDK contient les contrats publics partagés par plusieurs composants :

- format exécutable `.GsE` ;
- format objet `.GsObj` ;
- format de bibliothèque `.GsA` ;
- contrat de démarrage utilisé par le chargeur hébergé et Sanctuaire SE.

Les signatures courantes sont `GSOBJ:0`, `GSA:0` et `GSE:0`. Les formats
restent en version 1.0 et l’ABI native vaut 1.

Le compilateur et le chargeur UEFI incluent ce dossier sans dépendre de leurs
sources internes respectives.
