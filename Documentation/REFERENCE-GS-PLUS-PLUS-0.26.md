# Compilateur Gs++ 0.26.0 — référence courante

**VALIDÉ — 23 août 2026.**

Gs++ 0.26.0 conserve intégralement le langage et la durée de vie validés en
0.25.0. Son incrément produit est la bibliothèque hébergée nécessaire au début
de l’auto-hébergement du compilateur au jalon 0.27.

## Identité binaire inchangée

| Production | Extension | Signature | Format | ABI | En-tête |
| --- | --- | --- | ---: | ---: | ---: |
| objet | `.GsObj` | `GSOBJ:0` puis zéro | 1.0 | 1 | 112 octets |
| bibliothèque | `.GsA` | `GSA:0` puis trois zéros | 1.0 | 1 | 32 octets |
| exécutable | `.GsE` | `GSE:0` puis trois zéros | 1.0 | 1 | 112 octets |

Les signatures inter-unités commencent toujours par `GsAbi:x64-ms-v1`. Les
productions locales antérieures doivent être reconstruites, mais aucun format
natif n’est renuméroté.

## Bibliothèques

`GsSysteme.GsA` est freestanding et ne contient aucun import `Gs::Hote`.

`GsHebergee.GsA` fournit :

- validation UTF-8 stricte et chaînes propriétaires terminées par zéro ;
- vecteurs dynamiques d’octets et de naturels ;
- table de symboles dynamique à sondage linéaire ;
- arène par blocs garantissant la stabilité des adresses ;
- jointure de chemins et vues de nom ou d’extension ;
- chargement alloué et écriture de fichiers à résultat explicite ;
- erreurs par `CodeErreurHebergee`, sans exception.

La bibliothèque dépend exactement des cinq imports suivants :

```text
Gs::Hote::AllouerMemoire
Gs::Hote::LibererMemoire
Gs::Hote::LireFichier
Gs::Hote::EcrireFichier
Gs::Hote::EmettreDiagnostic
```

Chaque propriétaire possède une initialisation et une destruction explicites.
Une allocation échouée ne modifie pas le contenu antérieur d’une chaîne ou
d’un conteneur.

## Correction du langage requise par la bibliothèque

Une conversion explicite de pointeur conserve désormais les qualificatifs de
sa cible. La forme `convertir<constante T*>(T*)` produit donc réellement un
pointeur en lecture seule et peut initialiser ou retourner ce type.

## Conformité

La matrice 0.26 contient dix-huit exigences. Elle ajoute la présence exacte des
cinq imports hébergés et l’indépendance de `GsSysteme.GsA`. Le test exécutable
hébergé retourne `260` et vérifie notamment l’UTF-8 invalide, l’échec
d’allocation transactionnel, la croissance des conteneurs, la stabilité de
l’arène, les chemins et l’absence de fuite chez l’hôte de test.

Les constructions complètes réussissent 5/5 sous MSVC et 6/6 sous GNU/WSL.
La conformité réussit 18/18 sur les deux chaînes, les benchmarks smoke
réussissent leurs quatre scénarios sur chaque hôte et la preuve QEMU/OVMF
confirme mémoire, horloge et clavier. Le rapport complet est
[`Validations/VALIDATION-GS-PLUS-PLUS-0.26.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.26.0.md).

## Documents normatifs

- [`SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md`](SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md) ;
- [`BIBLIOTHEQUE_HEBERGEE_GS_PLUS_PLUS_0.26.md`](BIBLIOTHEQUE_HEBERGEE_GS_PLUS_PLUS_0.26.md) ;
- [`CONFORMITE_GS_PLUS_PLUS_1.0.md`](CONFORMITE_GS_PLUS_PLUS_1.0.md) ;
- [`ABI_GS_PLUS_PLUS_X64_MS_V1.md`](ABI_GS_PLUS_PLUS_X64_MS_V1.md) ;
- [`FORMAT_GSOBJ_1.0.md`](FORMAT_GSOBJ_1.0.md),
  [`FORMAT_GSA_1.0.md`](FORMAT_GSA_1.0.md) et
  [`FORMAT_GSE_1.0.md`](FORMAT_GSE_1.0.md).
