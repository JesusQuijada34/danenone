from __future__ import annotations

import json
import re
import urllib.request
import zipfile
from dataclasses import asdict, dataclass
from pathlib import Path
from xml.etree import ElementTree as ET


SYSTEM_ROOT = Path("/opt/influent-danenone")
REGISTRY = Path("/var/lib/influent/installed-packages.json")
GITHUB_API = "https://api.github.com"


@dataclass(frozen=True)
class PackageMetadata:
    publisher: str
    app: str
    name: str
    version: str
    author: str
    platform: str
    source: str


@dataclass(frozen=True)
class UpdateNotice:
    app: str
    name: str
    installed_version: str
    available_version: str
    release_url: str
    asset_name: str


def _version_key(version: str) -> tuple:
    parts = re.findall(r"\d+|[A-Za-z]+", version)
    return tuple(int(part) if part.isdigit() else part.lower() for part in parts)


def parse_details_xml(path: Path, source: str = "") -> PackageMetadata:
    root = ET.parse(path).getroot()
    get = lambda key, fallback="": (root.findtext(key) or fallback).strip()
    values = PackageMetadata(
        publisher=get("publisher", get("empresa")),
        app=get("app", get("name")),
        name=get("name", get("app")),
        version=get("version"),
        author=get("author", get("autor")),
        platform=get("platform", get("target")),
        source=source or str(path),
    )
    required = (values.app, values.version, values.author)
    if not all(required):
        raise ValueError(f"Metadatos incompletos en {path}")
    return values


def _json_request(url: str) -> dict | list:
    request = urllib.request.Request(url, headers={"Accept": "application/vnd.github+json", "User-Agent": "Influent-Danenone-Updater"})
    with urllib.request.urlopen(request, timeout=12) as response:
        return json.loads(response.read().decode("utf-8"))


def find_release(metadata: PackageMetadata) -> UpdateNotice | None:
    repo = f"{metadata.author}/{metadata.app}"
    releases = _json_request(f"{GITHUB_API}/repos/{repo}/releases")
    if not isinstance(releases, list):
        return None
    best = None
    for release in releases:
        if release.get("draft") or release.get("prerelease"):
            continue
        tag = str(release.get("tag_name", ""))
        version = tag[1:] if tag.startswith("v") else tag
        if _version_key(version) <= _version_key(metadata.version):
            continue
        for asset in release.get("assets", []):
            name = asset.get("name", "")
            if name.endswith(".iflapp") and metadata.app.lower() in name.lower():
                candidate = UpdateNotice(metadata.app, metadata.name, metadata.version, version, release.get("html_url", ""), name)
                if best is None or _version_key(candidate.available_version) > _version_key(best.available_version):
                    best = candidate
    return best


def installed_packages(registry: Path = REGISTRY) -> list[PackageMetadata]:
    if not registry.exists():
        return []
    data = json.loads(registry.read_text(encoding="utf-8"))
    result = []
    for item in data if isinstance(data, list) else []:
        path = Path(item.get("details_xml", ""))
        if path.exists():
            try:
                result.append(parse_details_xml(path, item.get("package", "")))
            except (ET.ParseError, OSError, ValueError):
                continue
    return result


def scan_updates(registry: Path = REGISTRY) -> list[UpdateNotice]:
    notices = []
    for package in installed_packages(registry):
        try:
            notice = find_release(package)
        except Exception:
            notice = None
        if notice:
            notices.append(notice)
    return notices


def validate_iflapp(path: Path) -> PackageMetadata:
    with zipfile.ZipFile(path) as archive:
        names = set(archive.namelist())
        if "details.xml" not in names:
            raise ValueError("El paquete no contiene details.xml")
        for name in names:
            if Path(name).is_absolute() or ".." in Path(name).parts:
                raise ValueError("Se detectó path traversal en el paquete")
        extracted = Path("/tmp") / f"influent-details-{path.stem}.xml"
        extracted.write_bytes(archive.read("details.xml"))
        return parse_details_xml(extracted, str(path))


def main() -> int:
    notices = scan_updates()
    print(json.dumps([asdict(item) for item in notices], ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
