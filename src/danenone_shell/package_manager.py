from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import tempfile
import zipfile
from dataclasses import dataclass
from pathlib import Path
from xml.etree import ElementTree as ET


@dataclass(frozen=True)
class InstalledPackage:
    name: str
    version: str
    kind: str
    path: str
    details_xml: str | None = None


class PackageManager:
    def __init__(self, root: Path = Path("/usr/lib/influent/packages"), registry: Path = Path("/var/lib/influent/installed-packages.json")):
        self.root = root
        self.registry = registry

    def validate_iflapp(self, package: Path) -> dict[str, str]:
        with zipfile.ZipFile(package) as archive:
            names = archive.namelist()
            if not names or any(Path(name).is_absolute() or ".." in Path(name).parts for name in names):
                raise ValueError("Paquete vacío o con path traversal")
            details_name = next((name for name in names if name.endswith("details.xml")), None)
            if not details_name:
                raise ValueError("Falta details.xml")
            root = ET.fromstring(archive.read(details_name))
            metadata = {key: (root.findtext(key) or "").strip() for key in ("publisher", "app", "version", "author", "platform")}
            if not metadata["app"] or not metadata["version"] or metadata["platform"] != "Danenone":
                raise ValueError("Metadatos Fluthin incompletos o plataforma incompatible")
            digest = hashlib.sha256(package.read_bytes()).hexdigest()
            metadata["sha256"] = digest
            return metadata

    def install_iflapp(self, package: Path) -> InstalledPackage:
        metadata = self.validate_iflapp(package)
        target = self.root / package.stem
        staging = Path(tempfile.mkdtemp(prefix="influent-stage-", dir=str(package.parent)))
        backup = None
        try:
            with zipfile.ZipFile(package) as archive:
                archive.extractall(staging)
            if target.exists():
                backup = target.with_name(target.name + ".backup")
                if backup.exists():
                    shutil.rmtree(backup)
                target.rename(backup)
            self.root.mkdir(parents=True, exist_ok=True)
            shutil.move(str(staging), str(target))
            if backup and backup.exists():
                shutil.rmtree(backup)
            installed = InstalledPackage(package.stem, metadata["version"], "iflapp", str(target), str(target / "details.xml"))
            self._register(installed)
            return installed
        except Exception:
            if target.exists():
                shutil.rmtree(target)
            if backup and backup.exists():
                backup.rename(target)
            shutil.rmtree(staging, ignore_errors=True)
            raise

    def install_deb(self, package: Path, dry_run: bool = True) -> bool:
        if package.suffix != ".deb":
            raise ValueError("Se esperaba un paquete .deb")
        command = ["dpkg", "-i", str(package)]
        if dry_run:
            return subprocess.run(["dpkg", "--info", str(package)], check=False).returncode == 0
        if os.geteuid() != 0:
            raise PermissionError("La instalación Debian requiere privilegios del sistema")
        return subprocess.run(command, check=False).returncode == 0

    def _register(self, package: InstalledPackage) -> None:
        self.registry.parent.mkdir(parents=True, exist_ok=True)
        current = []
        if self.registry.exists():
            try:
                current = json.loads(self.registry.read_text(encoding="utf-8"))
            except (ValueError, OSError):
                current = []
        current = [item for item in current if item.get("package") != package.path]
        current.append({"package": package.path, "details_xml": package.details_xml, "version": package.version})
        self.registry.write_text(json.dumps(current, ensure_ascii=False, indent=2), encoding="utf-8")
