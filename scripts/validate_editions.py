#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
EDITIONS = ROOT / "editions"
PACKAGE_DIR = ROOT / "packages" / "editions"
GENERATED = ROOT / "build" / "archiso-editions"
REQUIRED = {"EDITION_ID", "DISPLAY_NAME", "UPDATE_POLICY", "DEFAULT_THEME", "PACKAGE_SET", "APP_SET", "FEATURE_SET"}
POLICIES = {"automatic", "managed", "fast", "manual", "disabled"}
TOKEN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9@._+:-]*$")
errors: list[str] = []

for conf in sorted(EDITIONS.glob("*.conf")):
    values: dict[str, str] = {}
    for raw in conf.read_text(encoding="utf-8").splitlines():
        if not raw or raw.startswith("#") or "=" not in raw:
            continue
        key, value = raw.split("=", 1)
        values[key] = value
    missing = REQUIRED - values.keys()
    if missing:
        errors.append(f"{conf.name}: faltan claves {sorted(missing)}")
    if values.get("UPDATE_POLICY") not in POLICIES:
        errors.append(f"{conf.name}: UPDATE_POLICY inválida: {values.get('UPDATE_POLICY')}")
    if conf.stem == "frozen-lab" and "WARNING" not in values:
        errors.append("frozen-lab.conf: falta WARNING de mantenimiento manual")
    package_set = values.get("PACKAGE_SET", "")
    package_file = PACKAGE_DIR / f"{package_set}.packages"
    if not package_file.exists():
        errors.append(f"{conf.name}: no existe {package_file}")
        continue
    packages: list[str] = []
    for raw in package_file.read_text(encoding="utf-8").splitlines():
        item = raw.strip()
        if not item or item.startswith("#"):
            continue
        if not TOKEN.fullmatch(item):
            errors.append(f"{package_file.name}: nombre inválido {item!r}")
        packages.append(item)
    duplicates = sorted({item for item in packages if packages.count(item) > 1})
    if duplicates:
        errors.append(f"{package_file.name}: paquetes duplicados {duplicates}")
    generated = GENERATED / conf.stem / "airootfs" / "etc" / "influent-danenone" / "edition.conf"
    if GENERATED.exists() and not generated.exists():
        errors.append(f"{conf.name}: falta perfil generado {generated}")

if errors:
    print("EDITION_VALIDATION_STATUS=1")
    print("\n".join(errors))
    sys.exit(1)

print(f"EDITION_VALIDATION_STATUS=0 editions={len(list(EDITIONS.glob('*.conf')))}")
