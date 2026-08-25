#!/usr/bin/env bash
set -euo pipefail

racine_source="${1:?racine source manquante}"
racine_construction="${2:?racine de construction manquante}"
compilateur="${3:?compilateur manquant}"
verificateur="${4:?vérificateur GsE manquant}"
chargeur="${5:?chargeur GsE manquant}"
bibliotheque_systeme="${6:?bibliothèque système manquante}"
bibliotheque_hebergee="${7:?bibliothèque hébergée manquante}"
classificateur="${8:?classificateur manquant}"
test_hebergee="${9:?test hébergé manquant}"

repertoire_test="$racine_construction/Tests/Integration"
repertoire_systeme="$racine_construction/Tests/Systeme"
repertoire_hebergee="$racine_construction/Tests/Hebergee"
repertoire_separation="$racine_construction/Tests/Separation"
mkdir -p "$repertoire_test" "$repertoire_systeme" \
    "$repertoire_hebergee" "$repertoire_separation"

sortie_fr="$repertoire_test/Bonjour.obj"
sortie_en="$repertoire_test/Hello.obj"
sortie_alias="$repertoire_test/Alias.obj"
sortie_gse="$repertoire_test/Application.GsE"
sortie_types="$repertoire_test/TypesSysteme.GsE"
sortie_pointeurs_fonction="$repertoire_test/PointeursFonction.GsE"
sortie_valeurs_structures="$repertoire_test/ValeursStructures.GsE"
sortie_modele_objet="$repertoire_test/ModeleObjet.GsE"
sortie_heritage="$repertoire_test/Heritage.GsE"
sortie_initialisation_parent="$repertoire_test/InitialisationParent.GsE"
sortie_initialiseurs_champs="$repertoire_test/InitialiseursChamps.GsE"
sortie_champs_objets_classes="$repertoire_test/ChampsObjetsClasses.GsE"
sortie_tableaux_objets_classes="$repertoire_test/TableauxObjetsClasses.GsE"
sortie_initialisation_duree_vie="$repertoire_test/InitialisationDureeVie.GsE"
sortie_bibliotheque_systeme="$repertoire_test/BibliothequeSysteme.GsE"

cd "$racine_source"

"$compilateur" Exemples/Bonjour.Gs++ -o "$sortie_fr"
"$compilateur" Exemples/Hello.GsPlusPlus -o "$sortie_en"

cmp "$sortie_fr" "$sortie_en"
objdump -f "$sortie_fr" | grep -q "file format pe-x86-64"
objdump -r "$sortie_fr" | grep -q "IMAGE_REL_AMD64_REL32"
objdump -t "$sortie_fr" | grep -q "Shrine::Exemples::Additionner"
objdump -t "$sortie_fr" | grep -q "Shrine::Exemples::Principal"

"$compilateur" Exemples/Alias.GsPP -o "$sortie_alias"
adresse_canonique="$(objdump -t "$sortie_alias" \
    | awk '$NF == "Shrine::Exemples::CalculerAvecAlias" { print $(NF - 1) }')"
adresse_alias="$(objdump -t "$sortie_alias" \
    | awk '$NF == "Shrine::Exemples::CalculateWithAlias" { print $(NF - 1) }')"
test -n "$adresse_canonique"
test "$adresse_canonique" = "$adresse_alias"
objdump -r "$sortie_alias" | grep -q "Shrine::Exemples::CalculerAvecAlias"
if objdump -r "$sortie_alias" | grep -q "Shrine::Exemples::CalculateWithAlias"
then
    echo "Une relocalisation utilise l’alias au lieu du symbole canonique." >&2
    exit 1
fi

"$compilateur" \
    Exemples/Point.GsPP \
    Exemples/Globales.GsPP \
    Exemples/Application.GsPP \
    --format gse --point-entree Shrine::Exemples::Principal \
    --nom "Application Exemple" --version-application 0.10.2 \
    -o "$sortie_gse"

