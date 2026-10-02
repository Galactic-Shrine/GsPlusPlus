# Visual Studio 2026 — construction native

Ouvrir `GsPlusPlus.slnx` à la racine, sélectionner `Release | x64` ou
`Debug | x64`, puis construire la solution. Installer le composant
**Développement Desktop en C++**, MSVC **v145** et le SDK Windows 10/11.
Windows PowerShell 5.1 est utilisé pour les étapes Gs++ ; Python 3 est requis
uniquement pour la validation de style et de conformité.

Cette solution utilise directement les tâches C++ MSBuild, `cl.exe` et
`link.exe`. Elle ne lance ni CMake, ni Ninja, ni CTest. Aucun fichier généré
par CMake n'est nécessaire. Les deux systèmes de construction restent proposés.

Depuis un terminal développeur Visual Studio 2026, à la racine du dépôt :

```powershell
msbuild GsPlusPlus.slnx /m /p:Configuration=Release /p:Platform=x64
msbuild VisualStudio/Validation.vcxproj /m /p:Configuration=Release /p:Platform=x64
```

- `gspp_compiler` : bibliothèque C++ du bootstrap ;
- `gsppc`, `gseverifier`, `gsechargeur`, `gseload` : outils natifs Windows ;
- `Artefacts` : bibliothèques `.GsA`, objets intermédiaires et `Frontend.GsE` ;
- `gspp_tests`, `gspp_autohebergement_tests` : exécutables de tests ;
- `Validation` : exécute les tests, le contrôle de style et la conformité.
  Cette dernière cible est volontairement exclue de la construction globale ;
  la construire explicitement pour lancer les tests.

Les sorties sont isolées dans `Construction/MSBuild/x64/<Configuration>/`.
`Bin/` contient les outils ; `Artefacts/GsPlusPlus/` les fichiers Gs++.
`VERSION` reste la source unique de version ; le même modèle d'en-tête est
utilisé par CMake et MSBuild. Le paramètre MSBuild `GsPython` permet de choisir
un autre interpréteur Python. `Rebuild` reconstruit les outils puis les artefacts.

Les originaux graphiques sont externes au dépôt. `Assets/Gs++.png` est la copie
de diffusion conservée pour les README et les paquets ; elle ne participe pas
à la compilation. Gs++ n'a aucune dépendance de construction à Gs# ou au noyau.

## English

Open the root `GsPlusPlus.slnx` with Visual Studio 2026 and build either
`Release | x64` or `Debug | x64`. Install Desktop development with C++, MSVC
v145 and the Windows SDK. This is a native MSBuild build: CMake, Ninja and
CTest are not required. Windows PowerShell runs the Gs++ artifact steps.
Python 3 is only needed by the explicit `Validation.vcxproj` target.

Use the commands above from a VS developer terminal. Outputs are under
`Construction/MSBuild/x64/<Configuration>/`. `VERSION` is shared with CMake;
the compiler, libraries and self-hosted frontend are built from this checkout.
Build `Validation` explicitly to run unit, self-hosting, style and conformance
tests. The normal solution build does not run those tests automatically.
