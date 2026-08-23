#!/usr/bin/env python3
"""Banc de mesure reproductible pour Gs++ 0.27.0-alpha.1.

Le pilote n'invente ni syntaxe ni option du compilateur : tous les corpus et
les lignes de commande proviennent des tests d'intégration du dépôt.
"""

from __future__ import annotations

import argparse
import csv
import ctypes
import hashlib
import json
import math
import os
import platform
import random
import shutil
import statistics
import subprocess
import sys
import threading
import time
import uuid
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, Sequence
from xml.sax.saxutils import quoteattr


SCHEMA_VERSION = 1
EXPECTED_COMPILER_VERSION = "0.27.0-alpha.1"
EXPECTED_COMPILER_BANNER = "Gs++ Compiler 0.27.0-alpha.1"
EXPECTED_LOADER_BANNER = "Chargeur GsE 0.27.0-alpha.1"
BENCHMARK_VERSION = "1.0.0"
DEFAULT_BOOTSTRAP_SAMPLES = 2_000

MAGIC_GSOBJ = b"GSOBJ:0\x00"
MAGIC_GSA = b"GSA:0\x00\x00\x00"
MAGIC_GSE = b"GSE:0\x00\x00\x00"


class BenchmarkError(RuntimeError):
    """Erreur explicite du banc de mesure."""


@dataclass(frozen=True)
class Stage:
    name: str
    command: tuple[str, ...]
    expected_text: str | None = None


@dataclass
class PreparedScenario:
    scenario_id: str
    size: str
    description: str
    interface_mode: str
    inputs: list[Path]
    artifacts: list[Path]
    full_stages: list[Stage]
    incremental_stages: dict[str, list[Stage]]
    mutation_targets: dict[str, Path]
    expected_magic: dict[Path, bytes]


@dataclass
class ProcessMeasurement:
    exit_code: int
    elapsed_ns: int
    peak_rss_bytes: int | None
    stdout: str
    stderr: str


@dataclass
class RunContext:
    source_root: Path
    compiler: Path
    loader: Path
    session_dir: Path
    affinity: tuple[int, ...]
    keep_work: bool
    records: list[dict] = field(default_factory=list)


SCENARIO_CATALOG = {
    "s_source_gsobj": {
        "size": "S",
        "description": "Source unique Bonjour vers GsObj, sans interface.",
        "conditions": ("cold_artifacts", "warm_artifacts", "leaf_edit"),
    },
    "m_monolithic_gse": {
        "size": "M",
        "description": "Tableaux d'objets classes, compilation GsE monolithique.",
        "conditions": ("cold_artifacts", "warm_artifacts", "leaf_edit"),
    },
    "m_separate_gse": {
        "size": "M",
        "description": "Compilation séparée avec interface, deux GsObj puis GsE.",
        "conditions": (
            "cold_artifacts",
            "warm_artifacts",
            "leaf_edit",
            "interface_edit",
            "central_edit",
        ),
    },
    "l_system_library": {
        "size": "L",
        "description": "Bibliothèque système multifichier avec interface, construite par GsPj.",
        "conditions": (
            "cold_artifacts",
            "warm_artifacts",
            "leaf_edit",
            "interface_edit",
            "central_edit",
        ),
    },
}


def parse_arguments(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Banc de mesure Gs++ 0.27.0-alpha.1 fondé sur les corpus validés.",
    )
    parser.add_argument(
        "--source-root",
        type=Path,
        help="Racine du dépôt Gs++. Par défaut, elle est déduite de ce script.",
    )
    parser.add_argument("--compiler", type=Path, help="Chemin de gsppc 0.27.0-alpha.1.")
    parser.add_argument("--loader", type=Path, help="Chemin de gsechargeur 0.27.0-alpha.1.")
    parser.add_argument(
        "--output-root",
        type=Path,
        help="Dossier parent des sessions (aucun contenu existant n'est supprimé).",
    )
    parser.add_argument(
        "--mode",
        choices=("smoke", "pilot", "full"),
        default="smoke",
        help="smoke=1 mesure froide, pilot=5 mesures, full=30 mesures.",
    )
    parser.add_argument(
        "--scenario",
        action="append",
        choices=tuple(SCENARIO_CATALOG),
        help="Scénario à exécuter. Répétable. Tous par défaut.",
    )
    parser.add_argument(
        "--condition",
        action="append",
        choices=(
            "cold_artifacts",
            "warm_artifacts",
            "leaf_edit",
            "interface_edit",
            "central_edit",
        ),
        help="Condition à conserver. Répétable.",
    )
    parser.add_argument("--repetitions", type=int, help="Remplace le nombre de mesures du mode.")
    parser.add_argument("--warmups", type=int, help="Remplace le nombre d'échauffements du mode.")
    parser.add_argument(
        "--cpu",
        help="Liste de processeurs logiques, par exemple 2 ou 2,3. Facultatif.",
    )
    parser.add_argument(
        "--keep-work",
        action="store_true",
        help="Conserver les copies de travail et leurs artefacts.",
    )
    parser.add_argument(
        "--allow-version-mismatch",
        action="store_true",
        help="Autoriser un compilateur ne déclarant pas 0.27.0-alpha.1 (signalé dans les métadonnées).",
    )
    parser.add_argument(
        "--list-scenarios",
        action="store_true",
        help="Afficher le catalogue et quitter.",
    )
    return parser.parse_args(argv)