test "$(od -An -tx1 -N5 "$sortie_gse" | tr -d ' \n')" = "4753453a30"
test "$(od -An -tx1 -j5 -N3 "$sortie_gse" | tr -d ' \n')" = "000000"
test "$(od -An -tu2 -j8 -N2 "$sortie_gse" | tr -d ' \n')" = "1"
test "$(od -An -tu2 -j10 -N2 "$sortie_gse" | tr -d ' \n')" = "0"
test "$(od -An -tu2 -j12 -N2 "$sortie_gse" | tr -d ' \n')" = "112"
test "$(od -An -tu2 -j20 -N2 "$sortie_gse" | tr -d ' \n')" = "1"
"$verificateur" "$sortie_gse" | grep -q "GsE valide"
grep -aq "Application Exemple" "$sortie_gse"
grep -aq "Shrine::Exemples::Journaliser" "$sortie_gse"
"$chargeur" "$sortie_gse" --base 0x10000000 \
    --resoudre Shrine::Exemples::Journaliser=0x10002000 \
    | grep -q "Imports résolus : 1/1"

"$compilateur" Tests/Integration/TypesSysteme.GsPP --format gse \
    --point-entree Principal --nom "Types système Gs++ 0.17" \
    --version-application 0.27.0-alpha.1 -o "$sortie_types"
"$verificateur" "$sortie_types" | grep -q "GsE valide"
grep -aq "AjouterNaturel64" "$sortie_types"
"$chargeur" "$sortie_types" --executer | grep -q "Code de retour : 120"

"$compilateur" Tests/Integration/PointeursFonction.GsPP --format gse \
    --point-entree Essai::Fonctions::Principal \
    --nom "Pointeurs de fonction Gs++ 0.17" \
    --version-application 0.27.0-alpha.1 -o "$sortie_pointeurs_fonction"
"$verificateur" "$sortie_pointeurs_fonction" | grep -q "GsE valide"
grep -aq "Essai::Fonctions::OperationGlobale" "$sortie_pointeurs_fonction"
"$chargeur" "$sortie_pointeurs_fonction" --executer \
    | grep -q "Code de retour : 44"

"$compilateur" Tests/Integration/ValeursStructures.GsPP --format gse \
    --point-entree Essai::Valeurs::Principal \
    --nom "Valeurs structurées Gs++ 0.17" \
    --version-application 0.27.0-alpha.1 -o "$sortie_valeurs_structures"
"$verificateur" "$sortie_valeurs_structures" | grep -q "GsE valide"
grep -aq "Essai::Valeurs::ModifierGrand" "$sortie_valeurs_structures"
"$chargeur" "$sortie_valeurs_structures" --executer \
    | grep -q "Code de retour : 45"

"$compilateur" Tests/Integration/ModeleObjet.GsPP --format gse \
    --point-entree Principal --nom "Modèle objet Gs++ 0.18" \
    --version-application 0.27.0-alpha.1 -o "$sortie_modele_objet"
"$verificateur" "$sortie_modele_objet" | grep -q "GsE valide"
"$chargeur" "$sortie_modele_objet" --executer \
    | grep -q "Code de retour : 25"

"$compilateur" Tests/Integration/Heritage.GsPP --format gse \
    --point-entree Principal --nom "Héritage Gs++ 0.22" \
    --version-application 0.27.0-alpha.1 -o "$sortie_heritage"
"$verificateur" "$sortie_heritage" | grep -q "GsE valide"
"$chargeur" "$sortie_heritage" --executer \
    | grep -q "Code de retour : 88"

"$compilateur" \
    Tests/Integration/InitialisationParent.GsPP --format gse \
    --point-entree Principal --nom "Initialisation parent Gs++ 0.22" \
    --version-application 0.27.0-alpha.1 -o "$sortie_initialisation_parent"
