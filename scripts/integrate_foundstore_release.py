#!/usr/bin/env python3
"""Integra un release Fluthin verificado de Foundstore en un perfil Archiso.

La herramienta no ejecuta contenido del paquete. Comprueba rutas ZIP seguras,
metadatos y binario principal antes de copiar el artefacto y extraerlo dentro de
``airootfs`` para que la sesión live disponga de un lanzador local.
"""
from __future__ import annotations

import argparse
import shutil
import stat
import tempfile
import zipfile
from pathlib import Path, PurePosixPath
from xml.etree import ElementTree


def safe_member(name: str) -> bool:
    path = PurePosixPath(name)
    return bool(name) and not path.is_absolute() and ".." not in path.parts and "\\" not in name


def parse_artifact(artifact: Path) -> tuple[dict[str, str], list[str]]:
    if not artifact.is_file() or not zipfile.is_zipfile(artifact):
        raise ValueError("El artefacto Foundstore no es un ZIP/.iflapp válido.")
    with zipfile.ZipFile(artifact) as archive:
        names = archive.namelist()
        if not names or any(not safe_member(name) for name in names):
            raise ValueError("El artefacto contiene una ruta insegura.")
        if "details.xml" not in names or "bin/foundstore" not in names:
            raise ValueError("El artefacto no contiene details.xml y bin/foundstore.")
        root = ElementTree.fromstring(archive.read("details.xml"))
        metadata = {key: (root.findtext(key) or "").strip() for key in ("publisher", "app", "version", "platform")}
        if metadata["publisher"] != "Influent" or metadata["app"] != "foundstore":
            raise ValueError("El artefacto no pertenece a Influent Foundstore.")
        if metadata["platform"] != "Danenone":
            raise ValueError("El artefacto no fue compilado para Danenone.")
        return metadata, names


def safe_extract(artifact: Path, destination: Path) -> None:
    with zipfile.ZipFile(artifact) as archive:
        for info in archive.infolist():
            if not safe_member(info.filename):
                raise ValueError(f"Ruta insegura en paquete: {info.filename}")
            target = destination / PurePosixPath(info.filename)
            target.parent.mkdir(parents=True, exist_ok=True)
            if info.is_dir():
                target.mkdir(parents=True, exist_ok=True)
                continue
            with archive.open(info) as source, target.open("wb") as output:
                shutil.copyfileobj(source, output)
            mode = (info.external_attr >> 16) & 0o777
            if mode:
                target.chmod(mode)


def integrate(artifact: Path, profile: Path) -> Path:
    metadata, names = parse_artifact(artifact)
    airootfs = profile / "airootfs"
    if not (profile / "packages.x86_64").is_file() or not airootfs.is_dir():
        raise ValueError("La ruta no contiene un perfil Archiso válido.")

    package_name = artifact.stem
    package_dir = airootfs / "opt" / "influent-danenone" / "packages" / package_name
    artifact_dir = airootfs / "usr" / "share" / "influent" / "packages"
    if package_dir.exists():
        shutil.rmtree(package_dir)
    package_dir.mkdir(parents=True, exist_ok=True)
    safe_extract(artifact, package_dir)

    binary = package_dir / "bin" / "foundstore"
    if not binary.is_file():
        raise ValueError("El binario principal no quedó extraído en la ruta esperada.")
    binary.chmod(binary.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

    artifact_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(artifact, artifact_dir / artifact.name)

    launcher = airootfs / "usr" / "local" / "bin" / "foundstore"
    launcher.parent.mkdir(parents=True, exist_ok=True)
    launcher.write_text(f"#!/bin/sh\nexec /opt/influent-danenone/packages/{package_name}/bin/foundstore \"$@\"\n", encoding="utf-8")
    launcher.chmod(0o755)

    icon_sources = ("app/app-icon.png", "app/app-icon.ico", "app/app-icon.svg")
    icon_source = next((name for name in icon_sources if name in names), None)
    if icon_source:
        suffix = Path(icon_source).suffix.lower()
        icon_target = airootfs / "usr" / "share" / "icons" / "hicolor" / "256x256" / "apps" / f"influent-foundstore{suffix}"
        icon_target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(package_dir / icon_source, icon_target)

    desktop_entry = airootfs / "usr" / "share" / "applications" / "influent-foundstore.desktop"
    desktop_entry.parent.mkdir(parents=True, exist_ok=True)
    desktop_entry.write_text(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=Foundstore\n"
        "Comment=Tienda de aplicaciones Fluthin para Danenone\n"
        "Exec=foundstore\n"
        "Icon=influent-foundstore\n"
        "Terminal=false\n"
        "Categories=System;Utility;\n"
        "StartupNotify=true\n",
        encoding="utf-8",
    )
    return package_dir


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--artifact", type=Path, required=True)
    parser.add_argument("--profile", type=Path, required=True)
    arguments = parser.parse_args()
    result = integrate(arguments.artifact.resolve(), arguments.profile.resolve())
    print(result)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
