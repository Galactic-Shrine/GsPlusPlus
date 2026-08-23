# Compilateur Gs++

Ce dossier contient exclusivement le compilateur natif Gs++ et ses outils :

- `gsppc` : compilation des sources, projets et solutions Gs++ ;
- `gseverifier` : validation des exécutables `.GsE` ;
- `gsechargeur` et `gseload` : chargement hébergé des `.GsE`.

Les formats et contrats publics se trouvent dans `../SDK`. Les bibliothèques,
l’auto-hébergement, les exemples et les tests résident dans leurs projets
respectifs sous `GsPlusPlus/`.

La construction normale s’effectue depuis la racine du monorepo :

```bash
make compiler
```
