# Bibliothèque hébergée Gs++

`GsHebergee.GsA` fournit les briques utilisées par les outils exécutés dans un
système hôte :

- vues texte UTF-8 non propriétaires ;
- flux mémoire bornés ;
- vecteurs et tables de symboles à stockage explicite et parcours
  déterministe ;
- lecture/écriture de fichiers et diagnostics structurés par l’interface
  `Gs::Hote`.

La bibliothèque n’alloue pas implicitement. Le programme fournit les tampons
et l’hôte résout seulement trois imports : `LireFichier`, `EcrireFichier` et
`EmettreDiagnostic`.
