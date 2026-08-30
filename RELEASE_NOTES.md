# Notes de publication Gs++

## Dernière publication publique : 0.27.0-alpha.8

Gs++ 0.27.0-alpha.8 est une préversion publique du langage, du compilateur et
de sa chaîne native x86-64. Elle regroupe le classificateur, le lexeur,
l’analyseur syntaxique et la première passe sémantique auto-hébergée dans
l’unique image `Frontend.GsE`.

Cette publication préserve les contrats suivants :

- `.GsObj`, `.GsA` et `.GsE` en version 1.0 ;
- champs ABI fixés à 1 ;
- signatures `GSOBJ:0`, `GSA:0` et `GSE:0` ;
- préfixe public `GalacticShrine::GsPP::` ;
- compilation et validation sous Visual Studio 2026 et GNU/Linux ;
- comparaison bit à bit de l’image auto-hébergée entre les deux chaînes.

La preuve publiée est conservée dans
[`Documentation/Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.8.md`](Documentation/Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.8.md).
Le développement réalisé après cette publication est décrit dans le
[`CHANGELOG.md`](CHANGELOG.md) et dans la documentation du
[`frontend auto-hébergé 0.27`](Documentation/FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md).

Les archives de la publication sont :

- `GsPlusPlus-0.27.0-alpha.8-Windows-x86_64.zip` ;
- `GsPlusPlus-0.27.0-alpha.8-Linux-x86_64.tar.gz` ;
- `SHA256SUMS.txt`.

Cette version reste une préversion. Les notes détaillées des anciennes alphas
restent consultables dans l’historique Git et sur les releases GitHub.
