#!/usr/bin/env python3
"""Non-regression du mode CLI explicite de diagnostics frontend."""

import argparse
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--frontend", required=True, type=Path)
    parser.add_argument("--work-root", required=True, type=Path)
    args = parser.parse_args()
    compiler = args.compiler.resolve()
    frontend = args.frontend.resolve()
    root = args.work_root.resolve()
    root.mkdir(parents=True, exist_ok=True)
    source = root / "Principal.GsPP"
    interface = root / "Étoile.HGsPP"
    numero = 0

    def run(arguments):
        return subprocess.run([str(compiler), *map(str, arguments)], capture_output=True, timeout=60)

    def output(extension="GsObj"):
        nonlocal numero
        numero += 1
        path = root / f"sortie-{numero:03d}.{extension}"
        if path.exists():
            path.unlink()
        return path

    def check(condition, message):
        if not condition:
            raise AssertionError(message)

    def compile_mode(options, extension="GsObj", diagnostics="français"):
        target = output(extension)
        result = run([source, "--format", "gse" if extension == "GsE" else "gsobj", "-o", target,
                      "--langue-diagnostics", diagnostics, *options])
        return result, target

    mots = {"publique": "public", "vide": "void", "classe": "class", "espace": "namespace",
            "utilisant": "using", "entier32": "int32", "booléen": "bool", "retourner": "return",
            "convertir": "cast", "énumération": "enum", "vrai": "true", "structure": "struct",
            "constructeur": "constructor", "destructeur": "destructor", "opérateur": "operator",
            "constante": "const", "pointeur_fonction": "function_pointer", "entier8": "int8", "entier16": "int16"}

    def traduire(texte, anglais):
        return re.sub(r"\b(" + "|".join(mots) + r")\b", lambda match: mots[match[0]], texte) if anglais else texte

    comparaisons_binaires = 0
    for anglais in (False, True):
        interface.write_text(traduire("#pragma once\nespace N { entier32 Lire(); }", anglais), encoding="utf-8")
        source.write_bytes(("\ufeff#include \"Étoile.HGsPP\"\r\n" + traduire(
            "espace N { entier32 Lire() { retourner 42; } }\r\n"
            "publique entier32 Principal() { retourner N::Lire(); }", anglais)).encode("utf-8"))
        reference, fichier = compile_mode(["--jetons", "--ast"])
        check(reference.returncode == 0, reference.stderr.decode("utf-8", "replace"))
        for mode in ("--include-expander", "--diagnostics-frontend", "--frontend-diagnostics"):
            result, cible = compile_mode([mode, frontend, "--jetons", "--ast"])
            check(result.returncode == 0, result.stderr.decode("utf-8", "replace"))
            check(cible.read_bytes() == fichier.read_bytes(), f"objet différent : {mode}")
            check(result.stdout.split("Compilation réussie :".encode())[0]
                  == reference.stdout.split("Compilation réussie :".encode())[0], f"jetons/AST différents : {mode}")
            comparaisons_binaires += 1
        reference, fichier = compile_mode([], "GsE")
        result, cible = compile_mode(["--diagnostics-frontend", frontend], "GsE")
        check(reference.returncode == result.returncode == 0, result.stderr.decode("utf-8", "replace"))
        check(cible.read_bytes() == fichier.read_bytes(), "image différente")
        comparaisons_binaires += 1

    refus = [
        "espace N { classe C { publique: vide M() {} vide F(M* Objet) {} }; }",
        "classe C { publique: vide M() {} }; espace C { publique vide M(C& Objet, booléen V) {} } "
        "utilisant espace C; publique vide F(M* Objet) {}",
        "espace C { publique vide M(C& Objet, booléen V) {} } classe C { publique: vide M() {} }; "
        "utilisant espace C; publique vide F(M* Objet) {}",
        "classe C { publique: vide M() {} }; espace C { publique vide M(Vue& Objet) {} } "
        "alias Vue = C; utilisant espace C; publique vide F() { convertir<M*>(0); }",
        "espace C { publique vide M(Vue& Objet) {} } classe C { publique: vide M() {} }; alias Vue = C;",
        "structure S {}; alias Vue = S; publique vide F(S* a) {} publique vide F(Vue* b) {}",
        "classe C { publique: vide M(C* a) {} vide M(Vue* b) {} }; alias Vue = C;",
        "espace N { structure S {}; alias Vue = S; publique vide F(S* a) {} publique vide F(Vue* b) {} }",
        "classe C { publique: constructeur(C* a) {} constructeur(Vue* b) {} }; alias Vue = C;",
        "classe C { publique: entier32 opérateur+(C* a) { retourner 1; } entier32 opérateur+(Vue* b) { retourner 2; } }; alias Vue = C;",
        "classe C {}; alias Vue = C; publique booléen opérateur!(C& a) { retourner vrai; } "
        "publique booléen opérateur!(Vue& b) { retourner vrai; }",
        "classe C { publique: booléen opérateur!() { retourner vrai; } }; "
        "espace C { publique booléen opérateur!(Vue& a) { retourner vrai; } } alias Vue = C;",
        "espace C { publique entier32 opérateur+(Vue& a, entier32 b) { retourner b; } } "
        "classe C { publique: entier32 opérateur+(entier32 b) { retourner b; } }; alias Vue = C;",
        "classe C { publique: destructeur() {} destructeur() {} };",
        "espace Démo::API { classe C { publique: constructeur(C* a) {} constructeur(Vue* b) {} }; alias Vue = C; }",
        "espace Démo::API { classe C { publique: entier32 opérateur+(C* a) { retourner 1; } "
        "entier32 opérateur+(Vue* b) { retourner 2; } }; alias Vue = C; }",
        "espace Démo::API { classe C { publique: booléen opérateur!() { retourner vrai; } }; "
        "espace C { publique booléen opérateur!(Vue& a) { retourner vrai; } } alias Vue = C; }",
        "espace Démo::API { espace C { publique entier32 opérateur+(Vue& a, entier32 b) { retourner b; } } "
        "classe C { publique: entier32 opérateur+(entier32 b) { retourner b; } }; alias Vue = C; }",
        "classe C { publique: vide M(entier32& V) {} }; espace C { publique vide M(C& Objet, entier32 V) {} } "
        "publique vide F(C& Objet, entier32 V) { Objet.M(V); }",
        "publique vide F() {} publique vide F() {}",
        "publique vide F( {}",
        "énumération E { V = vrai };",
    ]
    a, b = "TypeCollision_Bf_8190k3Dbe13aCfcn", "TypeCollisioncAadlNBpc0Aabp0aAaba"
    ma, mb = "TypeCollisionmNbc8KY0YC1ce13aCfcn", "TypeCollisionaAaddD2paDrbbp0aAaba"
    types = f"structure {a} {{}}; structure {b} {{}}; "
    paire = f"publique vide F({a}* a) {{}} publique vide F({b}* b) {{}}"
    collisions = [
        types + paire,
        types + "espace Démo::API { " + paire + " }",
        types + f"publique entier32 opérateur+({a}& a, entier32 n) {{ retourner n; }} "
                f"publique entier32 opérateur+({b}& b, entier32 n) {{ retourner n; }}",
        f"structure {ma} {{}}; structure {mb} {{}}; classe C {{ publique: "
        f"vide F({ma}* a) {{}} vide F({mb}* b) {{}} }};",
        f"structure {ma} {{}}; structure {mb} {{}}; classe C {{ publique: "
        f"constructeur({ma}* a) {{}} constructeur({mb}* b) {{}} }};",
        types + f"publique vide F({a}* a) {{}} publique vide G({a}* a) {{}} "
                f"publique vide G({b}* b) {{}} publique vide F({b}* b) {{}}",
    ]
    refus.extend(collisions)
    fonctions_ambigues = "publique vide F(entier8 x) {} publique vide F(entier16 x) {} "
    appel_ambigu = "publique vide G() { F(1); }"
    appels_ambigus = [
        fonctions_ambigues + appel_ambigu,
        "espace Démo::API { " + fonctions_ambigues + appel_ambigu + " }",
        "espace Démo::API { " + fonctions_ambigues + " } utilisant espace Démo::API; " + appel_ambigu,
        "classe C { publique: vide M(entier8 x) {} vide M(entier16 x) {} }; publique vide G(C& Objet) { Objet.M(1); }",
        "classe B { publique: vide M(entier8 x) {} vide M(entier16 x) {} }; classe C : publique B {}; "
        "publique vide G(C& Objet) { Objet.M(1); }",
        "classe C { publique: constructeur(entier8 x) {} constructeur(entier16 x) {} }; publique vide G() { C Objet(1); }",
        "structure P {}; publique entier32 opérateur+(P& p, entier8 x) { retourner 1; } "
        "publique entier32 opérateur+(P& p, entier16 x) { retourner 1; } publique vide G(P& p) { p + 1; }",
        "espace A { " + fonctions_ambigues + " } espace B { " + fonctions_ambigues + " } "
        "utilisant espace A; utilisant espace B; " + appel_ambigu,
    ]
    refus.extend(appels_ambigus)
    importe_a = "espace A { publique vide F(entier8 x) {} } "
    importe_b = "espace B { publique vide F(entier16 x) {} } "
    utilisant_ab = "utilisant espace A; utilisant espace B; "
    groupes_importes = [
        importe_a + importe_b + utilisant_ab + appel_ambigu,
        importe_a + importe_b + "espace Relais { " + utilisant_ab + "} utilisant espace Relais; " + appel_ambigu,
        importe_a + importe_b + "espace Démo::API { " + utilisant_ab + appel_ambigu + " }",
        "espace A { espace API { publique vide F(entier8 x) {} } } "
        "espace B { espace API { publique vide F(entier16 x) {} } } " + utilisant_ab
        + "publique vide G() { API :: /* nom relatif */ F(1); }",
        "structure P {}; espace A { publique entier32 opérateur+(P& p, entier32 x) { retourner 1; } } "
        "espace B { publique entier32 opérateur+(P& p, entier32 x) { retourner 1; } } " + utilisant_ab
        + "publique vide G(P& p) { p + 1; }",
        "structure P {}; espace A { publique booléen opérateur!(P& p) { retourner vrai; } } "
        "espace B { publique booléen opérateur!(P& p) { retourner vrai; } } " + utilisant_ab
        + "publique vide G(P& p) { !p; }",
        "espace A { publique vide F(entier32* x) {} } espace B { publique vide F(booléen* x) {} } "
        + utilisant_ab + appel_ambigu,
        "structure P {}; espace A { publique entier32 opérateur+(P& p, entier32* x) { retourner 1; } } "
        "espace B { publique entier32 opérateur+(P& p, booléen* x) { retourner 1; } } " + utilisant_ab
        + "publique vide G(P& p) { p + 1; }",
    ]
    refus.extend(groupes_importes)
    echecs_appels = [
        "publique vide F(entier32* p) {} publique vide G() { F(); }",
        "publique vide F(entier32* p) {} publique vide G() { F(1); }",
        "publique vide F(entier32* p) {} publique vide G() { F(1, 2); }",
        "publique vide F(entier32& p) {} publique vide G() { F(1); }",
        "espace Démo::API { publique vide F(entier32* p) {} publique vide G() { F(1); } }",
        "classe C { publique: vide M(entier32* p) {} }; publique vide G(C& c) { c.M(1); }",
        "classe C { publique: vide M(entier32* p) {} }; publique vide G(C& c) { C::M(c, 1); }",
        "classe Base { publique: vide M(entier32* p) {} }; classe C : Base {}; publique vide G(C& c) { c.M(1); }",
        "publique vide F(entier32* p, entier32* q) {} publique vide G() { F(1, Absente); }",
        "espace A { publique vide F(entier32* p) {} } espace B { publique vide F(booléen* p) {} } "
        + utilisant_ab + "publique vide G() { F(); }",
        "classe C { publique: entier32 opérateur+(entier32* p) { retourner 1; } }; "
        "espace C { publique entier32 opérateur+(C& c, booléen* p) { retourner 1; } } publique vide G(C& c) { c + 1; }",
        "classe C { publique: constructeur(entier32* p) {} }; publique vide G() { C c(1); }",
        "publique vide F(entier32* p) {} alias Appeler = F; publique vide G() { Appeler(1); }",
        "espace N { publique vide F(entier32* p) {} alias Appeler = F; publique vide G() { Appeler(1); } }",
        "classe C { publique: vide M(entier32* p) {} }; alias Appeler = C::M; publique vide G(C& c) { Appeler(c, 1); }",
        "espace A { publique vide F(entier32* p) {} } espace N { alias F = A::F; publique vide G() { F(1); } }",
        "espace A { publique vide F(entier32* p) {} } espace N { alias F = A::F; } utilisant espace N; publique vide G() { F(1); }",
    ]
    refus.extend(echecs_appels)
    constructions = [
        "classe C { publique: constructeur(entier32* p) {} }; publique vide G() { C Objet(); }",
        "classe C { publique: constructeur(entier32* p) {} }; publique vide G() { C Objet; }",
        "classe C { publique: constructeur(entier32* p) {} }; publique vide G() { C Objets[2](1); }",
        "espace Démo::API { classe C { publique: constructeur(entier32* p) {} }; publique vide G() { C Objet(1); } }",
        "classe C { publique: constructeur(entier32* p) {} }; alias Vue = C; publique vide G() { Vue Objet(1); }",
        "classe C { publique: constructeur(entier32* p) {} constructeur() : soi(1) {} };",
        "classe B { publique: constructeur(entier32* p) {} }; classe C : publique B { publique: constructeur() : parent(1) {} };",
        "classe M { publique: constructeur(entier32* p) {} }; classe C { M m; publique: constructeur() : m(1) {} };",
        "classe M { publique: constructeur(entier32* p) {} }; classe C { M m[2]; publique: constructeur() : m(1) {} };",
        "classe C { publique: constructeur(entier32* p, entier32* q) {} }; publique vide G() { C Objet(1, Absente); }",
        "classe M { publique: constructeur(entier32* p) {} }; classe C { M m; }; publique vide G() { C Objet; }",
        "classe B { publique: constructeur(entier32* p) {} }; classe C : publique B { publique: constructeur() {} };",
    ]
    refus.extend(constructions)
    constructions_implicites = [
        "classe M { publique: constructeur(entier32* p) {} }; classe C { M m; publique: constructeur() {} };",
        "classe M { publique: constructeur(entier32* p) {} }; classe C { M m[2][3]; publique: constructeur() {} };",
        "classe M { publique: constructeur(entier32* p) {} }; classe N { M m; }; classe C { N n; publique: constructeur() {} };",
        "classe M { publique: constructeur(entier32* p) {} }; alias Vue = M; classe C { Vue m; publique: constructeur() {} };",
        "espace Démo::API { classe M { publique: constructeur(entier32* p) {} }; classe C { M Étoile; publique: constructeur() {} }; }",
        "classe B { publique: constructeur(entier32* p) {} }; classe C : publique B { publique: constructeur(entier32 p) {} };",
        "classe B { publique: constructeur(entier32* p) {} }; classe N : publique B {}; classe C : publique N { publique: constructeur() {} };",
        "classe B { publique: constructeur(entier32* p) {} }; classe C : publique B { publique: constructeur() { Absente; } };",
    ]
    refus.extend(constructions_implicites)
    alias_appels = [
        "publique vide F(entier32* p) {} alias Appeler = F; publique vide G() { Appeler(); }",
        "publique vide F(entier32* p) {} alias Appeler = F; publique vide G() { Appeler(1, 2); }",
        "espace Démo::API { publique vide F(entier32* p) {} alias Appeler = F; publique vide G() { Appeler(1); } }",
        "espace API { publique vide F(entier32* p) {} } alias Appeler = API::F; publique vide G() { Appeler(1); }",
        "publique vide F(entier32* p) {} alias Démo::Appeler = F; publique vide G() { Démo::Appeler(1); }",
        "publique vide F(entier32* p) {} alias Appeler = Copie; alias Copie = F; publique vide G() { Appeler(1); }",
        "classe C { publique: vide M(entier32* p) {} }; alias Appeler = C::M; publique vide G(C& c) { Appeler(c); }",
        "publique vide F(entier32* p, entier32* q) {} alias Appeler = F; publique vide G() { Appeler(1, Absente); }",
    ]
    alias_mixtes = [
        "espace Original { publique vide F(entier32* p) {} } espace A { alias F = Original::F; } "
        "espace B { publique vide F(booléen* p) {} } " + utilisant_ab + "publique vide G() { F(1); }",
        "espace Original { publique vide F(entier32* p) {} } espace A { alias F = Original::F; } "
        "espace B { alias F = Original::F; } " + utilisant_ab + "publique vide G() { F(1); }",
        "espace Original { publique vide F(entier32* p) {} } espace A { alias F = Original::F; } "
        "espace B { alias F = Original::F; } utilisant espace B; utilisant espace A; publique vide G() { F(); }",
        "espace Original { publique vide F(entier32* p) {} } espace A { alias F = Original::F; } "
        "utilisant espace A; utilisant espace Original; publique vide G() { F(1); }",
        "espace Original { publique vide F(entier32* p) {} } espace A { espace API { alias F = Original::F; } } "
        "espace B { espace API { alias F = Original::F; } } " + utilisant_ab + "publique vide G() { API :: /* nom */ F(1); }",
        "espace Original { publique vide F(entier32* p) {} } espace A { alias Écho = Original::F; } "
        "espace B { alias Écho = Original::F; } " + utilisant_ab + "publique vide G() { Écho(1); }",
        "espace Original { publique vide F(entier32* p, entier32* q) {} } espace A { alias F = Original::F; } "
        "espace B { alias F = Original::F; } " + utilisant_ab + "publique vide G() { F(1, Absente); }",
        "classe C { publique: vide M(entier32* p) {} }; espace A { alias F = C::M; } "
        "espace B { publique vide F(C& c, booléen* p) {} } " + utilisant_ab + "publique vide G(C& c) { F(c, 1); }",
    ]
    alias_appels.extend(alias_mixtes)
    refus.extend(alias_appels)
    appels_indirects = [
        "publique vide G(entier32 valeur) { valeur(); }",
        "structure S { entier32 Valeur; }; publique vide G(S& s) { s.Valeur(); }",
        "publique vide G() { (1 + 2)(); }",
        "publique vide G(entier32* valeurs) { valeurs[0](); }",
        "publique vide G(pointeur_fonction<vide(entier32)>* operations) { operations(1); }",
        "entier32 Lire() { retourner 1; } publique vide G() { Lire()(); }",
        "publique vide G(pointeur_fonction<vide(entier32)> operation) { operation(); }",
        "publique vide G(pointeur_fonction<vide(entier32)> operation) { operation(1, Absente); }",
        "publique vide G(pointeur_fonction<vide(entier32)>& operation) { operation(); }",
        "publique vide G(pointeur_fonction<vide(entier32)>* operations) { operations[0](); }",
        "vide Cible(entier32 valeur) {} pointeur_fonction<vide(entier32)> Creer() { retourner &Cible; } publique vide G() { Creer()(1, 2); }",
        "publique vide G(pointeur_fonction<vide(entier32)> operation, entier32 valeur) { valeur(); operation(); }",
        "publique vide G(pointeur_fonction<vide(entier32)> operation, entier32 valeur) { operation(); valeur(); }",
        "publique vide G(pointeur_fonction<vide(entier32)> operation) { operation(vrai); }",
    ]
    refus.extend(appels_indirects)
    comparaisons_refus = 0
    for texte in refus:
        for anglais in (False, True):
            source.write_text(traduire(texte + " publique entier32 Principal() { retourner 42; }", anglais), encoding="utf-8")
            for langue in ("français", "anglais"):
                reference, fichier = compile_mode([], diagnostics=langue)
                result, cible = compile_mode(["--frontend-diagnostics", frontend], diagnostics=langue)
                check(reference.returncode == result.returncode == 2, result.stderr.decode("utf-8", "replace"))
                check(b"GS1001" in result.stderr and result.stderr == reference.stderr, "message/position CLI différents")
                check(b"@GsUsing::" not in result.stderr, "clé de groupe interne affichée dans le diagnostic")
                check(not fichier.exists() and not cible.exists(), "sortie produite sur refus")
                comparaisons_refus += 1

    source.write_text("public int32 Principal() { return 42; }", encoding="utf-8")
    invalide = root / "Invalide.GsE"
    invalide.write_bytes(b"image invalide")
    partiel = root / "SansNomCanonique.GsE"
    contenu = frontend.read_bytes()
    nom = b"GalacticShrine::GsPP::Autohebergement::AnalyserSemantiqueUnitesAvecNom"
    check(contenu.count(nom) == 1, "export canonique absent ou multiple")
    partiel.write_bytes(contenu.replace(nom, nom.replace(b"::Analyser", b"::Xnalyser"), 1))
    result, cible = compile_mode(["--include-expander", partiel])
    check(result.returncode == 0 and cible.exists(), "extension facultative imposée à l'ancien mode")
    options_invalides = [
        [source, "--diagnostics-frontend"],
        [source, "--frontend-diagnostics", ""],
        [source, "--diagnostics-frontend", "--ast"],
        [source, "--diagnostics-frontend", frontend, "--frontend-diagnostics", frontend],
        [source, "--diagnostics-frontend", frontend, "--include-expander", frontend],
        [root / "Absent.GsObj", "--diagnostics-frontend", frontend],
        [root / "Absent.GsA", "--diagnostics-frontend", frontend],
        [root / "Absent.GsPj", "--diagnostics-frontend", frontend, "--format", "gsobj"],
        [root / "Absent.GsPs", "--diagnostics-frontend", frontend],
        [source, "--diagnostics-frontend", root / "Absent.GsE"],
        [source, "--diagnostics-frontend", invalide],
        [source, "--diagnostics-frontend", partiel],
    ]
    for options in options_invalides:
        cible = output()
        result = run([*options, "-o", cible])
        check(result.returncode == 1 and b"GS0001" in result.stderr and b"GS1001" not in result.stderr,
              "refus d'option/image incorrect : " + result.stderr.decode("utf-8", "replace"))
        check(not cible.exists(), "sortie créée après refus d'option/image")
    contexte_fr = b"GalacticShrine::GsPP::Autohebergement::AnalyserSemantiqueUnitesAvecContexte"
    contexte_en = b"GalacticShrine::GsPP::Autohebergement::AnalyzeContextualUnitSemantics"
    liaison_fr = b"GalacticShrine::GsPP::Autohebergement::AnalyserSemantiqueUnitesAvecLiaison"
    liaison_en = b"GalacticShrine::GsPP::Autohebergement::AnalyzeLinkUnitSemantics"
    appels_fr = b"GalacticShrine::GsPP::Autohebergement::AnalyserSemantiqueUnitesAvecAppels"
    appels_en = b"GalacticShrine::GsPP::Autohebergement::AnalyzeCallUnitSemantics"
    groupes_fr = b"GalacticShrine::GsPP::Autohebergement::AnalyserSemantiqueUnitesAvecGroupes"
    groupes_en = b"GalacticShrine::GsPP::Autohebergement::AnalyzeGroupUnitSemantics"
    echecs_fr = b"GalacticShrine::GsPP::Autohebergement::AnalyserSemantiqueUnitesAvecEchecsAppels"
    echecs_en = b"GalacticShrine::GsPP::Autohebergement::AnalyzeFailedCallUnitSemantics"
    constructions_fr = b"GalacticShrine::GsPP::Autohebergement::AnalyserSemantiqueUnitesAvecConstructions"
    constructions_en = b"GalacticShrine::GsPP::Autohebergement::AnalyzeConstructionUnitSemantics"
    implicites_fr = b"GalacticShrine::GsPP::Autohebergement::AnalyserSemantiqueUnitesAvecConstructionImplicite"
    implicites_en = b"GalacticShrine::GsPP::Autohebergement::AnalyzeImplicitConstructionUnitSemantics"
    alias_fr = b"GalacticShrine::GsPP::Autohebergement::AnalyserSemantiqueUnitesAvecAliasAppels"
    alias_en = b"GalacticShrine::GsPP::Autohebergement::AnalyzeAliasCallUnitSemantics"
    check(contenu.count(contexte_fr) == contenu.count(contexte_en) == 1, "exports contextuels absents ou multiples")
    check(contenu.count(liaison_fr) == contenu.count(liaison_en) == 1, "exports de liaison absents ou multiples")
    check(contenu.count(appels_fr) == contenu.count(appels_en) == 1, "exports d'appels absents ou multiples")
    check(contenu.count(groupes_fr) == contenu.count(groupes_en) == 1, "exports de groupes absents ou multiples")
    check(contenu.count(echecs_fr) == contenu.count(echecs_en) == 1, "exports d'échecs absents ou multiples")
    check(contenu.count(constructions_fr) == contenu.count(constructions_en) == 1, "exports de constructions absents ou multiples")
    check(contenu.count(implicites_fr) == contenu.count(implicites_en) == 1, "exports de constructions implicites absents ou multiples")
    check(contenu.count(alias_fr) == contenu.count(alias_en) == 1, "exports d'appels d'alias absents ou multiples")
    sans_alias_contenu = contenu.replace(alias_fr, alias_fr.replace(b"::Analyser", b"::Xnalyser"), 1) \
                               .replace(alias_en, alias_en.replace(b"::Analyze", b"::Xnalyze"), 1)
    sans_implicites_contenu = sans_alias_contenu.replace(implicites_fr, implicites_fr.replace(b"::Analyser", b"::Xnalyser"), 1) \
                                   .replace(implicites_en, implicites_en.replace(b"::Analyze", b"::Xnalyze"), 1)
    sans_constructions_contenu = sans_implicites_contenu.replace(constructions_fr, constructions_fr.replace(b"::Analyser", b"::Xnalyser"), 1) \
                                       .replace(constructions_en, constructions_en.replace(b"::Analyze", b"::Xnalyze"), 1)
    sans_echecs_contenu = sans_constructions_contenu.replace(echecs_fr, echecs_fr.replace(b"::Analyser", b"::Xnalyser"), 1) \
                                .replace(echecs_en, echecs_en.replace(b"::Analyze", b"::Xnalyze"), 1)
    sans_groupes_contenu = sans_echecs_contenu.replace(groupes_fr, groupes_fr.replace(b"::Analyser", b"::Xnalyser"), 1) \
                                 .replace(groupes_en, groupes_en.replace(b"::Analyze", b"::Xnalyze"), 1)
    sans_appels_contenu = sans_groupes_contenu.replace(appels_fr, appels_fr.replace(b"::Analyser", b"::Xnalyser"), 1) \
                                .replace(appels_en, appels_en.replace(b"::Analyze", b"::Xnalyze"), 1)
    ancien_contenu = sans_appels_contenu.replace(liaison_fr, liaison_fr.replace(b"::Analyser", b"::Xnalyser"), 1) \
                            .replace(liaison_en, liaison_en.replace(b"::Analyze", b"::Xnalyze"), 1)
    sans_contexte = root / "SansContexte.GsE"
    sans_contexte.write_bytes(ancien_contenu.replace(contexte_fr, contexte_fr.replace(b"::Analyser", b"::Xnalyser"), 1)
                             .replace(contexte_en, contexte_en.replace(b"::Analyze", b"::Xnalyze"), 1))
    partiel_contexte = root / "ContextePartiel.GsE"
    partiel_contexte.write_bytes(contenu.replace(contexte_fr, contexte_fr.replace(b"::Analyser", b"::Xnalyser"), 1))
    reference, fichier = compile_mode([])
    result, cible = compile_mode(["--frontend-diagnostics", sans_contexte])
    check(reference.returncode == result.returncode == 0 and fichier.read_bytes() == cible.read_bytes(),
          "extension contextuelle facultative imposée")
    for mode in ("--include-expander", "--frontend-diagnostics"):
        result, cible = compile_mode([mode, partiel_contexte])
        check((result.returncode == 0 and cible.exists()) if mode == "--include-expander" else
              (result.returncode == 1 and b"GS0001" in result.stderr and not cible.exists()),
              "paire contextuelle partielle mal contrôlée")
    replis_contextes = 0
    for anglais in (False, True):
        source.write_text(traduire("espace N { classe C { publique: constructeur(C* a) {} constructeur(Vue* b) {} }; alias Vue = C; }", anglais), encoding="utf-8")
        for langue in ("français", "anglais"):
            reference, fichier = compile_mode([], diagnostics=langue)
            result, cible = compile_mode(["--frontend-diagnostics", sans_contexte], diagnostics=langue)
            check(reference.returncode == result.returncode == 2 and reference.stderr == result.stderr
                  and b"GS1001" in result.stderr and not fichier.exists() and not cible.exists(),
                  "ancien repli de surcharge qualifiée perdu")
            replis_contextes += 1
    sans_liaison = root / "SansLiaison.GsE"
    sans_liaison.write_bytes(ancien_contenu)
    partiel_liaison = root / "LiaisonPartielle.GsE"
    partiel_liaison.write_bytes(contenu.replace(liaison_fr, liaison_fr.replace(b"::Analyser", b"::Xnalyser"), 1))
    source.write_text("public int32 Principal() { return 42; }", encoding="utf-8")
    reference, fichier = compile_mode([])
    result, cible = compile_mode(["--frontend-diagnostics", sans_liaison])
    check(reference.returncode == result.returncode == 0 and fichier.read_bytes() == cible.read_bytes(),
          "extension de liaison facultative imposée")
    for mode in ("--include-expander", "--frontend-diagnostics"):
        result, cible = compile_mode([mode, partiel_liaison])
        check((result.returncode == 0 and cible.exists()) if mode == "--include-expander" else
              (result.returncode == 1 and b"GS0001" in result.stderr and not cible.exists()),
              "paire de liaison partielle mal contrôlée")
    replis_liaison = 0
    for texte in collisions:
        for anglais in (False, True):
            source.write_text(traduire(texte, anglais), encoding="utf-8")
            for langue in ("français", "anglais"):
                reference, fichier = compile_mode([], diagnostics=langue)
                result, cible = compile_mode(["--frontend-diagnostics", sans_liaison], diagnostics=langue)
                check(reference.returncode == result.returncode == 2 and reference.stderr == result.stderr
                      and b"GS1001" in result.stderr and not fichier.exists() and not cible.exists(),
                      "ancien repli de collision perdu")
                replis_liaison += 1
    sans_appels = root / "SansAppels.GsE"
    sans_appels.write_bytes(sans_appels_contenu)
    partiel_appels = root / "AppelsPartiels.GsE"
    partiel_appels.write_bytes(contenu.replace(appels_fr, appels_fr.replace(b"::Analyser", b"::Xnalyser"), 1))
    source.write_text("public int32 Principal() { return 42; }", encoding="utf-8")
    reference, fichier = compile_mode([])
    result, cible = compile_mode(["--frontend-diagnostics", sans_appels])
    check(reference.returncode == result.returncode == 0 and fichier.read_bytes() == cible.read_bytes(),
          "extension d'appels facultative imposée")
    for mode in ("--include-expander", "--frontend-diagnostics"):
        result, cible = compile_mode([mode, partiel_appels])
        check((result.returncode == 0 and cible.exists()) if mode == "--include-expander" else
              (result.returncode == 1 and b"GS0001" in result.stderr and not cible.exists()),
              "paire d'appels partielle mal contrôlée")
    replis_appels = 0
    for texte in appels_ambigus:
        for anglais in (False, True):
            source.write_text(traduire(texte, anglais), encoding="utf-8")
            for langue in ("français", "anglais"):
                reference, fichier = compile_mode([], diagnostics=langue)
                result, cible = compile_mode(["--frontend-diagnostics", sans_appels], diagnostics=langue)
                check(reference.returncode == result.returncode == 2 and reference.stderr == result.stderr
                      and b"GS1001" in result.stderr and not fichier.exists() and not cible.exists(),
                      "ancien repli d'appel ambigu perdu")
                replis_appels += 1
    sans_groupes = root / "SansGroupes.GsE"
    sans_groupes.write_bytes(sans_groupes_contenu)
    partiel_groupes = root / "GroupesPartiels.GsE"
    partiel_groupes.write_bytes(contenu.replace(groupes_fr, groupes_fr.replace(b"::Analyser", b"::Xnalyser"), 1))
    source.write_text("public int32 Principal() { return 42; }", encoding="utf-8")
    reference, fichier = compile_mode([])
    result, cible = compile_mode(["--frontend-diagnostics", sans_groupes])
    check(reference.returncode == result.returncode == 0 and fichier.read_bytes() == cible.read_bytes(),
          "extension de groupes facultative imposée")
    for mode in ("--include-expander", "--frontend-diagnostics"):
        result, cible = compile_mode([mode, partiel_groupes])
        check((result.returncode == 0 and cible.exists()) if mode == "--include-expander" else
              (result.returncode == 1 and b"GS0001" in result.stderr and not cible.exists()),
              "paire de groupes partielle mal contrôlée")
    replis_groupes = 0
    for texte in groupes_importes:
        for anglais in (False, True):
            source.write_text(traduire(texte, anglais), encoding="utf-8")
            for langue in ("français", "anglais"):
                reference, fichier = compile_mode([], diagnostics=langue)
                result, cible = compile_mode(["--frontend-diagnostics", sans_groupes], diagnostics=langue)
                check(reference.returncode == result.returncode == 2 and reference.stderr == result.stderr
                      and b"GS1001" in result.stderr and b"@GsUsing::" not in result.stderr
                      and not fichier.exists() and not cible.exists(), "ancien repli de groupe importé perdu")
                replis_groupes += 1
    sans_echecs = root / "SansEchecsAppels.GsE"
    sans_echecs.write_bytes(sans_echecs_contenu)
    partiel_echecs = root / "EchecsAppelsPartiels.GsE"
    partiel_echecs.write_bytes(contenu.replace(echecs_fr, echecs_fr.replace(b"::Analyser", b"::Xnalyser"), 1))
    source.write_text("public int32 Principal() { return 42; }", encoding="utf-8")
    reference, fichier = compile_mode([])
    result, cible = compile_mode(["--frontend-diagnostics", sans_echecs])
    check(reference.returncode == result.returncode == 0 and fichier.read_bytes() == cible.read_bytes(),
          "extension d'échecs facultative imposée")
    for mode in ("--include-expander", "--frontend-diagnostics"):
        result, cible = compile_mode([mode, partiel_echecs])
        check((result.returncode == 0 and cible.exists()) if mode == "--include-expander" else
              (result.returncode == 1 and b"GS0001" in result.stderr and not cible.exists()),
              "paire d'échecs partielle mal contrôlée")
    replis_echecs = 0
    for texte in echecs_appels:
        for anglais in (False, True):
            source.write_text(traduire(texte, anglais), encoding="utf-8")
            for langue in ("français", "anglais"):
                reference, fichier = compile_mode([], diagnostics=langue)
                result, cible = compile_mode(["--frontend-diagnostics", sans_echecs], diagnostics=langue)
                check(reference.returncode == result.returncode == 2 and reference.stderr == result.stderr
                      and b"GS1001" in result.stderr and not fichier.exists() and not cible.exists(),
                      "ancien repli d'échec d'appel perdu")
                replis_echecs += 1
    sans_constructions = root / "SansConstructions.GsE"
    sans_constructions.write_bytes(sans_constructions_contenu)
    partiel_constructions = root / "ConstructionsPartielles.GsE"
    partiel_constructions.write_bytes(contenu.replace(constructions_fr, constructions_fr.replace(b"::Analyser", b"::Xnalyser"), 1))
    source.write_text("public int32 Principal() { return 42; }", encoding="utf-8")
    reference, fichier = compile_mode([])
    result, cible = compile_mode(["--frontend-diagnostics", sans_constructions])
    check(reference.returncode == result.returncode == 0 and fichier.read_bytes() == cible.read_bytes(),
          "extension de constructions facultative imposée")
    for mode in ("--include-expander", "--frontend-diagnostics"):
        result, cible = compile_mode([mode, partiel_constructions])
        check((result.returncode == 0 and cible.exists()) if mode == "--include-expander" else
              (result.returncode == 1 and b"GS0001" in result.stderr and not cible.exists()),
              "paire de constructions partielle mal contrôlée")
    replis_constructions = 0
    for texte in constructions:
        for anglais in (False, True):
            source.write_text(traduire(texte, anglais), encoding="utf-8")
            for langue in ("français", "anglais"):
                reference, fichier = compile_mode([], diagnostics=langue)
                result, cible = compile_mode(["--frontend-diagnostics", sans_constructions], diagnostics=langue)
                check(reference.returncode == result.returncode == 2 and reference.stderr == result.stderr
                      and b"GS1001" in result.stderr and not fichier.exists() and not cible.exists(),
                      "ancien repli de construction perdu")
                replis_constructions += 1
    sans_implicites = root / "SansConstructionImplicite.GsE"
    sans_implicites.write_bytes(sans_implicites_contenu)
    partiel_implicites = root / "ConstructionImplicitePartielle.GsE"
    partiel_implicites.write_bytes(contenu.replace(implicites_fr, implicites_fr.replace(b"::Analyser", b"::Xnalyser"), 1))
    source.write_text("public int32 Principal() { return 42; }", encoding="utf-8")
    reference, fichier = compile_mode([])
    result, cible = compile_mode(["--frontend-diagnostics", sans_implicites])
    check(reference.returncode == result.returncode == 0 and fichier.read_bytes() == cible.read_bytes(),
          "extension de construction implicite facultative imposée")
    for mode in ("--include-expander", "--frontend-diagnostics"):
        result, cible = compile_mode([mode, partiel_implicites])
        check((result.returncode == 0 and cible.exists()) if mode == "--include-expander" else
              (result.returncode == 1 and b"GS0001" in result.stderr and not cible.exists()),
              "paire de construction implicite partielle mal contrôlée")
    replis_implicites = 0
    for texte in constructions_implicites:
        for anglais in (False, True):
            source.write_text(traduire(texte, anglais), encoding="utf-8")
            for langue in ("français", "anglais"):
                reference, fichier = compile_mode([], diagnostics=langue)
                result, cible = compile_mode(["--frontend-diagnostics", sans_implicites], diagnostics=langue)
                check(reference.returncode == result.returncode == 2 and reference.stderr == result.stderr
                      and b"GS1001" in result.stderr and not fichier.exists() and not cible.exists(),
                      "ancien repli de construction implicite perdu")
                replis_implicites += 1
    sans_alias = root / "SansAliasAppels.GsE"
    sans_alias.write_bytes(sans_alias_contenu)
    partiel_alias = root / "AliasAppelsPartiels.GsE"
    partiel_alias.write_bytes(contenu.replace(alias_fr, alias_fr.replace(b"::Analyser", b"::Xnalyser"), 1))
    source.write_text("public int32 Principal() { return 42; }", encoding="utf-8")
    reference, fichier = compile_mode([])
    result, cible = compile_mode(["--frontend-diagnostics", sans_alias])
    check(reference.returncode == result.returncode == 0 and fichier.read_bytes() == cible.read_bytes(),
          "extension d'appels d'alias facultative imposée")
    for mode in ("--include-expander", "--frontend-diagnostics"):
        result, cible = compile_mode([mode, partiel_alias])
        check((result.returncode == 0 and cible.exists()) if mode == "--include-expander" else
              (result.returncode == 1 and b"GS0001" in result.stderr and not cible.exists()),
              "paire d'appels d'alias partielle mal contrôlée")
    replis_alias = 0
    for texte in alias_appels:
        for anglais in (False, True):
            source.write_text(traduire(texte, anglais), encoding="utf-8")
            for langue in ("français", "anglais"):
                reference, fichier = compile_mode([], diagnostics=langue)
                result, cible = compile_mode(["--frontend-diagnostics", sans_alias], diagnostics=langue)
                check(reference.returncode == result.returncode == 2 and reference.stderr == result.stderr
                      and b"GS1001" in result.stderr and not fichier.exists() and not cible.exists(),
                      "ancien repli d'appel d'alias perdu")
                replis_alias += 1
    for anglais in (False, True):
        valide_importe = ("espace A { publique entier32 F(entier8 x) { retourner 21; } } "
                          "espace B { publique entier32 F(entier16 x) { retourner 42; } } " + utilisant_ab
                          + "publique entier32 Principal() { retourner F(convertir<entier16>(1)); }")
        source.write_text(traduire(valide_importe, anglais), encoding="utf-8")
        for extension in ("GsObj", "GsE"):
            reference, fichier = compile_mode([], extension=extension)
            result, cible = compile_mode(["--frontend-diagnostics", frontend], extension=extension)
            check(reference.returncode == result.returncode == 0 and fichier.read_bytes() == cible.read_bytes(),
                  "sélection importée valide ou binaire modifiés")
            comparaisons_binaires += 1
    for anglais in (False, True):
        source.write_text(traduire("classe C { publique: constructeur(entier32 p) {} }; "
                                   "publique entier32 Principal() { C Objet(42); retourner 42; }", anglais), encoding="utf-8")
        for extension in ("GsObj", "GsE"):
            reference, fichier = compile_mode([], extension=extension)
            result, cible = compile_mode(["--frontend-diagnostics", frontend], extension=extension)
            check(reference.returncode == result.returncode == 0 and fichier.read_bytes() == cible.read_bytes(),
                  "construction valide ou binaire modifiés")
            comparaisons_binaires += 1
    for anglais in (False, True):
        source.write_text(traduire("espace Original { publique entier32 F(entier32 x) { retourner x; } } "
                                   "espace API { alias Appeler = Original::F; } "
                                   "publique entier32 Principal() { retourner API::Appeler(42); }", anglais), encoding="utf-8")
        for extension in ("GsObj", "GsE"):
            reference, fichier = compile_mode([], extension=extension)
            result, cible = compile_mode(["--frontend-diagnostics", frontend], extension=extension)
            check(reference.returncode == result.returncode == 0 and fichier.read_bytes() == cible.read_bytes(),
                  "appel d'alias valide ou binaire modifiés")
            comparaisons_binaires += 1
    for anglais in (False, True):
        source.write_text(traduire("espace Original { publique entier32 Gauche(entier8 x) { retourner 21; } "
                                   "publique entier32 Droite(entier16 x) { retourner 42; } } "
                                   "espace A { alias F = Original::Gauche; } espace B { alias F = Original::Droite; } "
                                   + utilisant_ab + "publique entier32 Principal() { retourner F(convertir<entier16>(1)); }", anglais), encoding="utf-8")
        for extension in ("GsObj", "GsE"):
            reference, fichier = compile_mode([], extension=extension)
            result, cible = compile_mode(["--frontend-diagnostics", frontend], extension=extension)
            check(reference.returncode == result.returncode == 0 and fichier.read_bytes() == cible.read_bytes(),
                  "appel d'alias importés valide ou binaire modifiés")
            comparaisons_binaires += 1
    for anglais in (False, True):
        source.write_text(traduire("publique entier32 Cible(entier32 x) { retourner x; } "
                                   "publique entier32 Principal() { pointeur_fonction<entier32(entier32)> operation = &Cible; "
                                   "retourner operation(42); }", anglais), encoding="utf-8")
        for extension in ("GsObj", "GsE"):
            reference, fichier = compile_mode([], extension=extension)
            result, cible = compile_mode(["--frontend-diagnostics", frontend], extension=extension)
            check(reference.returncode == result.returncode == 0 and fichier.read_bytes() == cible.read_bytes(),
                  "appel indirect valide ou binaire modifiés")
            comparaisons_binaires += 1
    solutions = remplacements = priorites_projets = 0
    for anglais in (False, True):
        for agrege in (False, True):
            projet = root / f"Projets-{'EN' if anglais else 'FR'}-{'Agrege' if agrege else 'Separe'}"
            projet.mkdir(exist_ok=True)

            def ecrire(nom, texte):
                (projet / nom).write_text(traduire(texte, anglais), encoding="utf-8")

            ecrire("Proto.HGsPP", "#pragma once\nespace Demo { publique entier32 F(); publique entier32 G(); }")
            ecrire("Api.HGsPP", '#include "Proto.HGsPP"')
            source_a = '#include "Proto.HGsPP"\nespace Demo { publique entier32 F() { retourner 40; } }'
            ecrire("A.GsPP", source_a)
            ecrire("B.GsPP", '#include "Proto.HGsPP"\nespace Demo { publique entier32 G() { retourner 2; } }')
            ecrire("Principal.GsPP", '#include "Proto.HGsPP"\nutilisant espace Demo; publique entier32 Principal() { retourner F() + G(); }')
            if anglais:
                bibliotheque = (f'<GsProject Version="1.0" Type="library" CompilationMode="{"aggregate" if agrege else "separate"}">'
                               '<Interface Path="Api.HGsPP"/><Source Path="A.GsPP"/><Source Path="B.GsPP"/>'
                               '<Build ObjectDirectory="ObjLib" Output="Lib.GsA"/></GsProject>')
                application = ('<GsProject Version="1.0"><Interface Path="Api.HGsPP"/><Source Path="Principal.GsPP"/>'
                               '<Library Path="Lib.GsA"/><Build ObjectDirectory="ObjApp" Output="App.GsE" Map="App.map"/></GsProject>')
            else:
                bibliotheque = (f'<GsProjet Version="1.0" Type="bibliotheque" ModeCompilation="{"agregee" if agrege else "separee"}">'
                               '<Interface Chemin="Api.HGsPP"/><Source Chemin="A.GsPP"/><Source Chemin="B.GsPP"/>'
                               '<Construction RepertoireObjets="ObjLib" Sortie="Lib.GsA"/></GsProjet>')
                application = ('<GsProjet Version="1.0"><Interface Chemin="Api.HGsPP"/><Source Chemin="Principal.GsPP"/>'
                               '<Bibliotheque Chemin="Lib.GsA"/><Construction RepertoireObjets="ObjApp" Sortie="App.GsE" Carte="App.map"/></GsProjet>')
            (projet / "Lib.GsPj").write_text(bibliotheque, encoding="utf-8")
            (projet / "App.GsProject").write_text(application, encoding="utf-8")
            (projet / "Solution.GsPs").write_text('<GsSolution Version="1.0"><Projet Chemin="Lib.GsPj"/>'
                                               '<Project Path="App.GsProject"/></GsSolution>', encoding="utf-8")

            def instantane():
                fichiers = [projet / nom for nom in ("Lib.GsA", "App.GsE", "App.map")]
                for dossier in ("ObjLib", "ObjApp"):
                    fichiers.extend((projet / dossier).glob("*.GsObj"))
                return {str(path.relative_to(projet)): path.read_bytes() for path in fichiers}

            reference = run([projet / "Solution.GsPs"])
            check(reference.returncode == 0, reference.stderr.decode("utf-8", "replace"))
            avant = instantane()
            for mode in ("--include-expander", "--diagnostics-frontend", "--frontend-diagnostics"):
                result = run([projet / "Solution.GsPs", mode, frontend])
                check(result.returncode == 0 and result.stdout == reference.stdout and instantane() == avant,
                      "solution/journal différents : " + result.stderr.decode("utf-8", "replace"))
                solutions += 1
            cible = projet / "Autre.GsA"
            result = run([projet / "Lib.GsPj", "--diagnostics-frontend", frontend, "-o", cible,
                          "--repertoire-objets", projet / "ObjAutre"])
            check(result.returncode == 0 and cible.read_bytes() == avant["Lib.GsA"] and instantane() == avant,
                  "remplacements de projet perdus")
            remplacements += 1
            ecrire("A.GsPP", "espace N { classe C { publique: vide M() {} vide F(M* Objet) {} }; }")
            reference = run([projet / "Lib.GsPj"])
            result = run([projet / "Lib.GsPj", "--diagnostics-frontend", frontend])
            check(reference.returncode == result.returncode == 2 and result.stderr == reference.stderr and instantane() == avant,
                  "premier refus de projet différent ou anciennes sorties remplacées")
            priorites_projets += 1
            ecrire("A.GsPP", importe_a + importe_b + utilisant_ab + appel_ambigu)
            reference = run([projet / "Lib.GsPj"])
            result = run([projet / "Lib.GsPj", "--diagnostics-frontend", frontend])
            check(reference.returncode == result.returncode == 2 and result.stderr == reference.stderr
                  and b"appel de surcharge ambigu pour F" in result.stderr and b"@GsUsing::" not in result.stderr
                  and instantane() == avant, "groupe importé de projet différent ou anciennes sorties remplacées")
            priorites_projets += 1
            ecrire("A.GsPP", "publique vide F(entier32* p) {} publique vide G() { F(1); }")
            reference = run([projet / "Lib.GsPj"])
            result = run([projet / "Lib.GsPj", "--diagnostics-frontend", frontend])
            check(reference.returncode == result.returncode == 2 and result.stderr == reference.stderr
                  and b"aucune surcharge compatible pour F" in result.stderr and instantane() == avant,
                  "échec de surcharge de projet différent ou anciennes sorties remplacées")
            priorites_projets += 1
            ecrire("A.GsPP", "espace API { publique vide Cible(entier32* p) {} alias Appeler = Cible; publique vide G() { Appeler(1); } }")
            reference = run([projet / "Lib.GsPj"])
            result = run([projet / "Lib.GsPj", "--diagnostics-frontend", frontend])
            check(reference.returncode == result.returncode == 2 and result.stderr == reference.stderr
                  and b"aucune surcharge compatible pour API::Appeler" in result.stderr and instantane() == avant,
                  "refus d'appel d'alias de projet différent ou anciennes sorties remplacées")
            priorites_projets += 1
            ecrire("A.GsPP", "espace Original { publique vide F(entier32* p) {} } espace A { alias F = Original::F; } "
                   "espace B { alias F = Original::F; } " + utilisant_ab + "publique vide G() { F(1); }")
            reference = run([projet / "Lib.GsPj"])
            result = run([projet / "Lib.GsPj", "--diagnostics-frontend", frontend])
            check(reference.returncode == result.returncode == 2 and result.stderr == reference.stderr
                  and b"aucune surcharge compatible pour F" in result.stderr and b"@GsUsing::" not in result.stderr
                  and instantane() == avant, "refus d'alias mixtes de projet différent ou anciennes sorties remplacées")
            priorites_projets += 1
            for texte_indirect, message_indirect in (
                    ("publique vide G(entier32 valeur) { valeur(); }", "la cible d’appel n’est pas un pointeur de fonction"),
                    ("publique vide G(pointeur_fonction<vide(entier32)> operation) { operation(); }", "nombre d’arguments incorrect")):
                ecrire("A.GsPP", texte_indirect)
                reference = run([projet / "Lib.GsPj"])
                result = run([projet / "Lib.GsPj", "--diagnostics-frontend", frontend])
                check(reference.returncode == result.returncode == 2 and result.stderr == reference.stderr
                      and message_indirect.encode("utf-8") in result.stderr and instantane() == avant,
                      "refus d'appel indirect de projet différent ou anciennes sorties remplacées")
                priorites_projets += 1
            ecrire("A.GsPP", "classe C { publique: constructeur(entier32* p) {} }; publique vide G() { C Objet(1); }")
            reference = run([projet / "Lib.GsPj"])
            result = run([projet / "Lib.GsPj", "--diagnostics-frontend", frontend])
            check(reference.returncode == result.returncode == 2 and result.stderr == reference.stderr
                  and b"aucune surcharge compatible pour C::$constructeur" in result.stderr and instantane() == avant,
                  "refus de construction de projet différent ou anciennes sorties remplacées")
            priorites_projets += 1
            ecrire("A.GsPP", source_a.replace("40", "41"))
            ecrire("Principal.GsPP", "classe C { publique: vide M() {} vide F(M* Objet) {} };")
            reference = run([projet / "Solution.GsPs"])
            attendu = instantane()
            check(reference.returncode == 2 and attendu["Lib.GsA"] != avant["Lib.GsA"]
                  and attendu["App.GsE"] == avant["App.GsE"] and attendu["App.map"] == avant["App.map"],
                  "fixture de solution partiellement reconstruite incorrecte")
            for nom, contenu_sortie in avant.items():
                (projet / nom).write_bytes(contenu_sortie)
            result = run([projet / "Solution.GsPs", "--frontend-diagnostics", frontend])
            check(result.returncode == 2 and result.stdout == reference.stdout and result.stderr == reference.stderr
                  and instantane() == attendu, "ordre/arrêt de solution ou état partiellement construit différents")
            priorites_projets += 1
    aide = run(["--help"])
    check(aide.returncode == 0 and b"--diagnostics-frontend" in aide.stdout and b"--frontend-diagnostics" in aide.stdout,
          "options absentes de l'aide")
    print(f"CLI diagnostics frontend : {comparaisons_binaires} comparaisons binaires/jetons/AST, "
          f"{comparaisons_refus} diagnostics FR/EN, {len(options_invalides)} refus options/images, "
          f"{solutions} solutions FR/EN, {remplacements} remplacements et {priorites_projets} refus/arrets de projets ; "
          f"{replis_contextes} replis contextuels, {replis_liaison} replis de liaison, {replis_appels} replis d'appels "
          f"et {replis_groupes} replis de groupes, {replis_echecs} replis d'échecs, {replis_constructions} replis de constructions ; "
          f"{replis_implicites} replis de constructions implicites, {replis_alias} replis d'appels d'alias ; "
          "24 contrôles d'exports facultatifs ; "
          "ancien mode et aide vérifiés.")


if __name__ == "__main__":
    main()