def resolve_source_root(argument: Path | None) -> Path:
    if argument is not None:
        root = argument.resolve()
    else:
        root = Path(__file__).resolve().parents[1]
    if not (root / "VERSION").is_file():
        raise BenchmarkError(f"Racine Gs++ invalide : {root}")
    return root


def default_tool(source_root: Path, name: str) -> Path:
    suffix = ".exe" if os.name == "nt" else ""
    toolchain = "VisualStudio" if os.name == "nt" else "Ninja"
    return (
        source_root
        / "Construction"
        / toolchain
        / "Release"
        / "Bin"
        / f"{name}{suffix}"
    )


def resolve_tool(argument: Path | None, source_root: Path, name: str) -> Path:
    candidate = (argument or default_tool(source_root, name)).resolve()
    if not candidate.is_file():
        raise BenchmarkError(f"Outil introuvable : {candidate}")
    return candidate


def parse_affinity(value: str | None) -> tuple[int, ...]:
    if not value:
        return ()
    try:
        cpus = tuple(sorted({int(part.strip()) for part in value.split(",")}))
    except ValueError as error:
        raise BenchmarkError("--cpu attend une liste d'entiers séparés par des virgules.") from error
    if not cpus or cpus[0] < 0:
        raise BenchmarkError("Les indices CPU doivent être positifs ou nuls.")
    logical_count = os.cpu_count()
    if logical_count is not None and cpus[-1] >= logical_count:
        raise BenchmarkError(
            f"CPU logique {cpus[-1]} hors plage ; la machine en expose {logical_count}."
        )
    if os.name == "nt" and cpus[-1] >= 64:
        raise BenchmarkError("L'affinité Windows de ce pilote est limitée au premier groupe de 64 CPU.")
    return cpus