"$verificateur" "$sortie_initialisation_parent" | grep -q "GsE valide"
"$chargeur" "$sortie_initialisation_parent" --executer \
    | grep -q "Code de retour : 82"

"$compilateur" \
    Tests/Integration/InitialiseursChamps.GsPP --format gse \
    --point-entree Principal --nom "Initialiseurs de champs Gs++ 0.22" \
    --version-application 0.27.0-alpha.1 -o "$sortie_initialiseurs_champs"
"$verificateur" "$sortie_initialiseurs_champs" | grep -q "GsE valide"
"$chargeur" "$sortie_initialiseurs_champs" --executer \
    | grep -q "Code de retour : 75"

"$compilateur" \
    Tests/Integration/ChampsObjetsClasses.GsPP --format gse \
    --point-entree Principal --nom "Champs objets classes Gs++ 0.22" \
    --version-application 0.27.0-alpha.1 -o "$sortie_champs_objets_classes"
"$verificateur" "$sortie_champs_objets_classes" | grep -q "GsE valide"
"$chargeur" "$sortie_champs_objets_classes" --executer \
    | grep -q "Code de retour : 91"

"$compilateur" \
    Tests/Integration/TableauxObjetsClasses.GsPP --format gse \
    --point-entree Principal --nom "Tableaux objets classes Gs++ 0.23" \
    --version-application 0.27.0-alpha.1 -o "$sortie_tableaux_objets_classes"
"$verificateur" "$sortie_tableaux_objets_classes" | grep -q "GsE valide"
"$chargeur" "$sortie_tableaux_objets_classes" --executer \
    | grep -q "Code de retour : 10"

"$compilateur" \
    Tests/Integration/InitialisationDureeVie.GsPP --format gse \
    --point-entree Principal --nom "Initialisation et durée de vie Gs++ 0.25" \
    --version-application 0.27.0-alpha.1 -o "$sortie_initialisation_duree_vie"
"$verificateur" "$sortie_initialisation_duree_vie" | grep -q "GsE valide"
"$chargeur" "$sortie_initialisation_duree_vie" --executer \
    | grep -q "Code de retour : 25"

test "$(od -An -tx1 -N5 "$bibliotheque_systeme" | tr -d ' \n')" = "4753413a30"
test "$(od -An -tu2 -j8 -N2 "$bibliotheque_systeme" | tr -d ' \n')" = "1"
test "$(od -An -tu2 -j20 -N2 "$bibliotheque_systeme" | tr -d ' \n')" = "1"
"$compilateur" Bibliotheques/Systeme/Systeme.HGsPP \
    Tests/Integration/BibliothequeSysteme.GsPP --format gsobj \
    -o "$repertoire_systeme/TestSysteme.GsObj"
"$compilateur" "$repertoire_systeme/TestSysteme.GsObj" \
    "$bibliotheque_systeme" --format gse --point-entree Principal \
    --nom "Bibliothèque système Gs++ 0.23" --version-application 0.27.0-alpha.1 \
    -o "$sortie_bibliotheque_systeme"
"$verificateur" "$sortie_bibliotheque_systeme" | grep -q "GsE valide"
"$chargeur" "$sortie_bibliotheque_systeme" --executer \
    | grep -q "Code de retour : 64"
grep -aq "GalacticShrine::GsPP::Systeme::CopierMemoire" "$sortie_bibliotheque_systeme"
grep -aq "GalacticShrine::GsPP::System::AtomicFetchAdd64" "$sortie_bibliotheque_systeme"
if grep -aq "GalacticShrine::GsPP::Intrinseques::" "$sortie_bibliotheque_systeme"
then
    echo "Une intrinsèque intégrée subsiste comme import dans le GsE." >&2
    exit 1
fi
cp "$bibliotheque_systeme" "$repertoire_systeme/GsSysteme-premiere.GsA"
"$compilateur" Bibliotheques/Systeme/GsSysteme.GsPj \
    -o "$repertoire_systeme/GsSysteme-reconstruite.GsA" \
    --repertoire-objets "$repertoire_systeme/Objets-Projet" >/dev/null
