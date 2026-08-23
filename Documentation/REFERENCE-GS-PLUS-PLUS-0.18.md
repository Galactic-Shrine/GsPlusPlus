# Compilateur Gs++ 0.18.0 — référence historique

> La référence courante est
> [`REFERENCE-GS-PLUS-PLUS-0.22.md`](REFERENCE-GS-PLUS-PLUS-0.22.md). Ce
> document conserve le contrat exact du jalon 0.18 avant l’ajout de l’héritage.

## État vérifié

Gs++ 0.18.0 est le langage système natif de Sanctuaire SE. Le compilateur reste
principalement écrit en C++, tandis que le classificateur de mots-clés Gs++ et
les bibliothèques système/hébergée constituent la fondation mesurable de
l’auto-hébergement.

La 0.18.0 ajoute le modèle objet système sans changer les formats binaires
canoniques établis en 0.17.1 :

- magic GsObj : sept octets `GSOBJ:0`, puis un octet réservé nul ;
- GsObj : format 1.0, ABI 1 ;
- GsA : format 1.0 ;
- GsE : format 1.0, ABI 1 ;
- signatures : préfixe `GsAbi:x64-ms-v1` ;
- architecture : x86-64, convention Microsoft x64 ;
- texte source et noms publics : UTF-8.

Les artefacts des anciennes numérotations locales ne sont pas convertis. Le
projet étant local, ils doivent être reconstruits.

## Fonctionnalités du langage

| Domaine | État en 0.18.0 |
|---|---|
| Lexeur UTF-8 et mots-clés FR/EN | validé |
| Entiers 8/16/32/64, booléen, octet, caractère | validé |
| `constante/const`, `volatile`, tableaux, unions, énumérations | validé |
| Structures, pointeurs et valeurs structurées | validé |
| Globales, imports, exports, espaces et alias | validé |
| Pointeurs de fonction et appels indirects | validé |
| Compilation séparée GsObj, archives GsA et liaison | validé |
| Bibliothèques système et hébergée | validé |
| Classes et visibilité | validé en 0.18.0 |
| Constructeurs/destructeurs locaux | validé en 0.18.0 |
| Références locales et de paramètres | validé en 0.18.0 |
| Surcharge de fonctions/constructeurs/opérateurs | validé en 0.18.0 |
| RAII sans runtime | validé en 0.18.0 |
| Méthodes virtuelles optionnelles | validé en 0.18.0 |
| Héritage et remplacement de méthodes | prévu, hors 0.18.0 |
| Exceptions et déroulement | non prévu pour le cœur freestanding actuel |

La spécification détaillée du nouveau périmètre est
[`MODELE_OBJET_GS_PLUS_PLUS_0.18.md`](MODELE_OBJET_GS_PLUS_PLUS_0.18.md).
La référence 0.17 reste le document détaillé des fonctionnalités héritées et
des contrats de Sanctuaire SE :
[`REFERENCE-GS-PLUS-PLUS-0.17.md`](REFERENCE-GS-PLUS-PLUS-0.17.md).

## Exemple objet minimal

```gspp
classe Compteur
{
    privée:
        entier32 Valeur;

    publique:
        constructeur(entier32 valeur)
        {
            soi.Valeur = valeur;
        }

        destructeur()
        {
            soi.Valeur = 0;
        }

        virtuel entier32 Lire()
        {
            retourner soi.Valeur;
        }
};

publique entier32 Principal()
{
    entier32 valeur = 42;
    entier32& référence = valeur;
    Compteur compteur(référence);
    retourner compteur.Lire();
}
```

L’équivalent anglais emploie `class`, `private`, `public`, `constructor`,
`destructor`, `virtual`, `this` et `return`. Les deux variantes produisent le
même code machine.

## ABI objet 0.18

Une méthode est une fonction ABI ordinaire avec un premier paramètre caché
`Classe& soi`. Une référence est physiquement une adresse de huit octets, mais
reste un type distinct dans la signature. Les fonctions non surchargées gardent
leur nom historique. Seuls les groupes surchargés reçoivent un suffixe manglé
déterministe.

Une classe non polymorphe conserve une disposition agrégée naturelle. Une
classe polymorphe possède :

1. un pointeur de table virtuelle au décalage 0 ;
2. les champs à partir du décalage 8, avec leur alignement naturel ;
3. un alignement global d’au moins 8 ;
4. une table locale `.data` contenant des adresses 64 bits relocalisées.

La signature ABI encode la nature classe/structure/union, le caractère
polymorphe, la taille, l’alignement et les champs. L’éditeur de liens peut donc
refuser deux unités qui ne partagent pas la même disposition tout en conservant
le numéro d’ABI canonique 1.

## Chaîne de compilation

Les outils principaux restent :

- `gsppc` / `gspluspluscompiler` : compilation, archivage et liaison ;
- `gseverifier` : validation structurelle et W^X d’une image GsE ;
- `gsechargeur` / `gseload` : chargement et exécution hébergée ;
- les constructeurs de projets/solutions `.GsProject` et `.GsPs`.

Exemple :

```text
gsppc ModeleObjet.GsPP --format gse --point-entree Principal \
  --nom "Modèle objet Gs++ 0.18" --version-application 0.18.0 \
  -o ModeleObjet.GsE
gseverifier ModeleObjet.GsE
gsechargeur ModeleObjet.GsE --executer
```

Le scénario de référence retourne `25`.

## Construction et tests

Sous Windows/MSVC :

```powershell
cmake -S . -B Construction/Validation-GSPP-0180
cmake --build Construction/Validation-GSPP-0180 --config Release --parallel
ctest --test-dir Construction/Validation-GSPP-0180 -C Release --output-on-failure
```

Sous GNU/WSL :

```bash
cmake -S . -B Construction/Validation-GSPP-0180-GNU \
  -DCMAKE_BUILD_TYPE=Release
cmake --build Construction/Validation-GSPP-0180-GNU --parallel
ctest --test-dir Construction/Validation-GSPP-0180-GNU \
  --output-on-failure
```

Les validations requises avant publication d’un nouveau résultat sont :

- tests unitaires du compilateur ;
- intégration GsE avec exécution du modèle objet ;
- reconstruction des bibliothèques `GsSysteme.GsA` et `GsHebergee.GsA` ;
- auto-hébergement du classificateur et concordance sur 79 entrées ;
- reconstruction de Sanctuaire SE 0.10.2 ;
- contrôle des en-têtes `GSOBJ:0`, GsA/GsE 1.0 et ABI 1 ;
- démarrage QEMU/OVMF lorsque la validation UEFI complète est demandée.

Le rapport daté du jalon est
[`Validations/VALIDATION-GS-PLUS-PLUS-0.18.0.md`](Validations/VALIDATION-GS-PLUS-PLUS-0.18.0.md).
Il consigne 4/4 tests MSVC, 5/5 tests GNU/WSL, les retours exécutés `25` et
`42`, ainsi que le démarrage réel QEMU/OVMF réussi.

## Limites explicites de 0.18.0

- aucun héritage ni remplacement de méthode de base ;
- aucun champ, globale, tableau ou retour par référence ;
- aucun constructeur/destructeur global implicite ;
- aucune synthèse récursive des destructeurs de champs objets ;
- aucune exception ni table de déroulement ;
- quatre paramètres ABI au maximum, `soi` compris ; trois avec retour de
  structure ;
- l’adresse d’un groupe surchargé exige un contexte non ambigu, qui n’est pas
  encore inféré depuis un type de pointeur de fonction destination.

Ces limites sont des diagnostics ou des frontières documentées. Elles ne sont
pas présentées comme des fonctionnalités complètes.
