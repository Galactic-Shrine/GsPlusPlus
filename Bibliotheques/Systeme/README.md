# GsSysteme 0.16.0

Bibliothèque système freestanding de Gs++ pour x86-64.

Fichiers importants :

- `Systeme.HGsPP` : interface bilingue ;
- `GsSysteme.GsPj` : projet de construction de la bibliothèque ;
- `Memoire.GsPP`, `Vues.GsPP`, `Bits.GsPP`, `Atomiques.GsPP` : sources ;
- `GsSysteme.GsA` : archive binaire, lorsqu’elle est fournie dans le paquet
  préconstruit.

Construction depuis la racine du compilateur :

```bash
make bibliotheque-systeme
```

Liaison :

```bash
Construction/Bin/gsppc Systeme.HGsPP Application.GsPP \
    --format gsobj -o Application.GsObj
Construction/Bin/gsppc Application.GsObj GsSysteme.GsA \
    --format gse --point-entree Principal -o Application.GsE
```

L’API française est dans `GalacticShrine::GsPP::Systeme`; les alias anglais sont dans
`GalacticShrine::GsPP::System`. La documentation complète se trouve dans
`docs/BIBLIOTHEQUE_SYSTEME_GS_PLUS_PLUS_0.16.md` du paquet compilateur.