cmp "$repertoire_systeme/GsSysteme-premiere.GsA" \
    "$repertoire_systeme/GsSysteme-reconstruite.GsA"

test "$(od -An -tx1 -N5 "$bibliotheque_hebergee" | tr -d ' \n')" = "4753413a30"
"$verificateur" "$classificateur" | grep -q "GsE valide"
"$verificateur" "$test_hebergee" | grep -q "GsE valide"
grep -aq "GalacticShrine::GsPP::Autohebergement::ClassifierMotCle" "$classificateur"
grep -aq "GalacticShrine::GsPP::Hote::EmettreDiagnostic" "$test_hebergee"
cp "$bibliotheque_hebergee" "$repertoire_hebergee/GsHebergee-premiere.GsA"
"$compilateur" Bibliotheques/Hebergee/GsHebergee.GsPj \
    -o "$repertoire_hebergee/GsHebergee-reconstruite.GsA" \
    --repertoire-objets "$repertoire_hebergee/Objets-Projet" >/dev/null
cmp "$repertoire_hebergee/GsHebergee-premiere.GsA" \
    "$repertoire_hebergee/GsHebergee-reconstruite.GsA"

if "$compilateur" Tests/Integration/ErreurType.GsPP \
    -o "$repertoire_test/ErreurType.obj" 2>"$repertoire_test/ErreurType.txt"
then
    echo "Une erreur de typage aurait dû être détectée." >&2
    exit 1
fi
grep -q "Tests/Integration/ErreurType.GsPP:5:" \
    "$repertoire_test/ErreurType.txt"
grep -q "incompatible" "$repertoire_test/ErreurType.txt"

if "$compilateur" \
    Tests/Integration/Separation/Ancienne.GsPPH \
    -o "$repertoire_test/Ancienne.obj" 2>"$repertoire_test/Ancienne.txt"
then
    echo "Une extension d’interface obsolète aurait dû être refusée." >&2
    exit 1
fi
grep -q "extension obsolète refusée" "$repertoire_test/Ancienne.txt"

if "$compilateur" Exemples/Bonjour.Gs++ --format gsobj \
    -o "$repertoire_test/Ancien.GsO" 2>"$repertoire_test/AncienObjet.txt"
then
    echo "Une extension de sortie GsO obsolète aurait dû être refusée." >&2
    exit 1
fi
grep -q "extension de sortie obsolète refusée" \
    "$repertoire_test/AncienObjet.txt"

if "$compilateur" \
    'Tests/Integration/Separation/Reservee.Gs#' \
    -o "$repertoire_test/Reservee.obj" 2>"$repertoire_test/Reservee.txt"
then
    echo "Une extension Gs# aurait dû être routée hors de gsppc." >&2
    exit 1
fi
grep -q "extension réservée à Gs#" "$repertoire_test/Reservee.txt"

"$compilateur" \
    Tests/Integration/Separation/Calculs.HGsPP \
    Tests/Integration/Separation/Calculs.GsPP --format gsobj \
    -o "$repertoire_separation/Calculs.GsObj"
"$compilateur" \
    Tests/Integration/Separation/Calculs.HeaderGsPlusPlus \
    Tests/Integration/Separation/Principal.GsPP --format gsobj \
    -o "$repertoire_separation/Principal.GsObj"
"$compilateur" Tests/Integration/Separation/Inutilise.GsPP \
    --format gsobj -o "$repertoire_separation/Inutilise.GsObj"

test "$(od -An -tx1 -N7 "$repertoire_separation/Calculs.GsObj" | tr -d ' \n')" = "47534f424a3a30"
test "$(od -An -tx1 -j7 -N1 "$repertoire_separation/Calculs.GsObj" | tr -d ' \n')" = "00"
test "$(od -An -tu2 -j8 -N2 "$repertoire_separation/Calculs.GsObj" | tr -d ' \n')" = "1"
test "$(od -An -tu2 -j14 -N2 "$repertoire_separation/Calculs.GsObj" | tr -d ' \n')" = "1"

