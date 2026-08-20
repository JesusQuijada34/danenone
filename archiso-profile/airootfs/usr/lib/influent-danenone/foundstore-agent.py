#!/usr/bin/env python3
"""Cliente local de Foundstore para solicitudes aprobadas por la persona usuaria.

El agente no sondea automáticamente ni instala paquetes por sí mismo. Solo consulta la
cola cuando se invoca y solo ejecuta ``flut install`` después de que la persona elija
explícitamente ``approve <id>`` en el terminal local de Danenone.
"""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import os
import subprocess
import sys
import re
import time
from datetime import datetime, timezone
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

CONFIG_PATH = Path(os.environ.get("XDG_CONFIG_HOME", Path.home() / ".config")) / "foundstore-agent" / "config.json"
PAIRING_CODE = re.compile(r"[A-Za-z0-9]{6,12}")


class AgentError(RuntimeError):
    pass


def fail(message: str) -> None:
    print(f"foundstore-agent: {message}", file=sys.stderr)
    raise SystemExit(1)


def request_json(url: str, payload: dict | None = None, agent_token: str | None = None) -> dict:
    headers = {"Accept": "application/json"}
    if agent_token:
        headers["X-Danenone-Agent-Token"] = agent_token
    if payload is None:
        request = urllib.request.Request(url, headers=headers, method="GET")
    else:
        headers["Content-Type"] = "application/json"
        request = urllib.request.Request(url, data=json.dumps({"json": payload}).encode("utf-8"), headers=headers, method="POST")
    try:
        with urllib.request.urlopen(request, timeout=20) as response:
            body = json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as error:
        raise AgentError(f"el servidor respondió {error.code}: {error.read().decode('utf-8', 'replace')[:240]}") from error
    except urllib.error.URLError as error:
        raise AgentError(f"no se pudo contactar Foundstore: {error.reason}") from error
    if "error" in body:
        raise AgentError(body["error"].get("json", {}).get("message", "solicitud rechazada"))
    return body.get("result", {}).get("data", {}).get("json", body)


def load_config() -> dict:
    if not CONFIG_PATH.exists():
        fail("este equipo no está emparejado; usa `foundstore-agent pair <código>` primero")
    try:
        return json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        fail("la configuración local del agente no es válida")


def write_config(config: dict) -> None:
    CONFIG_PATH.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    CONFIG_PATH.write_text(json.dumps(config, indent=2), encoding="utf-8")
    os.chmod(CONFIG_PATH, 0o600)


def pair(args: argparse.Namespace) -> None:
    server = args.server.rstrip("/")
    parsed_server = urllib.parse.urlparse(server)
    code = args.code.strip().upper()
    if parsed_server.scheme != "https" or not parsed_server.netloc:
        fail("el servidor Foundstore debe usar HTTPS")
    if not PAIRING_CODE.fullmatch(code):
        fail("el código de emparejamiento debe ser alfanumérico y tener entre 6 y 12 caracteres")
    payload = {"code": code, "displayName": args.name}
    data = request_json(f"{server}/api/trpc/agent.bootstrap", payload)
    write_config({"server": server, "deviceId": data["id"], "agentToken": data["agentToken"], "commandKey": data["commandKey"], "platform": data.get("platform", "Danenone")})
    print(f"Equipo emparejado: {args.name} ({data['id']})")
    print("La credencial se guardó con permisos locales 0600.")


def parse_pair_uri(uri: str) -> tuple[str, str]:
    parsed = urllib.parse.urlparse(uri)
    if parsed.scheme != "foundstore" or parsed.netloc != "agent" or parsed.path != "/pair":
        fail("la URI debe usar el formato foundstore://agent/pair?server=<https>&code=<código>")
    values = urllib.parse.parse_qs(parsed.query, strict_parsing=True)
    server = values.get("server", [""])[0].rstrip("/")
    code = values.get("code", [""])[0].upper()
    server_url = urllib.parse.urlparse(server)
    if server_url.scheme != "https" or not server_url.netloc:
        fail("la URI debe declarar un servidor Foundstore HTTPS válido")
    if not PAIRING_CODE.fullmatch(code):
        fail("la URI debe incluir un código alfanumérico de entre 6 y 12 caracteres")
    return server, code


def pair_uri(args: argparse.Namespace) -> None:
    server, code = parse_pair_uri(args.uri)
    pair(argparse.Namespace(server=server, code=code, name=args.name))


def pending(config: dict) -> list[dict]:
    query = urllib.parse.urlencode({"input": json.dumps({"json": {"deviceId": config["deviceId"]}})})
    data = request_json(f"{config['server']}/api/trpc/agent.pending?{query}", agent_token=config["agentToken"])
    return data if isinstance(data, list) else []