def tool_version(tool: Path) -> str:
    completed = subprocess.run(
        [str(tool), "--version"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
        encoding="utf-8",
        errors="replace",
    )
    if completed.returncode != 0:
        raise BenchmarkError(f"{tool} --version a échoué avec le code {completed.returncode}.")
    return completed.stdout.strip()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def count_lines(path: Path) -> int:
    with path.open("rb") as stream:
        return sum(1 for _ in stream)


def copy_fixture(source: Path, destination: Path) -> Path:
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    return destination


def stage(name: str, *command: str | Path, expected_text: str | None = None) -> Stage:
    return Stage(name, tuple(str(item) for item in command), expected_text)


def prepare_scenario(
    scenario_id: str,
    work_dir: Path,
    source_root: Path,
    compiler: Path,
    loader: Path,
) -> PreparedScenario:
    gspp_root = source_root
    work_dir.mkdir(parents=True, exist_ok=False)

    if scenario_id == "s_source_gsobj":
        source = copy_fixture(gspp_root / "Exemples" / "Bonjour.Gs++", work_dir / "Bonjour.Gs++")
        output = work_dir / "Bonjour.GsObj"
        compile_stage = stage(
            "compile_gsobj", compiler, source, "--format", "gsobj", "-o", output
        )
        return PreparedScenario(
            scenario_id,
            "S",
            SCENARIO_CATALOG[scenario_id]["description"],
            "sans_interface",
            [source],
            [output],
            [compile_stage],
            {"leaf_edit": [compile_stage]},
            {"leaf_edit": source},
            {output: MAGIC_GSOBJ},
        )

    if scenario_id == "m_monolithic_gse":
        source = copy_fixture(
            gspp_root / "Tests" / "Integration" / "TableauxObjetsClasses.GsPP",
            work_dir / "TableauxObjetsClasses.GsPP",
        )
        output = work_dir / "TableauxObjetsClasses.GsE"
        compile_stage = stage(
            "compile_gse",
            compiler,
            source,
            "--format",
            "gse",
            "--point-entree",
            "Principal",
            "--nom",
            "Benchmark monolithique Gs++",
            "--version-application",
            EXPECTED_COMPILER_VERSION,
            "-o",
            output,
        )
        execute_stage = stage(
            "execute_gse", loader, output, "--executer", expected_text="Code de retour : 10"
        )
        full = [compile_stage, execute_stage]
        return PreparedScenario(
            scenario_id,
            "M",
            SCENARIO_CATALOG[scenario_id]["description"],
            "sans_interface",
            [source],
            [output],
            full,
            {"leaf_edit": full},
            {"leaf_edit": source},
            {output: MAGIC_GSE},
        )

    if scenario_id == "m_separate_gse":
        source_root = gspp_root / "Tests" / "Integration" / "Separation"
        interface = copy_fixture(
            source_root / "TableauxObjetsClasses.HGsPP",
            work_dir / "TableauxObjetsClasses.HGsPP",
        )
        implementation = copy_fixture(
            source_root / "TableauxObjetsClassesImplementation.GsPP",
            work_dir / "TableauxObjetsClassesImplementation.GsPP",
        )
        principal = copy_fixture(
            source_root / "TableauxObjetsClassesPrincipal.GsPP",
            work_dir / "TableauxObjetsClassesPrincipal.GsPP",
        )
        implementation_object = work_dir / "Implementation.GsObj"
        principal_object = work_dir / "Principal.GsObj"
        output = work_dir / "TableauxObjetsClassesSepares.GsE"

        compile_implementation = stage(
            "compile_implementation_gsobj",
            compiler,
            implementation,
            "--format",
            "gsobj",
            "-o",
            implementation_object,
        )
        compile_principal = stage(
            "compile_interface_principal_gsobj",
            compiler,
            interface,
            principal,
            "--format",
            "gsobj",
            "-o",
            principal_object,
        )
        link = stage(
            "link_gse",
            compiler,
            implementation_object,
            principal_object,
            "--format",
            "gse",
            "--point-entree",
            "PrincipalTableauSepare",
            "--nom",
            "Benchmark séparation Gs++",
            "--version-application",
            EXPECTED_COMPILER_VERSION,
            "-o",
            output,
        )
        execute = stage(
            "execute_gse", loader, output, "--executer", expected_text="Code de retour : 10"
        )
        full = [compile_implementation, compile_principal, link, execute]
        return PreparedScenario(
            scenario_id,
            "M",
            SCENARIO_CATALOG[scenario_id]["description"],
            "avec_interface",
            [interface, implementation, principal],
            [implementation_object, principal_object, output],
            full,
            {
                "leaf_edit": [compile_principal, link, execute],
                "interface_edit": [compile_principal, link, execute],
                "central_edit": [compile_implementation, link, execute],
            },
            {
                "leaf_edit": principal,
                "interface_edit": interface,
                "central_edit": implementation,
            },
            {
                implementation_object: MAGIC_GSOBJ,
                principal_object: MAGIC_GSOBJ,
                output: MAGIC_GSE,
            },
        )

    if scenario_id == "l_system_library":
        source_root = gspp_root / "Bibliotheques" / "Systeme"
        names = (
            "Systeme.HGsPP",
            "Memoire.GsPP",
            "Vues.GsPP",
            "Bits.GsPP",
            "Atomiques.GsPP",
        )
        copied = {name: copy_fixture(source_root / name, work_dir / name) for name in names}
        object_dir = work_dir / "Objets"
        output = work_dir / "GsSysteme.GsA"
        project = work_dir / "GsSystemeBenchmark.GsPj"
        project.write_text(
            "\n".join(
                [
                    '<?xml version="1.0" encoding="UTF-8"?>',
                    '<GsProjet Version="1.0" Nom="GsSystemeBenchmark" Type="bibliotheque">',
                    f'    <Interface Chemin={quoteattr(copied["Systeme.HGsPP"].as_posix())} />',
                    f'    <Source Chemin={quoteattr(copied["Memoire.GsPP"].as_posix())} />',
                    f'    <Source Chemin={quoteattr(copied["Vues.GsPP"].as_posix())} />',
                    f'    <Source Chemin={quoteattr(copied["Bits.GsPP"].as_posix())} />',
                    f'    <Source Chemin={quoteattr(copied["Atomiques.GsPP"].as_posix())} />',
                    "    <Construction",
                    f'        RepertoireObjets={quoteattr(object_dir.as_posix())}',
                    f'        Sortie={quoteattr(output.as_posix())} />',
                    "</GsProjet>",
                ]
            ),
            encoding="utf-8",
        )
        build = stage("build_gspj_library", compiler, project)
        return PreparedScenario(
            scenario_id,
            "L",
            SCENARIO_CATALOG[scenario_id]["description"],
            "avec_interface",
            [copied[name] for name in names] + [project],
            [output],
            [build],
            {
                "leaf_edit": [build],
                "interface_edit": [build],
                "central_edit": [build],
            },
            {
                "leaf_edit": copied["Atomiques.GsPP"],
                "interface_edit": copied["Systeme.HGsPP"],
                "central_edit": copied["Memoire.GsPP"],
            },
            {output: MAGIC_GSA},
        )

    raise BenchmarkError(f"Scénario inconnu : {scenario_id}")


def append_neutral_change(path: Path, condition: str, iteration: int) -> None:
    with path.open("a", encoding="utf-8", newline="\n") as stream:
        stream.write(f"\n// Benchmark {condition}, répétition {iteration}.\n")


def set_process_affinity(pid: int, cpus: tuple[int, ...]) -> None:
    if not cpus:
        return
    if os.name != "nt":
        os.sched_setaffinity(pid, set(cpus))
        return
    from ctypes import wintypes

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel32.OpenProcess.restype = wintypes.HANDLE
    kernel32.SetProcessAffinityMask.argtypes = [wintypes.HANDLE, ctypes.c_size_t]
    kernel32.SetProcessAffinityMask.restype = wintypes.BOOL
    kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel32.CloseHandle.restype = wintypes.BOOL

    process_set_information = 0x0200
    handle = kernel32.OpenProcess(process_set_information, False, pid)
    if not handle:
        raise BenchmarkError(f"Impossible d'ouvrir le processus {pid} pour son affinité.")
    try:
        mask = sum(1 << cpu for cpu in cpus)
        if not kernel32.SetProcessAffinityMask(handle, mask):
            raise BenchmarkError(f"Impossible d'appliquer l'affinité CPU {cpus}.")
    finally:
        kernel32.CloseHandle(handle)


def windows_peak_rss_sampler(pid: int, process: subprocess.Popen[bytes]) -> int | None:
    from ctypes import wintypes

    class ProcessMemoryCounters(ctypes.Structure):
        _fields_ = [
            ("cb", ctypes.c_ulong),
            ("PageFaultCount", ctypes.c_ulong),
            ("PeakWorkingSetSize", ctypes.c_size_t),
            ("WorkingSetSize", ctypes.c_size_t),
            ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
            ("QuotaPagedPoolUsage", ctypes.c_size_t),
            ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
            ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
            ("PagefileUsage", ctypes.c_size_t),
            ("PeakPagefileUsage", ctypes.c_size_t),
        ]

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    psapi = ctypes.WinDLL("psapi", use_last_error=True)
    kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel32.OpenProcess.restype = wintypes.HANDLE
    kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel32.CloseHandle.restype = wintypes.BOOL
    psapi.GetProcessMemoryInfo.argtypes = [
        wintypes.HANDLE,
        ctypes.POINTER(ProcessMemoryCounters),
        wintypes.DWORD,
    ]
    psapi.GetProcessMemoryInfo.restype = wintypes.BOOL

    query_information = 0x0400
    vm_read = 0x0010
    handle = kernel32.OpenProcess(query_information | vm_read, False, pid)
    if not handle:
        return None
    peak = 0
    counters = ProcessMemoryCounters()
    counters.cb = ctypes.sizeof(counters)
    try:
        while process.poll() is None:
            if psapi.GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb):
                peak = max(peak, int(counters.PeakWorkingSetSize))
            time.sleep(0.001)
        if psapi.GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb):
            peak = max(peak, int(counters.PeakWorkingSetSize))
    finally:
        kernel32.CloseHandle(handle)
    return peak or None