"$compilateur" "$repertoire_separation/Calculs.GsObj" \
    "$repertoire_separation/Inutilise.GsObj" --format gsa \
    -o "$repertoire_separation/Calculs.GsA"
test "$(od -An -tx1 -N5 "$repertoire_separation/Calculs.GsA" | tr -d ' \n')" = "4753413a30"
test "$(od -An -tu2 -j8 -N2 "$repertoire_separation/Calculs.GsA" | tr -d ' \n')" = "1"
test "$(od -An -tu2 -j20 -N2 "$repertoire_separation/Calculs.GsA" | tr -d ' \n')" = "1"

"$compilateur" "$repertoire_separation/Principal.GsObj" \
    "$repertoire_separation/Calculs.GsA" --format gse \
    --point-entree Essai::Separation::Principal \
    --carte "$repertoire_separation/Application.map" \
    --nom "Compilation séparée Gs++" --version-application 0.27.0-alpha.1 \
    -o "$repertoire_separation/Application.GsE"
"$verificateur" "$repertoire_separation/Application.GsE" | grep -q "GsE valide"
"$chargeur" "$repertoire_separation/Application.GsE" --executer \
    | grep -q "Code de retour : 44"
if strings "$repertoire_separation/Application.GsE" \
    | grep -q "ServiceQuiNeDoitPasEtreImporte"
then
    echo "Un membre inutilisé de la bibliothèque GsA a été extrait." >&2
    exit 1
fi
grep -q "Tests/Integration/Separation/Principal.GsPP" \
    "$repertoire_separation/Application.map"
grep -q "GsAbi:x64-ms-v1:F" "$repertoire_separation/Application.map"

"$compilateur" Tests/Integration/Separation/Valeurs.HGsPP \
    Tests/Integration/Separation/Valeurs.GsPP --format gsobj \
    -o "$repertoire_separation/Valeurs.GsObj"
"$compilateur" Tests/Integration/Separation/Valeurs.HGsPP \
    Tests/Integration/Separation/ValeursPrincipal.GsPP --format gsobj \
    -o "$repertoire_separation/ValeursPrincipal.GsObj"
"$compilateur" "$repertoire_separation/Valeurs.GsObj" \
    "$repertoire_separation/ValeursPrincipal.GsObj" --format gse \
    --point-entree Essai::ValeursSeparation::PrincipalValeurs \
    --nom "Valeurs structurées séparées Gs++" \
    --version-application 0.27.0-alpha.1 \
    -o "$repertoire_separation/ValeursApplication.GsE"
"$verificateur" "$repertoire_separation/ValeursApplication.GsE" \
    | grep -q "GsE valide"
"$chargeur" "$repertoire_separation/ValeursApplication.GsE" --executer \
    | grep -q "Code de retour : 46"

"$compilateur" \
    Tests/Integration/Separation/ModeleObjetImplementation.GsPP \
    --format gsobj -o "$repertoire_separation/ModeleObjetImplementation.GsObj"
"$compilateur" \
    Tests/Integration/Separation/ModeleObjet.HGs++ \
    Tests/Integration/Separation/ModeleObjetPrincipal.GsPP \
    --format gsobj -o "$repertoire_separation/ModeleObjetPrincipal.GsObj"
"$compilateur" \
    "$repertoire_separation/ModeleObjetImplementation.GsObj" \
    "$repertoire_separation/ModeleObjetPrincipal.GsObj" \
    --format gse --point-entree PrincipalObjetSepare \
    --nom "Modèle objet séparé Gs++ 0.18" \
    --version-application 0.27.0-alpha.1 \
    -o "$repertoire_separation/ModeleObjetSepare.GsE"
"$verificateur" "$repertoire_separation/ModeleObjetSepare.GsE" \
    | grep -q "GsE valide"
