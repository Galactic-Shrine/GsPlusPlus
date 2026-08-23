#!/usr/bin/env python3
"""Suite de conformité portable du contrat candidat Gs++ 1.0."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import struct
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Callable
from xml.sax.saxutils import quoteattr


VERSION_GSPP = "0.27.0-alpha.2"
SIGNATURE_ABI = b"GsAbi:x64-ms-v1"


class EchecConformite(RuntimeError):
    """Erreur levée lorsqu’une exigence de conformité n’est pas satisfaite."""


def instant_utc() -> str:
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


def executer(arguments: list[str | Path]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(argument) for argument in arguments],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )


def sortie_complete(resultat: subprocess.CompletedProcess[str]) -> str:
    return (resultat.stdout + "\n" + resultat.stderr).strip()


def exiger(condition: bool, message: str) -> None:
    if not condition:
        raise EchecConformite(message)


def exiger_succes(
    resultat: subprocess.CompletedProcess[str], contexte: str
) -> subprocess.CompletedProcess[str]:
    if resultat.returncode != 0:
        raise EchecConformite(
            f"{contexte} a échoué avec le code {resultat.returncode}:\n"
            f"{sortie_complete(resultat)}"
        )
    return resultat


def exiger_refus(
    resultat: subprocess.CompletedProcess[str], fragment: str, contexte: str
) -> None:
    texte = sortie_complete(resultat)
    exiger(resultat.returncode != 0, f"{contexte} aurait dû échouer")
    exiger(
        fragment.casefold() in texte.casefold(),
        f"{contexte} ne contient pas le diagnostic attendu « {fragment} »:\n{texte}",
    )


def lire_fichier(chemin: Path) -> bytes:
    exiger(chemin.is_file(), f"artefact absent: {chemin}")
    return chemin.read_bytes()


def lire_u16(contenu: bytes, position: int) -> int:
    return struct.unpack_from("<H", contenu, position)[0]


def lire_u32(contenu: bytes, position: int) -> int:
    return struct.unpack_from("<I", contenu, position)[0]


def lire_u64(contenu: bytes, position: int) -> int:
    return struct.unpack_from("<Q", contenu, position)[0]


def empreinte(contenu: bytes) -> str:
    return hashlib.sha256(contenu).hexdigest()


def reinitialiser_repertoire(chemin: Path, parent_autorise: Path) -> None:
    cible = chemin.resolve()
    parent = parent_autorise.resolve()
    exiger(cible != parent, "le répertoire de travail ne peut pas être sa racine")
    exiger(parent in cible.parents, f"répertoire de travail hors racine: {cible}")
    if cible.exists():
        shutil.rmtree(cible)
    cible.mkdir(parents=True)


def arguments_communs() -> argparse.Namespace:
    analyseur = argparse.ArgumentParser(
        description="Vérifier le contrat candidat Gs++ 1.0"
    )
    analyseur.add_argument("--source-root", required=True, type=Path)
    analyseur.add_argument("--build-root", required=True, type=Path)
    analyseur.add_argument("--compiler", required=True, type=Path)
    analyseur.add_argument("--verifier", required=True, type=Path)
    analyseur.add_argument("--loader", required=True, type=Path)
    analyseur.add_argument("--hosted-library", required=True, type=Path)
    analyseur.add_argument("--system-library", required=True, type=Path)
    analyseur.add_argument("--hosted-test", required=True, type=Path)
    analyseur.add_argument("--report", required=True, type=Path)
    return analyseur.parse_args()


def main() -> int:
    options = arguments_communs()
    racine = options.source_root.resolve()
    racine_conformite = racine / "Tests" / "Conformite"
    corpus = racine_conformite / "Corpus"
    manifeste = json.loads(
        (racine_conformite / "conformite.json").read_text(encoding="utf-8")
    )
    exigences = {entree["id"]: entree for entree in manifeste["exigences"]}
    exiger(
        len(exigences) == len(manifeste["exigences"]),
        "le manifeste contient des identifiants dupliqués",
    )

    for outil in (
        options.compiler,
        options.verifier,
        options.loader,
        options.hosted_library,
        options.system_library,
        options.hosted_test,
    ):
        exiger(outil.resolve().is_file(), f"outil absent: {outil}")

    racine_sortie = (
        options.build_root.resolve() / "Tests" / "GsPlusPlus" / "Conformite"
    )
    travail = racine_sortie / "Travail"
    reinitialiser_repertoire(travail, racine_sortie)
    options.report.parent.mkdir(parents=True, exist_ok=True)

    resultats: list[dict[str, Any]] = []
    debut_suite = instant_utc()

    def cas(
        identifiant: str, verification: Callable[[], dict[str, Any] | None]
    ) -> None:
        exiger(identifiant in exigences, f"exigence inconnue: {identifiant}")
        debut = time.perf_counter()
        resultat: dict[str, Any] = {
            "id": identifiant,
            "domaine": exigences[identifiant]["domaine"],
            "titre": exigences[identifiant]["titre"],
        }
        try:
            details = verification() or {}
            resultat.update({"etat": "réussi", "details": details})
        except Exception as erreur:  # conserver tous les échecs dans le rapport
            resultat.update(
                {
                    "etat": "échoué",
                    "erreur": f"{type(erreur).__name__}: {erreur}",
                }
            )
        resultat["duree_ms"] = round((time.perf_counter() - debut) * 1000, 3)
        resultats.append(resultat)

    def compiler_objet(source: Path, sortie: Path) -> subprocess.CompletedProcess[str]:
        return executer(
            [options.compiler, source, "--format", "gsobj", "-o", sortie]
        )

    def creer_objet_canonique() -> Path:
        objet = travail / "Canonique.GsObj"
        exiger_succes(
            compiler_objet(corpus / "Principal.GsPP", objet),
            "compilation du GsObj canonique",
        )
        return objet

    def creer_archive_canonique(objet: Path) -> Path:
        archive = travail / "Canonique.GsA"
        exiger_succes(
            executer(
                [options.compiler, objet, "--format", "gsa", "-o", archive]
            ),
            "création de l’archive GsA canonique",
        )
        return archive

    def creer_image_canonique(objet: Path) -> Path:
        image = travail / "Canonique.GsE"
        exiger_succes(
            executer(
                [
                    options.compiler,
                    objet,
                    "--format",
                    "gse",
                    "--point-entree",
                    "Principal",
                    "--nom",
                    "Conformité Gs++",
                    "--version-application",
                    VERSION_GSPP,
                    "-o",
                    image,
                ]
            ),
            "création de l’image GsE canonique",
        )
        return image

    def verifier_cli() -> dict[str, Any]:
        compilateur = exiger_succes(
            executer([options.compiler, "--version"]), "version du compilateur"
        )
        chargeur = exiger_succes(
            executer([options.loader, "--version"]), "version du chargeur"
        )
        exiger(
            compilateur.stdout.strip() == f"Gs++ Compiler {VERSION_GSPP}",
            f"bannière compilateur inattendue: {compilateur.stdout.strip()}",
        )
        exiger(
            chargeur.stdout.strip() == f"Chargeur GsE {VERSION_GSPP}",
            f"bannière chargeur inattendue: {chargeur.stdout.strip()}",
        )
        return {
            "compilateur": compilateur.stdout.strip(),
            "chargeur": chargeur.stdout.strip(),
        }

    def verifier_extensions_sources() -> dict[str, Any]:
        produites: list[str] = []
        texte = (corpus / "Principal.GsPP").read_text(encoding="utf-8")
        dossier = travail / "ExtensionsSources"
        dossier.mkdir()
        for extension in (".Gs++", ".GsPP", ".GsPlusPlus"):
            source = dossier / f"Principal{extension}"
            suffixe = extension.replace(".", "_").replace("+", "p")
            sortie = dossier / f"Principal{suffixe}.GsObj"
            source.write_text(texte, encoding="utf-8", newline="\n")
            exiger_succes(
                compiler_objet(source, sortie), f"compilation de {extension}"
            )
            lire_fichier(sortie)
            produites.append(extension)
        return {"extensions": produites}

    def verifier_extensions_interfaces() -> dict[str, Any]:
        source_reference = (
            racine
            / "Tests"
            / "Integration"
            / "Separation"
            / "Calculs.HGsPP"
        )
        implementation_reference = source_reference.with_name("Calculs.GsPP")
        texte_interface = source_reference.read_text(encoding="utf-8")
        texte_implementation = implementation_reference.read_text(encoding="utf-8")
        dossier = travail / "ExtensionsInterfaces"
        dossier.mkdir()
        produites: list[str] = []
        for numero, extension in enumerate(
            (".HGs++", ".HGsPP", ".HeaderGsPlusPlus"), start=1
        ):
            sous_dossier = dossier / str(numero)
            sous_dossier.mkdir()
            interface = sous_dossier / f"Calculs{extension}"
            implementation = sous_dossier / "Calculs.GsPP"
            sortie = sous_dossier / "Calculs.GsObj"
            interface.write_text(texte_interface, encoding="utf-8", newline="\n")
            implementation.write_text(
                texte_implementation, encoding="utf-8", newline="\n"
            )
            exiger_succes(
                executer(
                    [
                        options.compiler,
                        interface,
                        implementation,
                        "--format",
                        "gsobj",
                        "-o",
                        sortie,
                    ]
                ),
                f"compilation de l’interface {extension}",
            )
            lire_fichier(sortie)
            produites.append(extension)
        return {"extensions": produites}

    def verifier_gsobj() -> dict[str, Any]:
        contenu = lire_fichier(creer_objet_canonique())
        exiger(len(contenu) >= 112, "GsObj plus petit que son en-tête")
        exiger(contenu[:8] == b"GSOBJ:0\0", "signature GsObj incorrecte")
        exiger(
            (lire_u16(contenu, 8), lire_u16(contenu, 10)) == (1, 0),
            "version GsObj différente de 1.0",
        )
        exiger(lire_u16(contenu, 12) == 0x8664, "architecture GsObj incorrecte")
        exiger(lire_u16(contenu, 14) == 1, "ABI GsObj différente de 1")
        exiger(
            lire_u32(contenu, 16) == 112, "taille d’en-tête GsObj incorrecte"
        )
        exiger(contenu[104:112] == b"\0" * 8, "réserve GsObj non nulle")
        return {"taille": len(contenu), "sha256": empreinte(contenu)}

    def verifier_gsa() -> dict[str, Any]:
        contenu = lire_fichier(creer_archive_canonique(creer_objet_canonique()))
        exiger(len(contenu) >= 32, "GsA plus petit que son en-tête")
        exiger(contenu[:8] == b"GSA:0\0\0\0", "signature GsA incorrecte")
        exiger(
            (lire_u16(contenu, 8), lire_u16(contenu, 10)) == (1, 0),
            "version GsA différente de 1.0",
        )
        exiger(lire_u32(contenu, 12) == 32, "taille d’en-tête GsA incorrecte")
        exiger(
            lire_u32(contenu, 16) == 1,
            "l’archive canonique doit contenir un membre",
        )
        exiger(lire_u16(contenu, 20) == 1, "ABI GsA différente de 1")
        exiger(
            lire_u64(contenu, 24) == len(contenu),
            "taille totale GsA incohérente",
        )
        return {"taille": len(contenu), "sha256": empreinte(contenu)}

    def verifier_gse() -> dict[str, Any]:
        image = creer_image_canonique(creer_objet_canonique())
        contenu = lire_fichier(image)
        exiger(len(contenu) >= 112, "GsE plus petit que son en-tête")
        exiger(contenu[:8] == b"GSE:0\0\0\0", "signature GsE incorrecte")
        exiger(
            (lire_u16(contenu, 8), lire_u16(contenu, 10)) == (1, 0),
            "version GsE différente de 1.0",
        )
        exiger(lire_u16(contenu, 12) == 112, "taille d’en-tête GsE incorrecte")
        exiger(lire_u16(contenu, 14) == 0x8664, "architecture GsE incorrecte")
        exiger(lire_u16(contenu, 20) == 1, "ABI GsE différente de 1")
        verification = exiger_succes(
            executer([options.verifier, image]), "vérification de l’image GsE"
        )
        exiger("GsE valide" in verification.stdout, "validation GsE non confirmée")
        execution = exiger_succes(
            executer([options.loader, image, "--executer"]),
            "exécution de l’image GsE",
        )
        exiger(
            "Code de retour : 24" in execution.stdout,
            "résultat GsE différent de 24",
        )
        return {
            "taille": len(contenu),
            "sha256": empreinte(contenu),
            "retour": 24,
        }

    def verifier_abi() -> dict[str, Any]:
        contenu = lire_fichier(creer_objet_canonique())
        exiger(SIGNATURE_ABI in contenu, "préfixe ABI absent du GsObj")
        return {"prefixe": SIGNATURE_ABI.decode("ascii")}

    def verifier_bilinguisme() -> dict[str, Any]:
        for source, nom, entree in (
            (corpus / "Principal.GsPP", "Francais", "Principal"),
            (corpus / "Main.GsPlusPlus", "English", "Main"),
        ):
            objet = travail / f"{nom}.GsObj"
            image = travail / f"{nom}.GsE"
            exiger_succes(compiler_objet(source, objet), f"compilation {nom}")
            exiger_succes(
                executer(
                    [
                        options.compiler,
                        objet,
                        "--format",
                        "gse",
                        "--point-entree",
                        entree,
                        "--nom",
                        nom,
                        "--version-application",
                        VERSION_GSPP,
                        "-o",
                        image,
                    ]
                ),
                f"édition de liens {nom}",
            )
            execution = exiger_succes(
                executer([options.loader, image, "--execute"]), f"exécution {nom}"
            )
            exiger(
                "Code de retour : 24" in execution.stdout,
                f"résultat {nom} différent de 24",
            )
        return {"francais": 24, "anglais": 24}

    def verifier_initialisation_duree_vie() -> dict[str, Any]:
        source = (
            racine
            / "Tests"
            / "Integration"
            / "InitialisationDureeVie.GsPP"
        )
        image = travail / "InitialisationDureeVie.GsE"
        exiger_succes(
            executer(
                [
                    options.compiler,
                    source,
                    "--format",
                    "gse",
                    "--point-entree",
                    "Principal",
                    "--nom",
                    "Conformité durée de vie Gs++",
                    "--version-application",
                    VERSION_GSPP,
                    "-o",
                    image,
                ]
            ),
            "compilation du scénario de durée de vie",
        )
        verification = exiger_succes(
            executer([options.verifier, image]),
            "vérification du scénario de durée de vie",
        )
        exiger("GsE valide" in verification.stdout, "image de durée de vie invalide")
        execution = exiger_succes(
            executer([options.loader, image, "--executer"]),
            "exécution du scénario de durée de vie",
        )
        exiger(
            "Code de retour : 25" in execution.stdout,
            "résultat du scénario de durée de vie différent de 25",
        )
        return {"retour": 25, "sha256": empreinte(lire_fichier(image))}

    def extraire_imports_hote(contenu: bytes) -> set[str]:
        return {
            nom.decode("ascii")
            for nom in re.findall(rb"Gs::Hote::[A-Za-z]+", contenu)
        }

    def verifier_bibliotheque_hebergee() -> dict[str, Any]:
        archive = lire_fichier(options.hosted_library)
        image = lire_fichier(options.hosted_test)
        imports_attendus = {
            "Gs::Hote::AllouerMemoire",
            "Gs::Hote::LibererMemoire",
            "Gs::Hote::LireFichier",
            "Gs::Hote::EcrireFichier",
            "Gs::Hote::EmettreDiagnostic",
        }
        exiger(archive[:8] == b"GSA:0\0\0\0", "signature de GsHebergee.GsA incorrecte")
        exiger(image[:8] == b"GSE:0\0\0\0", "signature du test hébergé incorrecte")
        exiger(
            extraire_imports_hote(archive) == imports_attendus,
            "GsHebergee.GsA ne porte pas exactement les cinq imports d’hôte",
        )
        exiger(
            extraire_imports_hote(image) == imports_attendus,
            "le test hébergé ne porte pas exactement les cinq imports d’hôte",
        )
        symboles_attendus = (
            b"Gs::Hebergee::ValiderUtf8",
            b"Gs::Hebergee::InitialiserChaineUtf8",
            b"Gs::Hebergee::InitialiserArene",
            b"Gs::Hebergee::InitialiserVecteurOctetsDynamique",
            b"Gs::Hebergee::InitialiserTableSymbolesDynamique",
            b"Gs::Hebergee::JoindreCheminUtf8",
            b"Gs::Hebergee::ChargerFichierAlloue",
        )
        absents = [
            symbole.decode("ascii")
            for symbole in symboles_attendus
            if symbole not in archive
        ]
        exiger(not absents, f"symboles hébergés absents: {', '.join(absents)}")
        verification = exiger_succes(
            executer([options.verifier, options.hosted_test]),
            "vérification de l’image de test hébergée",
        )
        exiger("GsE valide" in verification.stdout, "image hébergée non validée")
        return {
            "imports": sorted(imports_attendus),
            "symboles_verifies": len(symboles_attendus),
            "sha256_gsa": empreinte(archive),
            "sha256_gse": empreinte(image),
        }

    def verifier_independance_systeme() -> dict[str, Any]:
        archive = lire_fichier(options.system_library)
        exiger(archive[:8] == b"GSA:0\0\0\0", "signature de GsSysteme.GsA incorrecte")
        imports = extraire_imports_hote(archive)
        exiger(
            not imports and b"Gs::Hote::" not in archive,
            "GsSysteme.GsA dépend d’un service réservé au profil hébergé",
        )
        return {
            "imports_hote": 0,
            "sha256_gsa": empreinte(archive),
        }

    def verifier_reproductibilite() -> dict[str, Any]:
        source = corpus / "Principal.GsPP"
        objets = [
            travail / "Determinisme1.GsObj",
            travail / "Determinisme2.GsObj",
        ]
        for objet in objets:
            exiger_succes(
                compiler_objet(source, objet), "compilation reproductible GsObj"
            )
        donnees_objets = [lire_fichier(chemin) for chemin in objets]
        exiger(donnees_objets[0] == donnees_objets[1], "GsObj non reproductible")

        archives = [travail / "Determinisme1.GsA", travail / "Determinisme2.GsA"]
        for archive in archives:
            exiger_succes(
                executer(
                    [options.compiler, objets[0], "--format", "gsa", "-o", archive]
                ),
                "création reproductible GsA",
            )
        donnees_archives = [lire_fichier(chemin) for chemin in archives]
        exiger(donnees_archives[0] == donnees_archives[1], "GsA non reproductible")

        images = [travail / "Determinisme1.GsE", travail / "Determinisme2.GsE"]
        for image in images:
            exiger_succes(
                executer(
                    [
                        options.compiler,
                        objets[0],
                        "--format",
                        "gse",
                        "--point-entree",
                        "Principal",
                        "--nom",
                        "Déterminisme",
                        "--version-application",
                        VERSION_GSPP,
                        "-o",
                        image,
                    ]
                ),
                "création reproductible GsE",
            )
        donnees_images = [lire_fichier(chemin) for chemin in images]
        exiger(donnees_images[0] == donnees_images[1], "GsE non reproductible")
        return {
            "GsObj": empreinte(donnees_objets[0]),
            "GsA": empreinte(donnees_archives[0]),
            "GsE": empreinte(donnees_images[0]),
        }

    def refuser_gso() -> dict[str, Any]:
        resultat = compiler_objet(
            corpus / "Principal.GsPP", travail / "Ancien.GsO"
        )
        exiger_refus(
            resultat, "extension de sortie obsolète refusée", "sortie .GsO"
        )
        return {"extension": ".GsO"}

    def refuser_interface_obsolete() -> dict[str, Any]:
        reference = (
            racine
            / "Tests"
            / "Integration"
            / "Separation"
            / "Calculs.HGsPP"
        )
        ancienne = travail / "Ancienne.GsPPH"
        ancienne.write_text(
            reference.read_text(encoding="utf-8"), encoding="utf-8", newline="\n"
        )
        resultat = executer(
            [
                options.compiler,
                ancienne,
                "--format",
                "gsobj",
                "-o",
                travail / "Ancienne.GsObj",
            ]
        )
        exiger_refus(resultat, "obsolète", "interface .GsPPH")
        return {"extension": ".GsPPH"}

    def refuser_gssharp() -> dict[str, Any]:
        resultat = compiler_objet(
            corpus / "Reservee.Gs#", travail / "Reservee.GsObj"
        )
        exiger_refus(resultat, "extension réservée à Gs#", "source .Gs#")
        return {"extension": ".Gs#", "routage": "hors de gsppc"}

    def refuser_gse_tronque() -> dict[str, Any]:
        contenu = lire_fichier(creer_image_canonique(creer_objet_canonique()))
        tronque = travail / "Tronque.GsE"
        tronque.write_bytes(contenu[:31])
        resultat = executer([options.verifier, tronque])
        exiger_refus(resultat, "GsE invalide", "image GsE tronquée")
        return {"taille": 31}

    def refuser_objet_classe_global() -> dict[str, Any]:
        source = travail / "ObjetClasseGlobal.GsPP"
        source.write_text(
            """classe ObjetGlobal
{
    publique: constructeur() {}
};
ObjetGlobal Globale;
publique entier32 Principal() { retourner 0; }
""",
            encoding="utf-8",
            newline="\n",
        )
        resultat = compiler_objet(source, travail / "ObjetClasseGlobal.GsObj")
        exiger_refus(
            resultat,
            "objets classes globaux",
            "objet de classe global",
        )
        return {"type": "classe", "portee": "globale"}

    def refuser_cycle_delegation() -> dict[str, Any]:
        source = travail / "CycleDelegation.GsPP"
        source.write_text(
            """classe Cycle
{
    publique:
        constructeur() : soi(1) {}
        constructeur(entier32 valeur) : soi() {}
};
publique entier32 Principal() { retourner 0; }
""",
            encoding="utf-8",
            newline="\n",
        )
        resultat = compiler_objet(source, travail / "CycleDelegation.GsObj")
        exiger_refus(
            resultat,
            "cycle de délégation",
            "cycle de délégation entre constructeurs",
        )
        return {"cycle": "constructeur() -> constructeur(entier32) -> constructeur()"}

    def verifier_projets_xml() -> dict[str, Any]:
        dossier = travail / "ProjetsXml"
        dossier.mkdir()
        sources = racine / "Tests" / "Integration" / "Separation"
        archive = dossier / "Calculs.GsA"
        image = dossier / "Application.GsE"
        carte = dossier / "Application.map"
        projet_bibliotheque = dossier / "Bibliotheque.GsPj"
        projet_application = dossier / "Application.GsProject"
        solution = dossier / "Compilation.GsPs"

        projet_bibliotheque.write_text(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            '<GsProjet Version="1.0" Nom="Calculs" Type="bibliotheque">\n'
            f"    <Interface Chemin={quoteattr(str(sources / 'Calculs.HGsPP'))} />\n"
            f"    <Source Chemin={quoteattr(str(sources / 'Calculs.GsPP'))} />\n"
            "    <Construction\n"
            f"        RepertoireObjets={quoteattr(str(dossier / 'ObjetsBibliotheque'))}\n"
            f"        Sortie={quoteattr(str(archive))} />\n"
            "</GsProjet>\n",
            encoding="utf-8",
            newline="\n",
        )
        projet_application.write_text(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            '<GsProject Version="1.0" Name="Application" Type="executable"\n'
            '    EntryPoint="Essai::Separation::Principal"\n'
            f"    ApplicationVersion={quoteattr(VERSION_GSPP)}>\n"
            f"    <Interface Path={quoteattr(str(sources / 'Calculs.HeaderGsPlusPlus'))} />\n"
            f"    <Source Path={quoteattr(str(sources / 'Principal.GsPP'))} />\n"
            f"    <Library Path={quoteattr(str(archive))} />\n"
            "    <Build\n"
            f"        ObjectDirectory={quoteattr(str(dossier / 'ObjetsApplication'))}\n"
            f"        Output={quoteattr(str(image))}\n"
            f"        Map={quoteattr(str(carte))} />\n"
            "</GsProject>\n",
            encoding="utf-8",
            newline="\n",
        )
        solution.write_text(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            '<GsSolution Version="1.0">\n'
            '    <Projet Chemin="Bibliotheque.GsPj" />\n'
            '    <Project Path="Application.GsProject" />\n'
            "</GsSolution>\n",
            encoding="utf-8",
            newline="\n",
        )

        exiger_succes(executer([options.compiler, solution]), "solution XML")
        execution = exiger_succes(
            executer([options.loader, image, "--execute"]),
            "exécution de l’application XML",
        )
        exiger(
            "Code de retour : 44" in execution.stdout,
            "l’application issue de la solution XML ne retourne pas 44",
        )
        exiger(archive.is_file(), "la bibliothèque XML n’a pas été produite")
        exiger(carte.is_file(), "la carte XML n’a pas été produite")
        return {
            "projet_francais": projet_bibliotheque.name,
            "projet_anglais": projet_application.name,
            "solution": solution.name,
            "code_retour": 44,
        }

    def refuser_ancien_projet_texte() -> dict[str, Any]:
        ancien = travail / "Ancien.GsPj"
        ancien.write_text(
            "GsProjet 1\n"
            "nom = Ancien\n"
            "type = executable\n"
            "source = Principal.GsPP\n",
            encoding="utf-8",
            newline="\n",
        )
        resultat = executer([options.compiler, ancien])
        exiger_refus(
            resultat,
            "élément XML attendu",
            "ancien format texte GsPj",
        )
        return {"format_refuse": "cle = valeur"}

    cas("CONF-CLI-001", verifier_cli)
    cas("CONF-EXT-001", verifier_extensions_sources)
    cas("CONF-EXT-002", verifier_extensions_interfaces)
    cas("CONF-FMT-001", verifier_gsobj)
    cas("CONF-FMT-002", verifier_gsa)
    cas("CONF-FMT-003", verifier_gse)
    cas("CONF-ABI-001", verifier_abi)
    cas("CONF-LANG-001", verifier_bilinguisme)
    cas("CONF-LIFE-001", verifier_initialisation_duree_vie)
    cas("CONF-HOST-001", verifier_bibliotheque_hebergee)
    cas("CONF-HOST-002", verifier_independance_systeme)
    cas("CONF-DET-001", verifier_reproductibilite)
    cas("CONF-PROJ-001", verifier_projets_xml)
    cas("CONF-NEG-001", refuser_gso)
    cas("CONF-NEG-002", refuser_interface_obsolete)
    cas("CONF-NEG-003", refuser_gssharp)
    cas("CONF-NEG-004", refuser_gse_tronque)
    cas("CONF-NEG-005", refuser_objet_classe_global)
    cas("CONF-NEG-006", refuser_cycle_delegation)
    cas("CONF-NEG-007", refuser_ancien_projet_texte)

    executees = {resultat["id"] for resultat in resultats}
    absentes = sorted(set(exigences) - executees)
    exiger(not absentes, f"exigences non exécutées: {', '.join(absentes)}")
    nombre_echecs = sum(resultat["etat"] != "réussi" for resultat in resultats)
    rapport = {
        "schema": "GsPlusPlus.RapportConformite:1",
        "cible": manifeste["cible"],
        "producteur": manifeste["producteur"],
        "debut_utc": debut_suite,
        "fin_utc": instant_utc(),
        "etat": "réussi" if nombre_echecs == 0 else "échoué",
        "resume": {
            "total": len(resultats),
            "reussis": len(resultats) - nombre_echecs,
            "echoues": nombre_echecs,
        },
        "outils": {
            "compilateur": str(options.compiler.resolve()),
            "verificateur": str(options.verifier.resolve()),
            "chargeur": str(options.loader.resolve()),
        },
        "cas": resultats,
    }
    options.report.write_text(
        json.dumps(rapport, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )

    for resultat in resultats:
        symbole = "OK" if resultat["etat"] == "réussi" else "ECHEC"
        print(f"[{symbole}] {resultat['id']} — {resultat['titre']}")
        if "erreur" in resultat:
            print(f"        {resultat['erreur']}")
    print(
        f"Conformité Gs++: {len(resultats) - nombre_echecs}/{len(resultats)} "
        f"cas réussis. Rapport: {options.report.resolve()}"
    )
    return 0 if nombre_echecs == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
