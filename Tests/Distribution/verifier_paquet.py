"""Validate an extracted Gs++ package using only its distributed tools/assets."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def run(command: list[Path | str], work: Path) -> str:
    result = subprocess.run(
        [str(argument) for argument in command], cwd=work,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        encoding="utf-8", errors="replace", timeout=120,
    )
    require(result.returncode == 0,
            f"Command failed ({result.returncode}): {command}\n{result.stdout}")
    return result.stdout


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package-root", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--reference-frontend", type=Path, required=True)
    parser.add_argument("--auto-test", type=Path, required=True)
    parser.add_argument("--work-root", type=Path, required=True)
    options = parser.parse_args()
    root = options.package_root.resolve()
    source = options.source_root.resolve()
    work = options.work_root.resolve()
    require(root.is_dir(), f"Package root missing: {root}")
    require(root != work and root not in work.parents,
            "Validation outputs must remain outside the extracted package")
    work.mkdir(parents=True, exist_ok=False)
    version = (source / "VERSION").read_text(encoding="utf-8-sig").strip()
    require(re.fullmatch(r"\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?", version)
            is not None, f"Invalid source version: {version}")
    suffix = ".exe" if sys.platform == "win32" else ""
    compiler = root / "bin" / ("gsppc" + suffix)
    verifier = root / "bin" / ("gseverifier" + suffix)
    loader = root / "bin" / ("gsechargeur" + suffix)
    share = root / "share/GsPlusPlus"
    doc = root / "share/doc/GsPlusPlus"
    libraries = root / "lib/GsPlusPlus"
    hosted = share / "Bibliotheques/Hebergee/Hebergee.HGsPP"
    system = share / "Bibliotheques/Systeme/Systeme.HGsPP"
    frontend = libraries / "AutoHebergement/Frontend.GsE"
    headers = [share / f"AutoHebergement/{stage}/{stage}.HGsPP" for stage in (
        "ClassificateurMotsCles", "Lexeur", "AnalyseurDeclarations", "AnalyseurSemantique")]
    checks: list[dict[str, object]] = []

    def passed(name: str, **details: object) -> None:
        checks.append({"name": name, "status": "passed", **details})
        print(f"[OK] {name}", flush=True)

    expected = [compiler, verifier, loader, frontend, hosted, system, *headers,
                libraries / "GsSysteme.GsA", libraries / "GsHebergee.GsA",
                root / "include/GsPP/VersionProduit.hpp", doc / "VERSION",
                doc / "RELEASE_NOTES.md", doc / "Assets/Gs++.png",
                share / "Exemples/Bonjour.Gs++"]
    for path in expected:
        require(path.is_file(), f"Distributed file missing: {path}")
    require((doc / "VERSION").read_text(encoding="utf-8-sig").strip() == version,
            "Distributed VERSION differs from source VERSION")
    for path, original in zip(headers, (
            source / f"AutoHebergement/{stage}/{stage}.HGsPP" for stage in (
                "ClassificateurMotsCles", "Lexeur", "AnalyseurDeclarations", "AnalyseurSemantique"))):
        require(path.read_bytes() == original.read_bytes(),
                f"Stale public frontend interface: {path}")
    passed("distributed-files-and-public-interfaces")

    forbidden_dirs = {".git", ".vs", "construction", "cmakefiles", "__pycache__"}
    for path in root.rglob("*"):
        relative = path.relative_to(root)
        require(not any(part.casefold() in forbidden_dirs for part in relative.parts),
                f"Build/cache directory in package: {relative}")
        require(path.suffix.casefold() not in {".pdb", ".obj", ".o", ".pyc", ".user"},
                f"Native build artifact in package: {relative}")
    passed("no-build-or-cache-artifacts")

    for tool, banner in ((compiler, "Gs++ Compiler"), (loader, "Chargeur GsE"),
                         (root / "bin" / ("gseload" + suffix), "Chargeur GsE")):
        require(run([tool, "--version"], work).strip() == f"{banner} {version}",
                f"Wrong version banner: {tool}")
    require(f'VersionProduit = "{version}"' in
            (root / "include/GsPP/VersionProduit.hpp").read_text(encoding="utf-8-sig"),
            "Stale generated SDK version")
    passed("tool-and-sdk-versions", version=version)

    content = frontend.read_bytes()
    require(content[:8] == b"GSE:0\0\0\0", "Invalid frontend signature")
    require(struct.unpack_from("<HH", content, 8) == (1, 0)
            and struct.unpack_from("<H", content, 20)[0] == 1,
            "Unexpected frontend format or ABI")
    metadata_offset, metadata_size = struct.unpack_from("<QQ", content, 72)
    metadata = content[metadata_offset:metadata_offset + metadata_size].decode("utf-8")
    require(f'Version = "{version}";' in metadata, "Stale frontend metadata")
    verification = run([verifier, frontend], work)
    require("2 import(s), 75 export(s)" in verification, "Frontend export/import drift")
    require(sha256(frontend) == sha256(options.reference_frontend),
            "Distributed frontend differs from the tested build")
    passed("frontend-format-metadata-and-hash", sha256=sha256(frontend), bytes=len(content))

    run([compiler, share / "Exemples/Bonjour.Gs++", "--format", "gsobj",
         "-o", work / "Bonjour.GsObj"], work)
    require((work / "Bonjour.GsObj").read_bytes()[:8] == b"GSOBJ:0\0",
            "Invalid object generated from packaged Bonjour example")
    passed("packaged-bonjour-example")

    client = work / "Interfaces.GsPP"
    client.write_text("publique entier32 Principal() { retourner 42; }\n", encoding="utf-8")
    client_image = work / "Interfaces.GsE"
    run([compiler, hosted, *headers, client, "--format", "gse",
         "--point-entree", "Principal", "-o", client_image], work)
    run([verifier, client_image], work)
    require(re.search(r"Code de retour : 42\s*$", run([loader, client_image, "--executer"], work))
            is not None,
            "Packaged public-interface client did not return 42")
    passed("public-frontend-interface-client")

    fixtures = {
        "fr": (
            "classe C { publique: entier32 X; constructeur() : X(42) {} "
            "entier32 Lire() { retourner soi.X; } }; alias Appeler = C::Lire; "
            "pointeur_fonction<entier32(C&)> Rappel = Appeler; "
            "publique entier32 Principal() { C objet; retourner Rappel(objet); }\n"),
        "en": (
            "class C { public: int32 X; constructor() : X(42) {} "
            "int32 Lire() { return this.X; } }; alias Appeler = C::Lire; "
            "function_pointer<int32(C&)> Rappel = Appeler; "
            "public int32 Principal() { C objet; return Rappel(objet); }\n"),
    }
    for language, text in fixtures.items():
        fixture = work / f"Alias-{language}.GsPP"
        fixture.write_text(text, encoding="utf-8")
        image = work / f"Alias-{language}.GsE"
        run([compiler, fixture, "--format", "gse", "--point-entree", "Principal",
             "-o", image], work)
        run([verifier, image], work)
        require(re.search(r"Code de retour : 42\s*$", run([loader, image, "--executer"], work))
                is not None,
                f"Packaged method-alias callback failed ({language})")
        passed(f"method-alias-execution-{language}")

    system_object = work / "TestSysteme.GsObj"
    system_image = work / "TestSysteme.GsE"
    run([compiler, system, source / "Tests/Integration/BibliothequeSysteme.GsPP",
         "--format", "gsobj", "-o", system_object], work)
    run([compiler, system_object, libraries / "GsSysteme.GsA", "--format", "gse", "--point-entree", "Principal",
         "-o", system_image], work)
    run([verifier, system_image], work)
    require(re.search(r"Code de retour : 64\s*$", run([loader, system_image, "--executer"], work))
            is not None, "Packaged system-library client did not return 64")
    passed("packaged-system-library-client")

    hosted_object = work / "TestHebergee.GsObj"
    hosted_image = work / "TestHebergee.GsE"
    run([compiler, hosted, source / "Tests/AutoHebergement/BibliothequeHebergee.GsPP",
         "--format", "gsobj", "-o", hosted_object], work)
    run([compiler, hosted_object, libraries / "GsHebergee.GsA", "--format", "gse",
         "--point-entree", "TesterBibliothequeHebergee", "-o", hosted_image], work)
    run([verifier, hosted_image], work)
    passed("packaged-hosted-library-client")

    output = run([options.auto_test.resolve(), frontend, hosted_image], work)
    match = re.search(r"Sémantique différentielle : (\d+) corpus refusés", output)
    require(match is not None, "Differential frontend suite did not report its corpus count")
    passed("extracted-frontend-differential-suite", negative_corpora=int(match.group(1)))
    report = {"version": version, "package_root": str(root), "status": "passed", "checks": checks}
    (work / "rapport.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Package validated: {len(checks)}/{len(checks)} checks; {work / 'rapport.json'}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"Package validation failed: {error}", file=sys.stderr)
        raise SystemExit(1)