"$chargeur" "$repertoire_separation/ModeleObjetSepare.GsE" --executer \
    | grep -q "Code de retour : 42"

"$compilateur" \
    Tests/Integration/Separation/HeritageImplementation.GsPP \
    --format gsobj -o "$repertoire_separation/HeritageImplementation.GsObj"
"$compilateur" \
    Tests/Integration/Separation/Heritage.HGsPP \
    Tests/Integration/Separation/HeritagePrincipal.GsPP \
    --format gsobj -o "$repertoire_separation/HeritagePrincipal.GsObj"
"$compilateur" \
    "$repertoire_separation/HeritageImplementation.GsObj" \
    "$repertoire_separation/HeritagePrincipal.GsObj" \
    --format gse --point-entree PrincipalHeritageSepare \
    --nom "Héritage séparé Gs++ 0.22" \
    --version-application 0.27.0-alpha.1 \
    -o "$repertoire_separation/HeritageSepare.GsE"
"$verificateur" "$repertoire_separation/HeritageSepare.GsE" \
    | grep -q "GsE valide"
"$chargeur" "$repertoire_separation/HeritageSepare.GsE" --executer \
    | grep -q "Code de retour : 42"

"$compilateur" \
    Tests/Integration/Separation/InitialisationParentImplementation.GsPP \
    --format gsobj \
    -o "$repertoire_separation/InitialisationParentImplementation.GsObj"
"$compilateur" \
    Tests/Integration/Separation/InitialisationParent.HGsPP \
    Tests/Integration/Separation/InitialisationParentPrincipal.GsPP \
    --format gsobj \
    -o "$repertoire_separation/InitialisationParentPrincipal.GsObj"
"$compilateur" \
    "$repertoire_separation/InitialisationParentImplementation.GsObj" \
    "$repertoire_separation/InitialisationParentPrincipal.GsObj" \
    --format gse --point-entree PrincipalParentSepare \
    --nom "Initialisation parent séparée Gs++ 0.22" \
    --version-application 0.27.0-alpha.1 \
    -o "$repertoire_separation/InitialisationParentSepare.GsE"
"$verificateur" "$repertoire_separation/InitialisationParentSepare.GsE" \
    | grep -q "GsE valide"
"$chargeur" "$repertoire_separation/InitialisationParentSepare.GsE" \
    --executer | grep -q "Code de retour : 82"

"$compilateur" \
    Tests/Integration/Separation/InitialiseursChampsImplementation.GsPP \
    --format gsobj \
    -o "$repertoire_separation/InitialiseursChampsImplementation.GsObj"
"$compilateur" \
    Tests/Integration/Separation/InitialiseursChamps.HGsPP \
    Tests/Integration/Separation/InitialiseursChampsPrincipal.GsPP \
    --format gsobj \
    -o "$repertoire_separation/InitialiseursChampsPrincipal.GsObj"
"$compilateur" \
    "$repertoire_separation/InitialiseursChampsImplementation.GsObj" \
    "$repertoire_separation/InitialiseursChampsPrincipal.GsObj" \
    --format gse --point-entree PrincipalChampsSepares \
    --nom "Initialiseurs de champs séparés Gs++ 0.22" \
    --version-application 0.27.0-alpha.1 \
    -o "$repertoire_separation/InitialiseursChampsSepares.GsE"
"$verificateur" "$repertoire_separation/InitialiseursChampsSepares.GsE" \
    | grep -q "GsE valide"
"$chargeur" "$repertoire_separation/InitialiseursChampsSepares.GsE" \
    --executer | grep -q "Code de retour : 75"

"$compilateur" \
    Tests/Integration/Separation/ChampsObjetsClassesImplementation.GsPP \
    --format gsobj \
    -o "$repertoire_separation/ChampsObjetsClassesImplementation.GsObj"
