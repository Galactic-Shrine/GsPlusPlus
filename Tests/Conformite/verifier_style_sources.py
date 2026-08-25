#!/usr/bin/env python3
"""Vérifie les conventions mécaniques des sources Gs++ livrées."""

from __future__ import annotations

import argparse
import re
from pathlib import Path


EXTENSIONS_GSPP = {".GsPP", ".HGsPP"}
REPERTOIRES_LIVRES = ("AutoHebergement", "Bibliotheques")


def analyser_fichier(chemin: Path, racine: Path) -> list[str]:
    texte = chemin.read_text(encoding="utf-8")
    relatif = chemin.relative_to(racine).as_posix()
    erreurs: list[str] = []

    for numero, ligne in enumerate(texte.splitlines(), start=1):
        if "\t" in ligne:
            erreurs.append(f"{relatif}:{numero}: tabulation interdite")
        if ligne.strip() == "{":
            erreurs.append(
                f"{relatif}:{numero}: accolade ouvrante isolée"
            )
        if re.match(r"^\s*(?:espace|namespace)\s+GsPP(?:::|\s*\{)", ligne):
            erreurs.append(
                f"{relatif}:{numero}: préfixe public GsPP:: non canonique"
            )
        if re.match(r"^\s*/\*(?!\*)", ligne):
            erreurs.append(
                f"{relatif}:{numero}: commentaire multiligne sans /**"
            )
        if re.match(r"^\s*\*/\s*$", ligne):
            erreurs.append(
                f"{relatif}:{numero}: commentaire multiligne sans **/"
            )
        if "@Paramètre(" in ligne and not re.search(
            r"@Paramètre\([^:()]+: [^()]+\)", ligne
        ):
            erreurs.append(
                f"{relatif}:{numero}: paramètre attendu sous la forme "
                "@Paramètre(type: nom)"
            )
        if "@Retourner" in ligne and not re.search(
            r"@Retourner\([^()]+\)", ligne
        ):
            erreurs.append(
                f"{relatif}:{numero}: retour attendu sous la forme "
                "@Retourner(type)"
            )

    return erreurs


def verifier(racine: Path) -> list[str]:
    erreurs: list[str] = []

    for repertoire in REPERTOIRES_LIVRES:
        base = racine / repertoire
        if not base.is_dir():
            erreurs.append(f"répertoire livré absent : {repertoire}")
            continue

        for chemin in sorted(base.rglob("*")):
            if chemin.is_file() and chemin.suffix in EXTENSIONS_GSPP:
                erreurs.extend(analyser_fichier(chemin, racine))

    return erreurs


def principal() -> int:
    analyseur = argparse.ArgumentParser()
    analyseur.add_argument("--source-root", type=Path, required=True)
    options = analyseur.parse_args()
    racine = options.source_root.resolve()

    erreurs = verifier(racine)
    if erreurs:
        print("ÉCHEC : conventions de code Gs++ non respectées")
        for erreur in erreurs:
            print(f"- {erreur}")
        return 1

    print(
        "SUCCÈS : espaces de noms, accolades, indentation et "
        "commentaires Gs++ conformes"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(principal())
