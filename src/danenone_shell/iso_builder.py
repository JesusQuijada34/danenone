from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

REQUIRED_TOOLS = ("live-build", "debootstrap", "grub-mkrescue", "xorriso", "mksquashfs")


def available_tools() -> dict[str, str | None]:
    return {tool: shutil.which(tool) for tool in REQUIRED_TOOLS}


def write_manifest(destination: Path) -> Path:
    destination.mkdir(parents=True, exist_ok=True)
    manifest = {
        "distribution": "Danenone",
        "version": "0.1.0-prototype",
        "base": "Linux kernel with Debian-compatible userspace",
        "ui": "Python + PyQt5 prototype",
        "required_tools": list(REQUIRED_TOOLS),
        "tool_status": available_tools(),
    }
    path = destination / "danenone-manifest.json"
    path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return path


def build_iso(output: Path) -> Path:
    status = available_tools()
    missing = [tool for tool, path in status.items() if not path]
    if missing:
        raise RuntimeError("Faltan herramientas de ISO: " + ", ".join(missing))
    raise NotImplementedError("El backend live-build se habilita en la etapa de imagen reproducible")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Danenone ISO builder")
    parser.add_argument("--manifest", type=Path, default=Path("build/manifest"))
    parser.add_argument("--output", type=Path, default=Path("build/danenone.iso"))
    parser.add_argument("--build", action="store_true")
    args = parser.parse_args(argv)
    path = write_manifest(args.manifest)
    print(path)
    if args.build:
        print(build_iso(args.output))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
