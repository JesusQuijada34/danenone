from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class InstallPlan:
    device: str
    username: str
    hostname: str
    timezone: str
    dry_run: bool = True


class InstallerValidationError(ValueError):
    pass


def validate_plan(plan: InstallPlan) -> None:
    device = Path(plan.device)
    if not plan.username or not plan.username.replace("_", "").replace("-", "").isalnum():
        raise InstallerValidationError("El usuario debe usar letras, números, guion o guion bajo")
    if not plan.hostname or "/" in plan.hostname or ".." in plan.hostname:
        raise InstallerValidationError("Hostname inválido")
    if not plan.timezone or "/" not in plan.timezone:
        raise InstallerValidationError("Zona horaria inválida")
    if not device.is_absolute() or device == Path("/"):
        raise InstallerValidationError("El dispositivo debe ser una ruta absoluta distinta de /")


def plan_install(plan: InstallPlan) -> dict[str, str | bool]:
    validate_plan(plan)
    return {
        "device": plan.device,
        "username": plan.username,
        "hostname": plan.hostname,
        "timezone": plan.timezone,
        "dry_run": plan.dry_run,
        "status": "simulation-only" if plan.dry_run else "requires-privileged-executor",
    }
