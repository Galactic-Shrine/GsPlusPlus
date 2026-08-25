# Préparation à l’auto-hébergement de Gs++ 0.17

## Résultat du jalon

Gs++ 0.17 exécute le premier composant du compilateur écrit en Gs++ :
`GsPlusPlus/AutoHebergement/ClassificateurMotsCles/ClassificateurMotsCles.GsPP`. Ce composant reçoit une vue
UTF-8 et retourne le même `GenreJeton` que le classificateur C++ utilisé par le
lexeur principal.

Le test automatique compare 63 entrées :

- tous les mots-clés français ;
- tous leurs alias anglais ;
- les formes accentuées et non accentuées ;
- trois identifiants qui ne doivent pas être reconnus comme mots-clés.

Le composant Gs++ est compilé dans un `.GsObj`, lié avec
`GsHebergee.GsA`, chargé comme une image GsE puis appelé selon l’ABI Microsoft
x64. Il ne s’agit donc pas d’une simulation ou d’une réécriture exécutée en
C++.

## Éléments de langage ajoutés

### Littéraux chaîne

Les chaînes sont des séquences UTF-8 entre guillemets :

```gspp
publique constante caractère* NomService()
{
    retourner "InitialiserMémoire";
}
```

Les échappements `\\`, `\"`, `\n`, `\r`, `\t` et `\0` sont reconnus. Chaque
littéral est terminé par un octet nul dans la section de données, porte le type
`constante caractère*` et reçoit un symbole local. Les occurrences identiques
d’une unité partagent le même stockage.

Une chaîne non terminée, un saut de ligne brut ou un échappement inconnu
produit un diagnostic avec fichier, ligne et colonne. L’écriture directe dans
un littéral est refusée par l’analyse sémantique.

### Logique à court-circuit

`&&` possède une priorité supérieure à `||`, tous deux se placent sous les
opérateurs binaires entiers et au-dessus de l’affectation. Les opérandes
doivent être scalaires et le résultat est un `booléen`.

Le backend n’évalue la partie droite que lorsqu’elle est nécessaire. Le même
comportement est appliqué à l’évaluation des constantes globales.

## Bibliothèque hébergée

`GsPlusPlus/Bibliotheques/Hebergee/GsHebergee.GsA` complète la bibliothèque freestanding
`GsSysteme.GsA`. Elle garde un modèle explicite : aucun conteneur n’alloue son
propre stockage.

L’API française vit dans `GalacticShrine::GsPP::Hebergee` et les alias anglais dans
`GalacticShrine::GsPP::Hosted`. Elle fournit :

- `VueTexte` / `TextView` et le hachage FNV-1a déterministe ;
- `FluxLecture` / `InputStream` et `FluxEcriture` / `OutputStream` ;
- `VecteurNaturels` / `ValueVector` à capacité fixe ;
- `TableSymboles` / `SymbolTable` par adressage ouvert ;
- `ChargerFichier` / `LoadFile` et `SauverFichier` / `SaveFile` ;
- `Diagnostic` et `SignalerDiagnostic` / `ReportDiagnostic`.

La table conserve les clés sous forme de vues : l’appelant reste responsable
de la durée de vie des caractères. Une insertion d’une clé existante remplace
sa valeur sans modifier l’ordre de sondage.

## Contrat avec l’hôte

Les modules de fichiers et de diagnostics utilisent seulement trois imports :

```text
GalacticShrine::GsPP::Hote::LireFichier(RequeteFichier*)
GalacticShrine::GsPP::Hote::EcrireFichier(RequeteFichier*)
GalacticShrine::GsPP::Hote::EmettreDiagnostic(Diagnostic par valeur)
```

`RequeteFichier` contient le chemin, le tampon, sa taille utilisée et sa
capacité. `Diagnostic` contient le niveau, la position source et deux vues
texte. Le passage d’une structure par valeur suit `GsAbi:x64-ms-v1` : au
niveau machine, l’hôte reçoit l’adresse de la copie fournie par l’appelant.

## Construction et validation

```bash
make bibliotheque-hebergee
make autohebergement
make test
```

Les sorties principales sont :

```text
Construction/Artefacts/GsPlusPlus/Bibliotheques/Hebergee/GsHebergee.GsA
Construction/Artefacts/GsPlusPlus/AutoHebergement/ClassificateurMotsCles.GsE
Construction/hebergee/TestHebergee.GsE
```

Le test hébergé résout les trois imports au moyen de trampolines proches de
l’image, afin de respecter la portée `REL32`, puis vérifie les octets lus et
écrits, les conteneurs et le diagnostic reçu.

## Limite volontaire

Le compilateur complet reste écrit en C++. La 0.17 migre uniquement le
classificateur de mots-clés et établit l’infrastructure nécessaire aux
prochaines migrations. Le découpage complet des jetons, l’analyse syntaxique,
la gestion dynamique des arbres et le pilote de compilation ne sont pas encore
auto-hébergés.
