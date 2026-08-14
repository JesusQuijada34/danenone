from __future__ import annotations

import hashlib
import io
import json
import zipfile
from dataclasses import dataclass
from pathlib import PurePosixPath
from typing import Any

import requests


@dataclass(frozen=True)
class StoreRelease:
    repository: str
    tag: str
    asset_name: str
    download_url: str
    size: int
    digest: str | None = None


class FoundStoreClient:
    """Read-only FoundStore/GitHub release client with package validation."""

    def __init__(self, session: requests.Session | None = None, timeout: int = 20):
        self.session = session or requests.Session()
        self.timeout = timeout

    def list_releases(self, repositories: list[str]) -> list[StoreRelease]:
        releases: list[StoreRelease] = []
        for repository in repositories:
            response = self.session.get(
                f"https://api.github.com/repos/{repository}/releases",
                timeout=self.timeout,
                headers={"Accept": "application/vnd.github+json"},
            )
            response.raise_for_status()
            for release in response.json():
                for asset in release.get("assets", []):
                    name = asset.get("name", "")
                    if name.endswith(".iflapp"):
                        digest = asset.get("digest")
                        releases.append(StoreRelease(repository, release["tag_name"], name, asset["browser_download_url"], int(asset.get("size", 0)), digest))
        return releases

    def download_and_validate(self, release: StoreRelease, destination: str) -> dict[str, Any]:
        if not release.download_url.startswith("https://") or not release.asset_name.endswith(".iflapp"):
            raise ValueError("URL o extensión de paquete no segura")
        response = self.session.get(release.download_url, timeout=self.timeout, stream=True)
        response.raise_for_status()
        data = response.content
        if release.size and len(data) != release.size:
            raise ValueError("El tamaño descargado no coincide con el asset")
        digest = hashlib.sha256(data).hexdigest()
        if release.digest and release.digest.startswith("sha256:") and digest != release.digest.split(":", 1)[1]:
            raise ValueError("El SHA-256 no coincide")
        with zipfile.ZipFile(io.BytesIO(data)) as package:
            names = package.namelist()
            if not names or any(PurePosixPath(name).is_absolute() or ".." in PurePosixPath(name).parts for name in names):
                raise ValueError("Paquete vacío o con path traversal")
            details_name = next((name for name in names if name.endswith("details.xml")), None)
            if not details_name:
                raise ValueError("Falta details.xml")
        with open(destination, "wb") as handle:
            handle.write(data)
        return {"path": destination, "sha256": digest, "entries": len(names), "details": details_name}

    @staticmethod
    def load_catalog(path: str) -> list[str]:
        with open(path, encoding="utf-8") as handle:
            catalog = json.load(handle)
        return [str(repo) for repo in catalog.get("repositories", [])]
