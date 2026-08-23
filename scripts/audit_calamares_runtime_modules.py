#!/usr/bin/env python3
"""Read-only audit of Calamares runtime modules required by a reference sequence."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


MODULE_PATTERN = re.compile(r"/usr/lib/calamares/modules/([^/]+)/")
FORBIDDEN_SEQUENCE_MODULES = {"shellprocess", "contextualprocess", "webview"}


def reference_modules(sequence_path: Path) -> set[str]:
    modules = set()
    for raw_line in sequence_path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if line.startswith("- "):
            modules.add(line[2:].strip())
    return modules


def package_modules(package_listing: str) -> set[str]:
    return set(MODULE_PATTERN.findall(package_listing))


def audit(reference: set[str], available: set[str]) -> tuple[set[str], set[str]]:
    forbidden = reference & FORBIDDEN_SEQUENCE_MODULES
    missing = reference - available
    return missing, forbidden


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Audita módulos Calamares sin instalar ni ejecutar el runtime."
    )
    parser.add_argument("--package", required=True, help="Ruta del paquete .pkg.tar.zst visible para pacman.")
    parser.add_argument(
        "--sequence",
        required=True,
        type=Path,
        help="Archivo execution-sequence.yaml de referencia no activada.",
    )
    parser.add_argument("--pacman", default="pacman", help="Binario pacman a emplear para -Qlp.")
    parser.add_argument(
        "--chroot",
        help="Root opcional para ejecutar pacman dentro de un chroot de sólo inspección.",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    reference = reference_modules(args.sequence)
    command = [args.pacman, "-Qlp", args.package]
    if args.chroot:
        command = ["chroot", args.chroot, *command]

    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True)
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"RECHAZADO: no se pudo inspeccionar el paquete: {error}", file=sys.stderr)
        return 2

    available = package_modules(result.stdout)
    missing, forbidden = audit(reference, available)
    if forbidden:
        print(
            "RECHAZADO: la secuencia de referencia incluye módulos prohibidos: "
            + ", ".join(sorted(forbidden)),
            file=sys.stderr,
        )
        return 2
    if missing:
        print(
            "RECHAZADO: el runtime no contiene módulos requeridos: " + ", ".join(sorted(missing)),
            file=sys.stderr,
        )
        return 2

    print("VALIDADO: el runtime contiene todos los módulos de la secuencia de referencia.")
    print("required=" + ",".join(sorted(reference)))
    print("available_count=" + str(len(available)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