def _valid_command(command: dict, config: dict) -> bool:
    required = ("id", "deviceId", "type", "payload", "expiresAt", "signature")
    if any(key not in command for key in required) or command["deviceId"] != config["deviceId"]:
        return False
    try:
        expires_at = datetime.fromisoformat(command["expiresAt"].replace("Z", "+00:00"))
        if expires_at <= datetime.now(timezone.utc):
            return False
    except (TypeError, ValueError):
        return False
    if not config.get("commandKey"):
        return False
    signed = {key: command[key] for key in ("id", "deviceId", "type", "payload", "expiresAt")}
    canonical = json.dumps(signed, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
    expected = hmac.new(config["commandKey"].encode("utf-8"), canonical, hashlib.sha256).hexdigest()
    return hmac.compare_digest(expected, str(command["signature"]))


def next_commands(config: dict, wait_seconds: int = 25) -> dict:
    query = urllib.parse.urlencode({"input": json.dumps({"json": {"deviceId": config["deviceId"], "waitSeconds": min(max(wait_seconds, 0), 25)}})})
    data = request_json(f"{config['server']}/api/trpc/agent.next?{query}", agent_token=config["agentToken"])
    commands = [command for command in data.get("commands", []) if _valid_command(command, config)]
    return {"commands": commands, "retryAfterSeconds": data.get("retryAfterSeconds", 15)}


def daemon(config: dict, max_cycles: int | None = None, sleep_fn=time.sleep) -> None:
    """Mantiene un único long-poll, sin instalar ni aprobar paquetes automáticamente."""
    delay = 1
    cycles = 0
    seen: set[str] = set()
    while True:
        try:
            result = next_commands(config, wait_seconds=25)
            delay = min(max(int(result.get("retryAfterSeconds", 15)), 1), 120)
            for command in result["commands"]:
                if command["id"] not in seen:
                    seen.add(command["id"])
                    print(f"Solicitud firmada recibida: {command['payload']['publisher']}/{command['payload']['packageSlug']}. Usa `foundstore-agent pending` y `approve` para continuar.")
        except AgentError:
            delay = min(delay * 2, 120)
        cycles += 1
        if max_cycles is not None and cycles >= max_cycles:
            return
        sleep_fn(delay)


def resolve(config: dict, request_id: str, status: str, message: str | None = None) -> None:
    payload = {"deviceId": config["deviceId"], "requestId": request_id, "status": status}
    if message:
        payload["agentMessage"] = message[:500]
    request_json(f"{config['server']}/api/trpc/agent.resolve", payload, config["agentToken"])


def list_requests(_: argparse.Namespace) -> None:
    config = load_config()
    requests = pending(config)
    if not requests:
        print("No hay solicitudes pendientes de aprobación local.")
        return
    for entry in requests:
        print(f"{entry['id']}\n  Paquete: {entry['publisher']}/{entry['packageSlug']}\n  Versión: {entry.get('requestedVersion') or 'sin tag'}\n  Comando: {entry['command']}\n  Solicitado: {entry['requestedAt']}\n")


def approve(args: argparse.Namespace) -> None:
    config = load_config()
    requests = {entry["id"]: entry for entry in pending(config)}
    entry = requests.get(args.request_id)
    if not entry:
        fail("la solicitud no existe o ya fue revisada")
    print(f"Solicitud local: {entry['publisher']}/{entry['packageSlug']}")
    print(f"Se ejecutará: flut install {entry['publisher']}/{entry['packageSlug']}")
    confirmation = input("Escribe INSTALAR para confirmar: ").strip()
    if confirmation != "INSTALAR":
        print("Instalación cancelada localmente; la solicitud sigue pendiente.")
        return
    resolve(config, entry["id"], "approved", "Aprobada por la persona usuaria en Danenone")
    process = subprocess.run(["flut", "install", f"{entry['publisher']}/{entry['packageSlug']}"], text=True, capture_output=True, check=False)
    output = (process.stdout + "\n" + process.stderr).strip()
    if process.returncode == 0:
        resolve(config, entry["id"], "installed", output or "Instalación confirmada por flut")
        print("Paquete instalado correctamente.")
    else:
        resolve(config, entry["id"], "failed", output or f"flut terminó con código {process.returncode}")
        fail("flut no pudo instalar el paquete; el detalle se registró en Foundstore")


def reject(args: argparse.Namespace) -> None:
    config = load_config()
    resolve(config, args.request_id, "rejected", args.message or "Rechazada por la persona usuaria en Danenone")
    print("Solicitud rechazada localmente.")


def run_daemon(_: argparse.Namespace) -> None:
    daemon(load_config())


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Agente local de solicitudes Foundstore para Danenone")
    subcommands = parser.add_subparsers(dest="command", required=True)
    pair_parser = subcommands.add_parser("pair", help="Emparejar este Danenone con un código de Foundstore")
    pair_parser.add_argument("code", help="Código alfanumérico de 6 a 12 caracteres creado desde Mi espacio")
    pair_parser.add_argument("--server", required=True, help="URL HTTPS de Foundstore")
    pair_parser.add_argument("--name", default=os.uname().nodename, help="Nombre visible del Danenone")
    pair_parser.set_defaults(handler=pair)
    uri_parser = subcommands.add_parser("uri", help="Canjear una URI foundstore://agent/pair con código de un solo uso")
    uri_parser.add_argument("uri", help="URI proporcionada por Foundstore; no contiene el token del agente")
    uri_parser.add_argument("--name", default=os.uname().nodename, help="Nombre visible del Danenone")
    uri_parser.set_defaults(handler=pair_uri)
    list_parser = subcommands.add_parser("pending", help="Mostrar solicitudes pendientes; no instala nada")
    list_parser.set_defaults(handler=list_requests)
    approve_parser = subcommands.add_parser("approve", help="Pedir confirmación y ejecutar flut para una solicitud")
    approve_parser.add_argument("request_id")
    approve_parser.set_defaults(handler=approve)
    reject_parser = subcommands.add_parser("reject", help="Rechazar una solicitud sin instalar")
    reject_parser.add_argument("request_id")
    reject_parser.add_argument("--message")
    reject_parser.set_defaults(handler=reject)
    daemon_parser = subcommands.add_parser("daemon", help="Mantener la recepción de solicitudes firmadas con bajo consumo; nunca instala automáticamente")
    daemon_parser.set_defaults(handler=run_daemon)
    return parser


if __name__ == "__main__":
    arguments = build_parser().parse_args()
    try:
        arguments.handler(arguments)
    except AgentError as error:
        fail(str(error))
