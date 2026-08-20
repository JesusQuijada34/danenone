#!/usr/bin/env python3
"""Cliente local recuperable de Cloud Danenone Devices.

Este cliente sólo transmite una huella SHA-256 de identidad local. No recopila
ubicación. La UI de bloqueo y OOBE consume su salida JSON y mantiene la
recuperación mediante OTP como única vía de desbloqueo remoto.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
import tempfile
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path
from typing import Any

PAIRING_CODE = re.compile(r"^[A-Za-z0-9]{6,12}$")
OTP = re.compile(r"^\d{6}$")
CONFIG_PATH = Path(os.environ.get("XDG_CONFIG_HOME", Path.home() / ".config")) / "influent-danedesk" / "device.json"
LOCK_STATE_PATH = Path(os.environ.get("XDG_STATE_HOME", Path.home() / ".local" / "state")) / "influent-danedesk" / "lock-state.json"


class DaneDeskError(RuntimeError):
    pass


def hardware_id_hash() -> str:
    values: list[str] = []
    for candidate in (Path("/etc/machine-id"), Path("/sys/class/dmi/id/product_uuid")):
        try:
            value = candidate.read_text(encoding="utf-8").strip()
        except OSError:
            value = ""
        if value:
            values.append(value)
    if not values:
        raise DaneDeskError("No se pudo derivar una identidad de hardware para DaneDesk")
    return hashlib.sha256("\0".join(values).encode("utf-8")).hexdigest()


def save_json(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(prefix=f".{path.name}-", dir=path.parent)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as file:
            json.dump(value, file, ensure_ascii=False, sort_keys=True)
            file.write("\n")
        os.chmod(temporary, 0o600)
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def load_config() -> dict[str, Any]:
    try:
        return json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        raise DaneDeskError("Este DaneDesk no está activado todavía") from error


def server_url(value: str) -> str:
    parsed = urllib.parse.urlparse(value.rstrip("/"))
    if parsed.scheme != "https" or not parsed.netloc:
        raise DaneDeskError("Cloud Danenone Devices debe usar una URL HTTPS")
    return value.rstrip("/")


def trpc_call(server: str, route: str, payload: dict[str, Any], mutation: bool) -> dict[str, Any]:
    if mutation:
        request = urllib.request.Request(
            f"{server}/api/trpc/{route}",
            data=json.dumps({"json": payload}).encode("utf-8"),
            headers={"Accept": "application/json", "Content-Type": "application/json"},
            method="POST",
        )
    else:
        query = urllib.parse.urlencode({"input": json.dumps({"json": payload})})
        request = urllib.request.Request(f"{server}/api/trpc/{route}?{query}", headers={"Accept": "application/json"}, method="GET")
    try:
        with urllib.request.urlopen(request, timeout=20) as response:  # nosec B310: server validated as explicit HTTPS configuration
            body = json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as error:
        detail = error.read().decode("utf-8", errors="replace")[:240]
        raise DaneDeskError(f"Cloud Danenone Devices respondió {error.code}: {detail}") from error
    except urllib.error.URLError as error:
        raise DaneDeskError("No se pudo contactar Cloud Danenone Devices") from error
    if "error" in body:
        raise DaneDeskError(body["error"].get("json", {}).get("message", "Solicitud DaneDesk rechazada"))
    return body.get("result", {}).get("data", {}).get("json", body)


def activate(args: argparse.Namespace) -> dict[str, Any]:
    if not PAIRING_CODE.fullmatch(args.code.strip()):
        raise DaneDeskError("El código de activación debe ser alfanumérico y tener entre 6 y 12 caracteres")
    server = server_url(args.server)
    result = trpc_call(server, "danedesk.activate", {"code": args.code.strip().upper(), "hardwareIdHash": hardware_id_hash(), "displayName": args.name.strip()[:80] or "DaneDesk"}, True)
    if not result.get("id") or not result.get("agentToken"):
        raise DaneDeskError("La activación no devolvió una identidad DaneDesk válida")
    save_json(CONFIG_PATH, {"server": server, "deviceId": result["id"], "agentToken": result["agentToken"], "hardwareIdHash": hardware_id_hash(), "locationProtection": bool(result.get("locationProtection"))})
    return {"deviceId": result["id"], "locationProtection": bool(result.get("locationProtection")), "platform": result.get("platform", "Danenone")}


def check_status(args: argparse.Namespace) -> dict[str, Any]:
    config = load_config() if not args.server else {}
    server = server_url(args.server or str(config.get("server", "")))
    return trpc_call(server, "danedesk.checkStatus", {"hardwareIdHash": hardware_id_hash()}, False)


def enforce_lock(args: argparse.Namespace) -> dict[str, Any]:
    status = check_status(args)
    if status.get("recoveryRequired"):
        save_json(LOCK_STATE_PATH, {"status": status.get("status"), "lockReason": status.get("lockReason"), "checkedAt": __import__("datetime").datetime.now(__import__("datetime").timezone.utc).isoformat()})
    elif LOCK_STATE_PATH.exists():
        LOCK_STATE_PATH.unlink()
    return status


def recover(args: argparse.Namespace) -> dict[str, Any]:
    if not OTP.fullmatch(args.otp):
        raise DaneDeskError("El OTP debe contener seis dígitos")
    config = load_config()
    server = server_url(str(config.get("server", "")))
    result = trpc_call(server, "danedesk.recoverLocal", {"hardwareIdHash": hardware_id_hash(), "purpose": "unlock", "code": args.otp}, True)
    if result.get("success") and LOCK_STATE_PATH.exists():
        LOCK_STATE_PATH.unlink()
    return result


def emit(value: dict[str, Any]) -> None:
    print(json.dumps(value, ensure_ascii=False, sort_keys=True))


def main() -> int:
    parser = argparse.ArgumentParser(description="Cliente local recuperable de Cloud Danenone Devices")
    subcommands = parser.add_subparsers(dest="command", required=True)
    activate_parser = subcommands.add_parser("activate", help="Activar este DaneDesk mediante un código temporal")
    activate_parser.add_argument("--server", required=True)
    activate_parser.add_argument("--code", required=True)
    activate_parser.add_argument("--name", default=os.uname().nodename)
    activate_parser.set_defaults(handler=activate)
    status_parser = subcommands.add_parser("check-status", help="Consultar el estado recuperable sin enviar identificadores crudos")
    status_parser.add_argument("--server")
    status_parser.set_defaults(handler=check_status)
    lock_parser = subcommands.add_parser("enforce-lock", help="Actualizar el estado local de bloqueo desde Cloud DaneDesk")
    lock_parser.add_argument("--server")
    lock_parser.set_defaults(handler=enforce_lock)
    recovery_parser = subcommands.add_parser("recover", help="Validar localmente un OTP emitido al propietario")
    recovery_parser.add_argument("--otp", required=True)
    recovery_parser.set_defaults(handler=recover)
    args = parser.parse_args()
    try:
        emit(args.handler(args))
    except DaneDeskError as error:
        print(f"danedesk-client: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