"$compilateur" \
    Tests/Integration/Separation/ChampsObjetsClasses.HGsPP \
    Tests/Integration/Separation/ChampsObjetsClassesPrincipal.GsPP \
    --format gsobj \
    -o "$repertoire_separation/ChampsObjetsClassesPrincipal.GsObj"
"$compilateur" \
    "$repertoire_separation/ChampsObjetsClassesImplementation.GsObj" \
    "$repertoire_separation/ChampsObjetsClassesPrincipal.GsObj" \
    --format gse --point-entree PrincipalObjetsSepares \
    --nom "Champs objets classes séparés Gs++ 0.22" \
    --version-application 0.27.0-alpha.1 \
    -o "$repertoire_separation/ChampsObjetsClassesSepares.GsE"
"$verificateur" "$repertoire_separation/ChampsObjetsClassesSepares.GsE" \
    | grep -q "GsE valide"
"$chargeur" "$repertoire_separation/ChampsObjetsClassesSepares.GsE" \
    --executer | grep -q "Code de retour : 91"

"$compilateur" \
    Tests/Integration/Separation/TableauxObjetsClassesImplementation.GsPP \
    --format gsobj \
    -o "$repertoire_separation/TableauxObjetsClassesImplementation.GsObj"
"$compilateur" \
    Tests/Integration/Separation/TableauxObjetsClasses.HGsPP \
    Tests/Integration/Separation/TableauxObjetsClassesPrincipal.GsPP \
    --format gsobj \
    -o "$repertoire_separation/TableauxObjetsClassesPrincipal.GsObj"
"$compilateur" \
    "$repertoire_separation/TableauxObjetsClassesImplementation.GsObj" \
    "$repertoire_separation/TableauxObjetsClassesPrincipal.GsObj" \
    --format gse --point-entree PrincipalTableauSepare \
    --nom "Tableaux objets classes séparés Gs++ 0.23" \
    --version-application 0.27.0-alpha.1 \
    -o "$repertoire_separation/TableauxObjetsClassesSepares.GsE"
"$verificateur" "$repertoire_separation/TableauxObjetsClassesSepares.GsE" \
    | grep -q "GsE valide"
"$chargeur" "$repertoire_separation/TableauxObjetsClassesSepares.GsE" \
    --executer | grep -q "Code de retour : 10"

"$compilateur" \
    Tests/Integration/Separation/InitialisationDureeVieImplementation.GsPP \
    --format gsobj \
    -o "$repertoire_separation/InitialisationDureeVieImplementation.GsObj"
"$compilateur" \
    Tests/Integration/Separation/InitialisationDureeVie.HGsPP \
    Tests/Integration/Separation/InitialisationDureeViePrincipal.GsPP \
    --format gsobj \
    -o "$repertoire_separation/InitialisationDureeViePrincipal.GsObj"
"$compilateur" \
    "$repertoire_separation/InitialisationDureeVieImplementation.GsObj" \
    "$repertoire_separation/InitialisationDureeViePrincipal.GsObj" \
    --format gse --point-entree PrincipalDureeSeparee \
    --nom "Initialisation et durée de vie séparées Gs++ 0.25" \
    --version-application 0.27.0-alpha.1 \
    -o "$repertoire_separation/InitialisationDureeVieSeparee.GsE"
"$verificateur" "$repertoire_separation/InitialisationDureeVieSeparee.GsE" \
    | grep -q "GsE valide"
"$chargeur" "$repertoire_separation/InitialisationDureeVieSeparee.GsE" \
    --executer | grep -q "Code de retour : 25"

"$compilateur" \
    Tests/Integration/Separation/Calculs.HeaderGsPlusPlus \
    Tests/Integration/Separation/Principal.GsPP --format gsobj \
    -o "$repertoire_separation/Principal-reproduit.GsObj"
cmp "$repertoire_separation/Principal.GsObj" \
    "$repertoire_separation/Principal-reproduit.GsObj"

