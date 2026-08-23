#!/usr/bin/env python3
"""Validate future Calamares lab inputs without launching or modifying anything."""

from __future__ import annotations

import argparse
import hashlib
import re
import sys
from pathlib import Path


RC1_SHA256 = "44eb1e0cb35c3d26220400501151c2a5d6e6fde1929775668737ed150c5c1901"
RC1_NAME_FRAGMENT = "plasma-0.5.0-rc1"
SHA256_PATTERN = re.compile(r"^[0-9a-f]{64}$")


def fail(message: str) -> None:
    raise ValueError(message)


def canonical_workspace(value: str) -> Path:
    path = Path(value).expanduser().resolve(strict=True)
    if not path.is_dir():
        fail("El directorio de trabajo debe existir y ser un directorio.")
    return path


def canonical_child(value: str, workspace: Path, label: str) -> Path:
    path = Path(value).expanduser().resolve(strict=True)
    if path.is_relative_to(Path("/dev")):
        fail(f"{label} no puede apuntar a un dispositivo físico bajo /dev.")
    if not path.is_file():
        fail(f"{label} debe ser un archivo regular existente.")
    try:
        path.relative_to(workspace)
    except ValueError:
        fail(f"{label} debe estar dentro del directorio de trabajo indicado.")
    return path


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate(workspace_value: str, iso_value: str, disks: list[str], expected_hash: str) -> tuple[Path, Path, list[Path], str]:
    workspace = canonical_workspace(workspace_value)
    expected_hash = expected_hash.lower()
    if not SHA256_PATTERN.fullmatch(expected_hash):
        fail("El SHA-256 esperado debe contener exactamente 64 caracteres hexadecimales.")

    iso = canonical_child(iso_value, workspace, "La ISO")
    if iso.suffix.lower() != ".iso":
        fail("La ISO de laboratorio debe terminar en .iso.")
    if RC1_NAME_FRAGMENT in iso.name.casefold():
        fail("La ISO candidata Plasma RC1 no es admisible como medio de laboratorio.")

    observed_hash = sha256_file(iso)
    if observed_hash == RC1_SHA256:
        fail("La ISO coincide con el checksum publicado de Plasma RC1 y está prohibida.")
    if observed_hash != expected_hash:
        fail("El SHA-256 observado de la ISO no coincide con el valor esperado.")

    if not disks:
        fail("Debe proporcionar al menos un disco virtual qcow2.")
    validated_disks = []
    for disk_value in disks:
        disk = canonical_child(disk_value, workspace, "El disco virtual")
        if disk.suffix.lower() != ".qcow2":
            fail("Cada disco de laboratorio debe terminar en .qcow2.")
        validated_disks.append(disk)

    return workspace, iso, validated_disks, observed_hash


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Valida entradas de laboratorio Calamares; no inicia QEMU ni escribe discos."
    )
    parser.add_argument("--workspace", required=True, help="Directorio de laboratorio permitido.")
    parser.add_argument("--iso", required=True, help="ISO de laboratorio nueva dentro del directorio.")
    parser.add_argument(
        "--disk",
        action="append",
        default=[],
        help="Disco qcow2 existente dentro del directorio; puede repetirse.",
    )
    parser.add_argument("--expect-iso-sha256", required=True, help="SHA-256 registrado para la ISO.")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    try:
        args = parse_args(argv)
        workspace, iso, disks, observed_hash = validate(
            args.workspace, args.iso, args.disk, args.expect_iso_sha256
        )
    except (OSError, ValueError) as error:
        print(f"RECHAZADO: {error}", file=sys.stderr)
        return 2

    print("VALIDADO: no se iniciará ninguna instalación.")
    print(f"workspace={workspace}")
    print(f"iso={iso}")
    print(f"iso_sha256={observed_hash}")
    for disk in disks:
        print(f"qcow2={disk}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
