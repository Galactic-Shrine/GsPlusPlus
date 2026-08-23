# Format de bibliothèque GsA 1.0

GsA 1.0 est le format de bibliothèque native canonique de Gs++ 0.26.0. Tous
les entiers sont non signés, en petit-boutiste. L’ABI de l’en-tête vaut `1`.

## En-tête de 32 octets

| Position | Taille | Champ |
| ---: | ---: | --- |
| 0 | 5 | signature ASCII `GSA:0` |
| 5 | 3 | réservé, nul |
| 8 | 2 | version majeure, `1` |
| 10 | 2 | version mineure, `0` |
| 12 | 4 | taille de l’en-tête, `32` |
| 16 | 4 | nombre de membres |
| 20 | 2 | ABI, `1` |
| 22 | 2 | réservé, nul |
| 24 | 8 | taille totale du fichier |

Chaque membre commence par une entrée de 16 octets : longueur du nom sur
4 octets, champ réservé nul sur 4 octets et taille du GsObj sur 8 octets. Le
nom UTF-8 terminé par zéro puis le contenu GsObj sont chacun alignés sur
16 octets.

Le lecteur refuse les anciennes signatures, toute version différente de 1.0,
une ABI différente de 1, les champs réservés non nuls, les objets invalides et
les tailles incohérentes. Les bibliothèques locales antérieures doivent être
reconstruites.