"$compilateur" \
    Tests/Integration/Separation/CalculsIncompatibles.HGsPP \
    Tests/Integration/Separation/PrincipalIncompatible.GsPP \
    --format gsobj -o "$repertoire_separation/PrincipalIncompatible.GsObj"
if "$compilateur" "$repertoire_separation/PrincipalIncompatible.GsObj" \
    "$repertoire_separation/Calculs.GsA" --format gse \
    --point-entree Essai::Separation::Principal \
    -o "$repertoire_separation/Incompatible.GsE" \
    2>"$repertoire_separation/Incompatible.txt"
then
    echo "Une incompatibilité ABI entre unités aurait dû être refusée." >&2
    exit 1
fi
grep -q "incompatibilité ABI" "$repertoire_separation/Incompatible.txt"
grep -q "CalculsIncompatibles.HGsPP:3:5" "$repertoire_separation/Incompatible.txt"
grep -q "Calculs.GsPP:5:5" "$repertoire_separation/Incompatible.txt"

projet_bibliotheque="$repertoire_separation/Bibliotheque.GsPj"
projet_application="$repertoire_separation/Application.GsProject"
solution_compilation="$repertoire_separation/Compilation.GsPs"

cat >"$projet_bibliotheque" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<GsProjet Version="1.0" Nom="CalculsSeparation" Type="bibliotheque">
    <Interface Chemin="$racine_source/Tests/Integration/Separation/Calculs.HGsPP" />
    <Source Chemin="$racine_source/Tests/Integration/Separation/Calculs.GsPP" />
    <Construction
        RepertoireObjets="$repertoire_separation/ProjetBibliotheque/Objets"
        Sortie="$repertoire_separation/CalculsSeparation.GsA" />
</GsProjet>
EOF

cat >"$projet_application" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<GsProject
    Version="1.0"
    Name="ApplicationSeparation"
    Type="executable"
    EntryPoint="Essai::Separation::Principal"
    ApplicationVersion="0.27.0-alpha.1">
    <Interface Path="$racine_source/Tests/Integration/Separation/Calculs.HeaderGsPlusPlus" />
    <Source Path="$racine_source/Tests/Integration/Separation/Principal.GsPP" />
    <Library Path="$repertoire_separation/CalculsSeparation.GsA" />
    <Build
        ObjectDirectory="$repertoire_separation/ProjetApplication/Objets"
        Output="$repertoire_separation/ApplicationProjet.GsE"
        Map="$repertoire_separation/ApplicationProjet.map" />
</GsProject>
EOF

cat >"$solution_compilation" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<GsSolution Version="1.0">
    <Projet Chemin="Bibliotheque.GsPj" />
    <Project Path="Application.GsProject" />
</GsSolution>
EOF

"$compilateur" "$solution_compilation"
"$chargeur" "$repertoire_separation/ApplicationProjet.GsE" --executer \
    | grep -q "Code de retour : 44"
grep -q "Principal.GsPP" "$repertoire_separation/ApplicationProjet.map"
cp "$repertoire_separation/CalculsSeparation.GsA" \
    "$repertoire_separation/CalculsSeparation-premiere.GsA"
cp "$repertoire_separation/ApplicationProjet.GsE" \
    "$repertoire_separation/ApplicationProjet-premiere.GsE"
cp "$repertoire_separation/ApplicationProjet.map" \
    "$repertoire_separation/ApplicationProjet-premiere.map"
"$compilateur" "$solution_compilation" >/dev/null
cmp "$repertoire_separation/CalculsSeparation-premiere.GsA" \
    "$repertoire_separation/CalculsSeparation.GsA"
cmp "$repertoire_separation/ApplicationProjet-premiere.GsE" \
    "$repertoire_separation/ApplicationProjet.GsE"
cmp "$repertoire_separation/ApplicationProjet-premiere.map" \
    "$repertoire_separation/ApplicationProjet.map"

echo "Tests d’intégration Gs++ réussis."
