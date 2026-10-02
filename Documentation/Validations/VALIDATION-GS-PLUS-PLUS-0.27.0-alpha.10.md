# Validation Gs++ 0.27.0-alpha.10

**VALIDÉ — sources, constructions Visual Studio 2026, GNU/Linux,
MSBuild natif et paquets extraits — 2 octobre 2026.**

Cette matrice porte sur la préversion alpha.10, pas sur un frontend
0.27 complet ni sur Gs++ 1.0 stable. La
[preuve de publication alpha.9](VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.9.md)
reste historique et inchangée ; ses résultats ne sont pas attribués à alpha.10.

## Périmètre consolidé depuis alpha.9

- adaptations implicites composées, types effectifs et sélection de surcharges ;
- signatures et références de callbacks, types nommés contextuels, tableaux
  à indirections profondes et contraintes de paramètres ;
- opérateurs libres sur structures/unions ;
- chaînes d'alias de champs et alias racines de types, fonctions et globales ;
- appels via alias de méthodes non liées, récepteur mutable explicite,
  contrôle des arguments et visibilité des appels directs ;
- données globales et relocalisations de fonctions canoniques émises en mémoire ;
- 619 corpus sémantiques négatifs comparés au bootstrap, avec code, ligne,
  colonne et AST d'entrée intact vérifiés ;
- sortie bornée, sentinelles, absence d'écriture partielle et déterminisme ;
- formats GsObj/GsA/GsE 1.0, signatures GSOBJ:0/GSA:0/GSE:0 et ABI 1.

## Résultats locaux

| Contrôle | Windows / CMake VS 2026 | GNU/Linux WSL / CMake | VS 2026 / MSBuild natif |
| --- | ---: | ---: | ---: |
| Construction Release x64 | réussie | réussie | réussie, `.slnx` sans CMake |
| CTest | 5/5 | 6/6 | sans objet : validation native séparée |
| Tests unitaires et auto-hébergement | réussis | réussis | réussis |
| Conformité portable | 20/20 | 20/20 | 20/20 |
| Style Gs++ et cohérence des projets VS | réussis | réussis | réussis |
| Benchmark smoke | 4/4 | 4/4 | non exécuté sur cette construction |
| Vérification de Frontend.GsE | réussie | réussie | réussie, image identique |
| Bannières gsppc/gsechargeur/gseload | alpha.10 | alpha.10 | alpha.10 |

Le test supplémentaire GNU est l'intégration Bash. La validation native passe
par `VisualStudio/Validation.vcxproj`, en plus de la construction complète de
`GsPlusPlus.slnx`. Les sept cibles C++ natives sont contrôlées contre CMake.
Les outils fonctionnent sur deux hôtes ; les artefacts GsE conservent la cible
x86-64 et la convention `GsAbi:x64-ms-v1`, sans revendication de sortie ELF.

Commandes exécutées depuis la racine du dépôt :

```powershell
cmake --build --preset windows-release --target espace_travail --parallel 6
ctest --preset windows-release --output-on-failure
pwsh.exe -NoProfile -ExecutionPolicy Bypass -File Benchmarks/Invoke-GsPlusPlusBenchmark.ps1 -Mode smoke

& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' GsPlusPlus.slnx /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' VisualStudio\Validation.vcxproj /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
```

```bash
cmake --build --preset linux-release --target espace_travail --parallel 4
ctest --preset linux-release --output-on-failure
bash Benchmarks/invoke-gsplusplus-benchmark.sh --mode smoke
```

Sessions smoke (contrôle fonctionnel, pas campagne de performances) :

- Windows : `20261002T145221.700798Z-dc348116` ;
- GNU/Linux : `20261002T145217.455711Z-d4500a1a`.

## Version, image et ABI

`VERSION` contient `0.27.0-alpha.10`. Les en-têtes CMake/MSBuild générés, les
bannières des trois outils versionnés, les métadonnées de `Frontend.GsE`
et de `TestHebergee.GsE` et les configurations CPack annoncent cette version.
Le vérificateur `gseverifier` annonce le format GsE 1.0, pas la version produit.