def run_process(command: Sequence[str], cwd: Path, affinity: tuple[int, ...]) -> ProcessMeasurement:
    effective_command = list(command)
    time_file: Path | None = None
    if os.name != "nt" and Path("/usr/bin/time").is_file():
        time_file = cwd / f".time-{uuid.uuid4().hex}.txt"
        effective_command = [
            "/usr/bin/time",
            "-f",
            "%M",
            "-o",
            str(time_file),
            "--",
            *effective_command,
        ]

    start_ns = time.perf_counter_ns()
    process = subprocess.Popen(
        effective_command,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    set_process_affinity(process.pid, affinity)

    peak_holder: dict[str, int | None] = {"value": None}
    sampler: threading.Thread | None = None
    if os.name == "nt":
        sampler = threading.Thread(
            target=lambda: peak_holder.update(value=windows_peak_rss_sampler(process.pid, process)),
            daemon=True,
        )
        sampler.start()

    stdout_bytes, stderr_bytes = process.communicate()
    if sampler is not None:
        sampler.join()
    elapsed_ns = time.perf_counter_ns() - start_ns

    if time_file is not None:
        try:
            maximum_kib = int(time_file.read_text(encoding="ascii").strip())
            peak_holder["value"] = maximum_kib * 1024
        except (OSError, ValueError):
            peak_holder["value"] = None
        finally:
            time_file.unlink(missing_ok=True)

    return ProcessMeasurement(
        exit_code=process.returncode,
        elapsed_ns=elapsed_ns,
        peak_rss_bytes=peak_holder["value"],
        stdout=stdout_bytes.decode("utf-8", errors="replace"),
        stderr=stderr_bytes.decode("utf-8", errors="replace"),
    )


def validate_magic(expected_magic: dict[Path, bytes]) -> None:
    for artifact, expected in expected_magic.items():
        if not artifact.is_file():
            raise BenchmarkError(f"Artefact attendu absent : {artifact}")
        with artifact.open("rb") as stream:
            actual = stream.read(len(expected))
        if actual != expected:
            raise BenchmarkError(
                f"Signature invalide pour {artifact.name}: {actual!r}, attendu {expected!r}."
            )


def run_stages(
    stages: Iterable[Stage],
    prepared: PreparedScenario,
    condition: str,
    iteration: int,
    warmup: bool,
    logs_dir: Path,
    affinity: tuple[int, ...],
) -> list[dict]:
    records: list[dict] = []
    logs_dir.mkdir(parents=True, exist_ok=True)
    for stage_index, current_stage in enumerate(stages, start=1):
        measurement = run_process(current_stage.command, prepared.inputs[0].parent, affinity)
        stdout_path = logs_dir / f"{stage_index:02d}-{current_stage.name}.stdout.txt"
        stderr_path = logs_dir / f"{stage_index:02d}-{current_stage.name}.stderr.txt"
        stdout_path.write_text(measurement.stdout, encoding="utf-8")
        stderr_path.write_text(measurement.stderr, encoding="utf-8")
        if measurement.exit_code != 0:
            raise BenchmarkError(
                f"{prepared.scenario_id}/{condition}/{current_stage.name} a échoué "
                f"avec le code {measurement.exit_code}. Voir {stderr_path}."
            )
        combined_output = measurement.stdout + "\n" + measurement.stderr
        if current_stage.expected_text and current_stage.expected_text not in combined_output:
            raise BenchmarkError(
                f"Sortie attendue absente pour {current_stage.name}: "
                f"{current_stage.expected_text!r}. Voir {stdout_path}."
            )
        records.append(
            {
                "record_type": "stage",
                "schema_version": SCHEMA_VERSION,
                "scenario": prepared.scenario_id,
                "size": prepared.size,
                "condition": condition,
                "interface_mode": prepared.interface_mode,
                "iteration": iteration,
                "warmup": warmup,
                "stage": current_stage.name,
                "command": list(current_stage.command),
                "elapsed_ns": measurement.elapsed_ns,
                "elapsed_ms": measurement.elapsed_ns / 1_000_000,
                "peak_rss_bytes": measurement.peak_rss_bytes,
                "exit_code": measurement.exit_code,
                "stdout_log": str(stdout_path),
                "stderr_log": str(stderr_path),
            }
        )
    validate_magic(prepared.expected_magic)
    return records


def artifact_size(artifacts: Iterable[Path]) -> int:
    return sum(path.stat().st_size for path in artifacts if path.is_file())


def input_metrics(inputs: Iterable[Path]) -> dict:
    source_inputs = [path for path in inputs if path.suffix.lower() not in {".gspj", ".gsproject"}]
    return {
        "input_file_count": len(source_inputs),
        "input_bytes": sum(path.stat().st_size for path in source_inputs),
        "input_lines": sum(count_lines(path) for path in source_inputs),
    }


def run_sample(
    context: RunContext,
    scenario_id: str,
    condition: str,
    iteration: int,
    warmup: bool,
) -> list[dict]:
    run_kind = "warmup" if warmup else "measure"
    work_dir = (
        context.session_dir
        / "work"
        / scenario_id
        / condition
        / f"{run_kind}-{iteration:03d}"
    )
    logs_dir = (
        context.session_dir
        / "logs"
        / scenario_id
        / condition
        / f"{run_kind}-{iteration:03d}"
    )
    prepared = prepare_scenario(
        scenario_id, work_dir, context.source_root, context.compiler, context.loader
    )
    metrics = input_metrics(prepared.inputs)

    if condition != "cold_artifacts":
        baseline_logs = logs_dir / "baseline"
        run_stages(
            prepared.full_stages,
            prepared,
            "baseline_unmeasured",
            iteration,
            True,
            baseline_logs,
            context.affinity,
        )
        if condition not in {"warm_artifacts"}:
            target = prepared.mutation_targets.get(condition)
            if target is None:
                raise BenchmarkError(
                    f"Condition {condition} non prise en charge par {scenario_id}."
                )
            append_neutral_change(target, condition, iteration)

    measured_stages = (
        prepared.full_stages
        if condition in {"cold_artifacts", "warm_artifacts"}
        else prepared.incremental_stages[condition]
    )
    stage_records = run_stages(
        measured_stages,
        prepared,
        condition,
        iteration,
        warmup,
        logs_dir / "measured",
        context.affinity,
    )
    elapsed_ns = sum(record["elapsed_ns"] for record in stage_records)
    rss_values = [
        record["peak_rss_bytes"]
        for record in stage_records
        if record["peak_rss_bytes"] is not None
    ]
    total_record = {
        "record_type": "sample",
        "schema_version": SCHEMA_VERSION,
        "scenario": prepared.scenario_id,
        "size": prepared.size,
        "description": prepared.description,
        "condition": condition,
        "interface_mode": prepared.interface_mode,
        "iteration": iteration,
        "warmup": warmup,
        "stage": "pipeline_total",
        "stage_count": len(stage_records),
        "elapsed_ns": elapsed_ns,
        "elapsed_ms": elapsed_ns / 1_000_000,
        "peak_rss_bytes": max(rss_values) if rss_values else None,
        "artifact_bytes": artifact_size(prepared.artifacts),
        "validated": True,
        **metrics,
    }
    return [*stage_records, total_record]


def percentile(values: Sequence[float], probability: float) -> float:
    ordered = sorted(values)
    if len(ordered) == 1:
        return ordered[0]
    position = (len(ordered) - 1) * probability
    lower = math.floor(position)
    upper = math.ceil(position)
    if lower == upper:
        return ordered[lower]
    fraction = position - lower
    return ordered[lower] * (1 - fraction) + ordered[upper] * fraction


def bootstrap_median_interval(
    values: Sequence[float], seed: int, samples: int = DEFAULT_BOOTSTRAP_SAMPLES
) -> tuple[float, float]:
    if len(values) < 2:
        value = float(values[0])
        return value, value
    generator = random.Random(seed)
    medians = []
    for _ in range(samples):
        draw = [values[generator.randrange(len(values))] for _ in values]
        medians.append(float(statistics.median(draw)))
    return percentile(medians, 0.025), percentile(medians, 0.975)


def summarize(records: Sequence[dict], repetitions: int) -> list[dict]:
    groups: dict[tuple[str, str], list[dict]] = {}
    for record in records:
        if record["record_type"] != "sample" or record["warmup"]:
            continue
        groups.setdefault((record["scenario"], record["condition"]), []).append(record)

    summaries = []
    for group_index, ((scenario_id, condition), samples) in enumerate(sorted(groups.items())):
        elapsed = [float(sample["elapsed_ms"]) for sample in samples]
        rss = [
            float(sample["peak_rss_bytes"])
            for sample in samples
            if sample["peak_rss_bytes"] is not None
        ]
        confidence_low, confidence_high = bootstrap_median_interval(
            elapsed, seed=2300 + group_index
        )
        median_elapsed = float(statistics.median(elapsed))
        summaries.append(
            {
                "schema_version": SCHEMA_VERSION,
                "scenario": scenario_id,
                "condition": condition,
                "size": samples[0]["size"],
                "interface_mode": samples[0]["interface_mode"],
                "sample_count": len(samples),
                "requested_repetitions": repetitions,
                "inferential_ready": len(samples) >= 20,
                "elapsed_ms": {
                    "median": median_elapsed,
                    "minimum": min(elapsed),
                    "maximum": max(elapsed),
                    "q1": percentile(elapsed, 0.25),
                    "q3": percentile(elapsed, 0.75),
                    "iqr": percentile(elapsed, 0.75) - percentile(elapsed, 0.25),
                    "mad": float(
                        statistics.median(abs(value - median_elapsed) for value in elapsed)
                    ),
                    "bootstrap_median_ci95": [confidence_low, confidence_high],
                },
                "peak_rss_bytes_median": float(statistics.median(rss)) if rss else None,
                "artifact_bytes_median": float(
                    statistics.median(sample["artifact_bytes"] for sample in samples)
                ),
                "validated_samples": sum(bool(sample["validated"]) for sample in samples),
            }
        )
    return summaries


def write_json(path: Path, value: object) -> None:
    path.write_text(
        json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def append_jsonl(path: Path, records: Iterable[dict]) -> None:
    with path.open("a", encoding="utf-8", newline="\n") as stream:
        for record in records:
            stream.write(json.dumps(record, ensure_ascii=False, sort_keys=True) + "\n")


def write_summary_csv(path: Path, summaries: Sequence[dict]) -> None:
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(
            [
                "scenario",
                "condition",
                "size",
                "interface_mode",
                "sample_count",
                "inferential_ready",
                "median_ms",
                "q1_ms",
                "q3_ms",
                "iqr_ms",
                "ci95_low_ms",
                "ci95_high_ms",
                "peak_rss_bytes_median",
                "artifact_bytes_median",
            ]
        )
        for summary in summaries:
            elapsed = summary["elapsed_ms"]
            writer.writerow(
                [
                    summary["scenario"],
                    summary["condition"],
                    summary["size"],
                    summary["interface_mode"],
                    summary["sample_count"],
                    summary["inferential_ready"],
                    elapsed["median"],
                    elapsed["q1"],
                    elapsed["q3"],
                    elapsed["iqr"],
                    elapsed["bootstrap_median_ci95"][0],
                    elapsed["bootstrap_median_ci95"][1],
                    summary["peak_rss_bytes_median"],
                    summary["artifact_bytes_median"],
                ]
            )


def source_manifest(source_root: Path) -> list[dict]:
    gspp_root = source_root
    relative_paths = [
        "Exemples/Bonjour.Gs++",
        "Tests/Integration/TableauxObjetsClasses.GsPP",
        "Tests/Integration/Separation/TableauxObjetsClasses.HGsPP",
        "Tests/Integration/Separation/TableauxObjetsClassesImplementation.GsPP",
        "Tests/Integration/Separation/TableauxObjetsClassesPrincipal.GsPP",
        "Bibliotheques/Systeme/Systeme.HGsPP",
        "Bibliotheques/Systeme/Memoire.GsPP",
        "Bibliotheques/Systeme/Vues.GsPP",
        "Bibliotheques/Systeme/Bits.GsPP",
        "Bibliotheques/Systeme/Atomiques.GsPP",
    ]
    manifest = []
    for relative in relative_paths:
        path = gspp_root / relative
        manifest.append(
            {
                "path": relative,
                "bytes": path.stat().st_size,
                "lines": count_lines(path),
                "sha256": sha256_file(path),
            }
        )
    return manifest


def safe_remove_work(session_dir: Path) -> None:
    work_dir = (session_dir / "work").resolve()
    resolved_session = session_dir.resolve()
    if work_dir.parent != resolved_session or not work_dir.is_dir():
        return
    marker = resolved_session / "session.json"
    if not marker.is_file():
        raise BenchmarkError(f"Refus de nettoyer un dossier sans marqueur de session : {work_dir}")
    shutil.rmtree(work_dir)


def selected_conditions(scenario_id: str, requested: list[str] | None, mode: str) -> list[str]:
    available = list(SCENARIO_CATALOG[scenario_id]["conditions"])
    if mode == "smoke" and requested is None:
        return ["cold_artifacts"]
    if requested is None:
        return available
    return [condition for condition in requested if condition in available]


def main(argv: Sequence[str] | None = None) -> int:
    arguments = parse_arguments(argv or sys.argv[1:])
    if arguments.list_scenarios:
        for scenario_id, metadata in SCENARIO_CATALOG.items():
            print(f"{scenario_id} [{metadata['size']}] : {metadata['description']}")
            print(f"  conditions : {', '.join(metadata['conditions'])}")
        return 0

    source_root = resolve_source_root(arguments.source_root)
    compiler = resolve_tool(arguments.compiler, source_root, "gsppc")
    loader = resolve_tool(arguments.loader, source_root, "gsechargeur")
    affinity = parse_affinity(arguments.cpu)
    compiler_version = tool_version(compiler)
    loader_version = tool_version(loader)
    compiler_version_matches = compiler_version == EXPECTED_COMPILER_BANNER
    loader_version_matches = loader_version == EXPECTED_LOADER_BANNER
    version_matches = compiler_version_matches and loader_version_matches
    if not version_matches and not arguments.allow_version_mismatch:
        raise BenchmarkError(
            "Versions d'outils refusées : "
            f"compilateur={compiler_version!r}, chargeur={loader_version!r}; "
            f"attendu {EXPECTED_COMPILER_BANNER!r} et {EXPECTED_LOADER_BANNER!r}."
        )

    mode_defaults = {
        "smoke": (1, 0),
        "pilot": (5, 1),
        "full": (30, 3),
    }
    repetitions, warmups = mode_defaults[arguments.mode]
    if arguments.repetitions is not None:
        repetitions = arguments.repetitions
    if arguments.warmups is not None:
        warmups = arguments.warmups
    if repetitions < 1 or warmups < 0:
        raise BenchmarkError("Les répétitions doivent être >= 1 et les échauffements >= 0.")

    scenarios = arguments.scenario or list(SCENARIO_CATALOG)
    selections = {
        scenario_id: selected_conditions(scenario_id, arguments.condition, arguments.mode)
        for scenario_id in scenarios
    }
    empty = [scenario_id for scenario_id, conditions in selections.items() if not conditions]
    if empty:
        raise BenchmarkError(
            "Aucune condition compatible pour : " + ", ".join(empty)
        )

    output_root = (
        arguments.output_root
        or source_root / "Construction" / "Benchmarks" / "GsPlusPlus"
    ).resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    session_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ") + f"-{uuid.uuid4().hex[:8]}"
    session_dir = output_root / session_id
    session_dir.mkdir(parents=False, exist_ok=False)

    metadata = {
        "schema_version": SCHEMA_VERSION,
        "benchmark_version": BENCHMARK_VERSION,
        "session_id": session_id,
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "source_root": str(source_root),
        "mode": arguments.mode,
        "repetitions": repetitions,
        "warmups": warmups,
        "affinity": list(affinity),
        "cache_policy": {
            "cold_artifacts": "Répertoire d'artefacts neuf; cache OS non vidé.",
            "warm_artifacts": "Construction complète répétée avec artefacts conservés.",
            "incremental": "Construction de référence, commentaire neutre sur une copie, étapes ciblées documentées.",
        },
        "compiler": {
            "path": str(compiler),
            "version": compiler_version,
            "sha256": sha256_file(compiler),
            "expected_version": EXPECTED_COMPILER_VERSION,
            "expected_banner": EXPECTED_COMPILER_BANNER,
            "version_matches": compiler_version_matches,
        },
        "loader": {
            "path": str(loader),
            "version": loader_version,
            "sha256": sha256_file(loader),
            "expected_banner": EXPECTED_LOADER_BANNER,
            "version_matches": loader_version_matches,
        },
        "host": {
            "platform": platform.platform(),
            "system": platform.system(),
            "release": platform.release(),
            "machine": platform.machine(),
            "processor": platform.processor(),
            "logical_cpu_count": os.cpu_count(),
            "python": platform.python_version(),
        },
        "selections": selections,
        "corpus": source_manifest(source_root),
        "limitations": [
            "cold_artifacts ne vide pas le cache de fichiers du système d'exploitation",
            "les phases internes du compilateur ne sont pas séparables sans instrumentation",
            "les scénarios incrémentaux manuels ne prouvent pas l'existence d'un cache dans gsppc",
            "aucune comparaison C++20 ou Rust n'est calculée par ce pilote",
        ],
    }
    write_json(session_dir / "session.json", metadata)

    context = RunContext(
        source_root=source_root,
        compiler=compiler,
        loader=loader,
        session_dir=session_dir,
        affinity=affinity,
        keep_work=arguments.keep_work,
    )
    results_path = session_dir / "results.jsonl"

    print(f"Session : {session_dir}")
    try:
        for scenario_id in scenarios:
            for condition in selections[scenario_id]:
                print(f"[{scenario_id}] {condition}")
                for warmup_index in range(1, warmups + 1):
                    warmup_records = run_sample(
                        context, scenario_id, condition, warmup_index, True
                    )
                    append_jsonl(results_path, warmup_records)
                    context.records.extend(warmup_records)
                for iteration in range(1, repetitions + 1):
                    sample_records = run_sample(
                        context, scenario_id, condition, iteration, False
                    )
                    append_jsonl(results_path, sample_records)
                    context.records.extend(sample_records)
        summaries = summarize(context.records, repetitions)
        write_json(session_dir / "summary.json", summaries)
        write_summary_csv(session_dir / "summary.csv", summaries)
        write_json(
            session_dir / "status.json",
            {
                "status": "passed",
                "completed_utc": datetime.now(timezone.utc).isoformat(),
                "validated_samples": sum(
                    1
                    for record in context.records
                    if record["record_type"] == "sample" and not record["warmup"]
                ),
            },
        )
        if not arguments.keep_work:
            safe_remove_work(session_dir)
        print(f"Résultats : {session_dir / 'summary.csv'}")
        print("Statut : réussi")
        return 0
    except Exception as error:
        write_json(
            session_dir / "status.json",
            {
                "status": "failed",
                "completed_utc": datetime.now(timezone.utc).isoformat(),
                "error": str(error),
                "work_preserved": True,
            },
        )
        print(f"Échec : {error}", file=sys.stderr)
        print(f"Session conservée : {session_dir}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except BenchmarkError as error:
        print(f"Erreur : {error}", file=sys.stderr)
        raise SystemExit(2)
