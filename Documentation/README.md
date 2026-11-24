# Documentation Gs++

Ce dossier contient les contrats courants, la dernière matrice de validation
de Gs++ et la preuve historique de publication alpha.9.
Les rapports de jalons remplacés ont été retirés du dépôt
actif afin que la documentation publiée reste lisible ; ils demeurent
consultables dans l’historique Git.

## Langage et produit

- [Spécification candidate du langage 1.0](SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md)
- [Inclusions et utilisation d'espaces : règles et limites](SPECIFICATION_LANGAGE_GS_PLUS_PLUS_1.0.md#inclusion-textuelle-et-utilisation-despaces-de-noms)
- [Conventions de code Gs++ 1.0](CONVENTIONS_CODE_GS_PLUS_PLUS_1.0.md)
- [Profils freestanding et hébergé](PROFILS_GS_PLUS_PLUS_1.0.md)
- [Plan produit vers Gs++ 1.0](PLAN_PRODUIT_GS_PLUS_PLUS_1.0.md)
- [Feuille de route](FEUILLE_DE_ROUTE_GS_PLUS_PLUS.md)

## Chaîne native

- [Format XML des projets et solutions 1.0](FORMAT_PROJETS_GS_PLUS_PLUS_1.0.md)
- [Format objet GsObj 1.0](FORMAT_GSOBJ_1.0.md)
- [Format bibliothèque GsA 1.0](FORMAT_GSA_1.0.md)
- [Format exécutable GsE 1.0](FORMAT_GSE_1.0.md)
- [ABI native x86-64](ABI_GS_PLUS_PLUS_X64_MS_V1.md)

## Validation et développement

- [Correctif CI Windows : encodage UTF-8 des arguments et chemins (24 novembre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#correction-de-lencodage-des-processus-windows--24-novembre-2026)
- [Appels indirects : cibles, arités et priorités (24 novembre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#appels-indirects--cibles-et-arités--24-novembre-2026)
- [Groupes mixtes d'alias importés : nom appelé, priorités et compatibilité (24 novembre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#groupes-mixtes-dalias-importés--24-novembre-2026)
- [Appels d'alias refusés : noms canoniques et compatibilité (24 novembre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#appels-dalias-refusés-possédés--24-novembre-2026)
- [Constructions implicites refusées : champs, bases et compatibilité (23 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#constructions-implicites-refusées--23-octobre-2026)
- [Matrice de conformité 1.0](CONFORMITE_GS_PLUS_PLUS_1.0.md)
- [Frontend auto-hébergé 0.27](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md)
- [Diagnostics frontend facultatifs : compilations directes, projets et solutions (21 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#raccord-explicite-des-diagnostics-au-pilote--21-octobre-2026)
- [Doublons de signatures ordinaires : diagnostic 118 possédé (21 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#doublons-de-signatures-ordinaires--21-octobre-2026)
- [Constructeurs, destructeurs et opérateurs : diagnostic 118 possédé (22 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#fonctions-spéciales-dans-le-diagnostic-118--22-octobre-2026)
- [Préfixes de namespaces des surcharges : entrée native facultative et compatibilité (22 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#préfixes-de-surcharges-possédés--22-octobre-2026)
- [Collisions de liaison : empreinte native possédée et compatibilité (22 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#collisions-de-liaison-possédées--22-octobre-2026)
- [Appels ambigus : groupe canonique natif et compatibilité (22 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#appels-ambigus-possédés--22-octobre-2026)
- [Groupes importés ambigus : noms publics, contexte natif et compatibilité (23 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#groupes-importés-possédés--23-octobre-2026)
- [Aucune surcharge compatible : appels directs, méthodes et opérateurs (23 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#échecs-dappels-possédés--23-octobre-2026)
- [Constructeurs sans surcharge compatible : variables et initialiseurs (23 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#constructions-refusées-possédées--23-octobre-2026)
- [Types introuvables possédés : bases de classes, déclarations, callbacks et conversions (20 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#types-introuvables-possédés--20-octobre-2026)
- [Bases trouvées non classes : diagnostic 114 possédé (21 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#bases-trouvées-non-classes--21-octobre-2026)
- [Noms non types déjà canoniques : messages 100/99 possédés (21 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#noms-non-types-déjà-canoniques--21-octobre-2026)
- [Noms qualifiés implicitement possédés : extension native facultative (21 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#noms-qualifiés-implicitement-possédés--21-octobre-2026)
- [Noms complets de méthodes et groupes mixtes dans les diagnostics de types (21 octobre)](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#noms-complets-de-méthodes--21-octobre-2026)
- [Diagnostics sémantiques possédés : répétitions, conflits, cycles/cibles d'alias de champs et globaux, préfixes de namespaces, surcharges d'alias, espaces introuvables et noms importés ambigus dans les alias, expressions, bases de classes, types déclarés directs, callbacks imbriqués et conversions, énumérations/héritage, contextes, noms, accès et initialiseurs de champs, bilan actualisé le 15 octobre](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#diagnostics-sémantiques-possédés--7-octobre-2026)
- [Sémantique assemblée possédée](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#sémantique-assemblée-possédée--7-octobre-2026)
- [Diagnostics d'assemblage possédés](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#diagnostics-dassemblage-possédés--7-octobre-2026)
- [Assemblage multi-unités possédé](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#assemblage-multi-unités-possédé--7-octobre-2026)
- [Diagnostics syntaxiques possédés](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#diagnostics-syntaxiques-possédés--7-octobre-2026)
- [Adaptateur hôte d'AST possédé](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#adaptateur-hôte-dast-possédé--7-octobre-2026)
- [Priorités des bases, champs et initialiseurs](FRONTEND_AUTOHEBERGE_GS_PLUS_PLUS_0.27.md#priorités-des-bases-champs-et-initialiseurs--tranche-locale-du-5-octobre-2026)
- [Protocole de benchmark](PROTOCOLE-BENCHMARK-GS-PLUS-PLUS-0.25.md)
- [Dernière matrice : 0.27.0-alpha.10](Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.10.md)
- [Matrice historique de publication alpha.9](Validations/VALIDATION-GS-PLUS-PLUS-0.27.0-alpha.9.md)

Le [`CHANGELOG.md`](../CHANGELOG.md) conserve la chronologie synthétique des
versions et du développement postérieur à la dernière préversion publique.