Les trois images `Frontend.GsE` sont identiques bit à bit :

- taille : **366 719 octets** ;
- format GsE 1.0, ABI 1, trois segments, huit sections ;
- **75 exports**, deux imports d'hôte : allocation et libération ;
- SHA-256 :
  `4cccad8f5ad8b57168f9942caf00b5d37d4495d7a070071512cc0c1c3a8bf401`.

Chemins de construction :

- `Construction/CMake/VisualStudio/Release/` ;
- `Construction/CMake/Ninja/Release/` ;
- `Construction/MSBuild/x64/Release/`.

Les dispositions publiques restent inchangées : AST 64 octets par nœud,
requête sémantique 120 octets, requête d'émission 136 octets, descripteurs
globaux 48 octets et relocalisations 32 octets. Le récepteur des méthodes
est pris en compte dans la signature privée, sans ajout de nœud public.

## Distribution et contrôles des paquets

CPack reprend automatiquement la version centrale pour les noms :

- `GsPlusPlus-0.27.0-alpha.10-Windows-x86_64.zip` ;
- `GsPlusPlus-0.27.0-alpha.10-Linux-x86_64.tar.gz`.

Les deux archives extraites réussissent **11/11 contrôles** avec le pilote
[`verifier_paquet.py`](../../Tests/Distribution/verifier_paquet.py) : présence
des fichiers et interfaces publiques identiques aux sources, absence de caches
et d'artefacts natifs intermédiaires, versions des outils et du SDK, format et
empreinte du frontend, compilation de `Bonjour.Gs++`, client des quatre
interfaces du frontend, exécution des callbacks d'alias de méthodes en français
et en anglais (retour 42), bibliothèque système (retour 64), bibliothèque
hébergée et suite différentielle (619 refus) contre le frontend livré.

La validation compile ses clients avec les outils, interfaces et bibliothèques
extraits ; le pilote natif de test du frontend est celui de la construction
de référence. Les sorties de validation restent hors de l'archive extraite.
Les archives définitives sont reconstruites depuis un export Git du commit
signé de publication, puis extraites et contrôlées à nouveau avant mise en ligne.
Le manifeste `SHA256SUMS.txt` est calculé après ce dernier paquetage, hors des
archives pour éviter une référence circulaire.

Livrables et rapports finaux : `Construction/GsPlusPlus-0.27.0-alpha.10/`.
La CI distante reste une preuve distincte, consultable dans les exécutions
GitHub Actions du commit/tag publié ; elle ne remplace pas les tests des paquets.

Exemple de validation après extraction (adapter le suffixe `.exe` sous Linux) :

```powershell
python Tests/Distribution/verifier_paquet.py `
  --package-root <repertoire-du-paquet-extrait> `
  --source-root <export-du-commit-de-publication> `
  --reference-frontend <Frontend.GsE-de-la-construction-de-reference> `
  --auto-test <gspp_autohebergement_tests.exe-de-la-construction-de-reference> `
  --work-root <nouveau-repertoire-de-validation>
```

## Limites conservées

- frontend partiel : qualifications, contraintes d'héritage et autres familles
  sémantiques restent à compléter ;
- prise d'adresse d'une méthode privée/protégée conforme au comportement actuel
  du bootstrap, sans contrôle supplémentaire de visibilité sur les callbacks ;
- validation intégrale des appels virtuels via alias non revendiquée ;
- données globales émises en mémoire, pas encore de fichier objet produit par
  un backend auto-hébergé ; génération machine et liaison assurées par le C++ ;
- sorties natives Windows PE/Linux ELF, SDK de destination et matrice croisée
  toujours prévus, non implémentés ;
- `.GsA` reste courant ; migration `.Glib` et réservation `.GdLib` prévues pour
  0.28.0, sans annonce de support dynamique déjà livré.
