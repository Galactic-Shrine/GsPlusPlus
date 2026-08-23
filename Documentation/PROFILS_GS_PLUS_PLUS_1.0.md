# Profils Gs++ 1.0 : freestanding et hébergé

**NORMATIF — cible produit Gs++ 1.0.**

Gs++ utilise le même langage, les mêmes formats 1.0 et l’ABI 1 dans deux
environnements. Un composant doit déclarer et documenter son profil ; il ne
peut pas acquérir silencieusement une dépendance du profil hébergé.

## Profil freestanding

Le profil freestanding vise le noyau, le chargeur, les pilotes et les
bibliothèques de bas niveau.

### Garanties

- aucun runtime obligatoire ;
- aucune allocation dynamique cachée ;
- aucune exception ou table de déroulement implicite ;
- aucun constructeur global caché ;
- aucun import hébergé ajouté automatiquement ;
- aucune dépendance à un système de fichiers, une console ou un processus ;
- RAII généré uniquement à partir des objets explicites du programme ;
- code et données sérialisés directement dans GsObj puis GsE.

### Bibliothèque de référence

`GsSysteme.GsA` fournit les primitives compatibles avec ce profil : mémoire,
vues, bits et atomiques. Ses fonctions doivent rester auditables et ne pas
appeler `GsHebergee.GsA`.

### Environnements de validation

- exécution hébergée contrôlée de `Noyau.GsE` ;
- chargement par `BOOTX64.EFI` ;
- démarrage réel virtualisé sous QEMU/OVMF.

## Profil hébergé

Le profil hébergé vise le compilateur, les outils et les applications disposant
de services d’hôte explicites.

### Capacités validées en 0.26

- chaînes propriétaires et vues UTF-8 strictes ;
- conteneurs à stockage dynamique explicite ;
- arène stable et table de symboles dynamique ;
- fichiers alloués et chemins ;
- modèle d’erreur explicite sans exception ;
- cinq imports d’hôte déclarés et résolus par le chargeur de test approprié.

La migration des structures AST et des autres composants du compilateur vers
ces primitives reste un travail d’auto-hébergement, pas une capacité cachée de
la bibliothèque.

### Bibliothèque de référence

`GsHebergee.GsA` est la bibliothèque de base. Son contrat de propriété mémoire
0.26 est `VALIDÉ` : chaînes, conteneurs, arène, table de symboles, chemins et
fichiers sont couverts par les tests hébergés et la conformité. Cela ne rend
pas encore le compilateur auto-hébergé : le frontend, le backend et l’éditeur
de liens restent majoritairement écrits en C++ et doivent migrer avant Gs++
1.0.

## Règles communes

- signatures `GSOBJ:0`, `GSA:0`, `GSE:0` ;
- formats GsObj/GsA/GsE 1.0 ;
- ABI 1 et préfixe `GsAbi:x64-ms-v1` ;
- mêmes règles de types, dispositions et appels ;
- diagnostics bilingues cohérents ;
- compilation séparée et contrôle inter-unités ;
- aucune dépendance implicite choisie en fonction du système hôte.

## Frontière entre profils

Un composant freestanding peut être consommé par un composant hébergé. Le sens
inverse est interdit sauf adaptation explicite remplaçant toutes les
dépendances hébergées. Le graphe attendu est :

```text
GsSysteme.GsA
      ↓
GsHebergee.GsA
      ↓
Compilateur et outils
```

Le noyau et le chargeur ne dépendent jamais de `GsHebergee.GsA`.

## Erreurs

L’absence d’exceptions du langage ne dispense pas d’un contrat d’erreur. Les
bibliothèques doivent utiliser des résultats, codes, états ou diagnostics
explicites dont la propriété et la durée de vie sont documentées. Une erreur
d’allocation ne peut pas être transformée en comportement indéfini silencieux.

## Construction et projets

Un projet peut sélectionner son ensemble de bibliothèques et ses imports. La
convergence 1.0 doit définir une propriété de profil explicite dans le contrat
de projet ou démontrer qu’un choix entièrement déterminé par les dépendances
est suffisant. Tant que cette décision n’est pas implémentée, le profil est
établi par les bibliothèques liées et contrôlé par les tests.

## Critères de conformité

### Freestanding

- construction sans bibliothèque hébergée ;
- GsE sans import externe pour le noyau de référence ;
- absence d’initialisation globale cachée ;
- validation W^X ;
- démarrage UEFI réussi.

### Hébergé

- imports explicitement listés ;
- erreurs de résolution diagnostiquées ;
- conteneurs et fichiers couverts par des tests ;
- exécution du composant auto-hébergé ;
- absence de dépendance accidentelle à une API non déclarée.

## Évolution

Les profils n’introduisent pas une seconde ABI. Une capacité d’hôte future est
une bibliothèque ou un import explicite, pas une modification silencieuse de
la convention d’appel ou des formats binaires.
