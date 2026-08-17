#!/usr/bin/env python3
"""Indexación pasiva y segura de paquetes Fluthin .iflapp para Danenone."""
from __future__ import annotations

import argparse
import base64
import json
import os
import shutil
import sys
import zipfile
from pathlib import Path, PurePosixPath
from xml.etree import ElementTree

ROOT = Path(__file__).resolve().parents[1]
LOCAL_ARTIFACTS = ROOT / "build" / "system-fluthin-artifacts"
SYSTEM_DIRS = [
    Path("/var/lib/influent/packages"),
    Path.home() / ".local/share/influent/packages",
    Path("/usr/share/influent/packages"),
    LOCAL_ARTIFACTS,
]
CACHE = Path.home() / ".cache/influent-danenone/iflapp-icons"


def safe_member(name: str) -> bool:
    path = PurePosixPath(name)
    return not path.is_absolute() and ".." not in path.parts and "\\" not in name


def xml_text(root: ElementTree.Element, key: str) -> str:
    node = root.find(key)
    return (node.text or "").strip() if node is not None else ""


def package_record(path: Path) -> dict[str, str] | None:
    try:
        with zipfile.ZipFile(path) as archive:
            names = archive.namelist()
            if not names or any(not safe_member(name) for name in names):
                return None
            details = next((name for name in names if PurePosixPath(name).name == "details.xml"), None)
            if not details:
                return None
            root = ElementTree.fromstring(archive.read(details))
            app_id = xml_text(root, "app") or path.stem.split(".")[1] if "." in path.stem else path.stem
            publisher = xml_text(root, "publisher")
            name = xml_text(root, "name") or app_id
            version = xml_text(root, "version")
            platform = xml_text(root, "platform")
            icon = next((candidate for candidate in names if candidate in {"app/app-icon.ico", "app/app-icon.png", "app/app-icon.svg"}), None)
            if icon is None:
                icon = next((candidate for candidate in names if PurePosixPath(candidate).parent.name == "app" and PurePosixPath(candidate).stem.endswith("-icon") and PurePosixPath(candidate).suffix.lower() in {".ico", ".png", ".svg"}), None)
            if not icon:
                return None
            CACHE.mkdir(parents=True, exist_ok=True)
            target = CACHE / f"{publisher or 'package'}-{app_id}{Path(icon).suffix.lower()}"
            target.write_bytes(archive.read(icon))
            return {
                "id": f"iflapp:{publisher}.{app_id}" if publisher else f"iflapp:{app_id}",
                "name": name,
                "version": version,
                "platform": platform,
                "package": str(path),
                "icon": str(target),
                "publisher": publisher,
            }
    except (OSError, zipfile.BadZipFile, ElementTree.ParseError, KeyError):
        return None


def index() -> list[dict[str, str]]:
    records: list[dict[str, str]] = []
    seen: set[str] = set()
    for directory in SYSTEM_DIRS:
        if not directory.is_dir():
            continue
        for path in sorted(directory.glob("*.iflapp")):
            record = package_record(path)
            if record and record["id"] not in seen:
                records.append(record)
                seen.add(record["id"])
    return records


def install(package: Path) -> dict[str, str]:
    record = package_record(package)
    if record is None:
        raise ValueError("El paquete no es un .iflapp válido con details.xml e icono reconocido")
    destination = Path.home() / ".local/share/influent/packages"
    destination.mkdir(parents=True, exist_ok=True)
    target = destination / package.name
    shutil.copy2(package, target)
    return {**record, "installed": str(target)}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--install")
    args = parser.parse_args()
    if args.install:
        try:
            print(json.dumps(install(Path(args.install)), ensure_ascii=False))
            return 0
        except ValueError as exc:
            print(json.dumps({"error": str(exc)}), file=sys.stderr)
            return 2
    records = index()
    print(json.dumps(records, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
