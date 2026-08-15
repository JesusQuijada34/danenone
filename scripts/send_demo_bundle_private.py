#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
from pathlib import Path

import requests

ENV_PATH = Path('/home/ubuntu/upload/.env')
CHAT_ID = '7736662759'
ROOT = Path('/home/ubuntu/danenone')

FILES = [
    ('photo', ROOT / 'build/influent-danenone-long-ad-reference.png', 'Referencia visual de Influent Danenone'),
    ('photo', ROOT / 'build/packagemaker-long-ad-reference.png', 'Referencia visual de PackageMaker'),
    ('photo', ROOT / 'build/shell-captures/01-desktop.png', 'Captura real: escritorio'),
    ('photo', ROOT / 'build/shell-captures/02-file-manager.png', 'Captura real: Influent File Manager'),
    ('photo', ROOT / 'build/shell-captures/03-control-center.png', 'Captura real: Centro de control'),
    ('photo', ROOT / 'build/shell-captures/04-notification-banner.png', 'Captura real: notificación Slime'),
    ('document', ROOT / 'build/telegram-demo-prompts.md', 'Prompts y secuencia de demostración'),
    ('video', ROOT / 'build/influent-danenone-shell-demo.mp4', 'Demostración del shell con estados interactivos'),
    ('photo', ROOT / 'build/danenone-screen.png', 'VM: menú de arranque Influent Danenone'),
    ('photo', ROOT / 'build/danenone-after-wait.png', 'VM: Linux y servidor X inicializando'),
]


def load_env(path: Path) -> dict[str, str]:
    values = {}
    for raw in path.read_text(encoding='utf-8').splitlines():
        line = raw.strip()
        if line and not line.startswith('#') and '=' in line:
            key, value = line.split('=', 1)
            values[key.strip()] = value.strip().strip('"').strip("'")
    return values


def send(token: str, kind: str, path: Path, caption: str) -> dict:
    if not path.is_file():
        raise FileNotFoundError(path)
    endpoint = {'photo': 'sendPhoto', 'video': 'sendVideo', 'document': 'sendDocument'}[kind]
    field = kind
    with path.open('rb') as handle:
        response = requests.post(
            f'https://api.telegram.org/bot{token}/{endpoint}',
            data={'chat_id': CHAT_ID, 'caption': caption[:1024]},
            files={field: (path.name, handle, 'video/mp4' if kind == 'video' else None)},
            timeout=180,
        )
    response.raise_for_status()
    payload = response.json()
    if not payload.get('ok'):
        raise RuntimeError(payload.get('description', 'Telegram rechazó el archivo'))
    return payload.get('result', {})


def main() -> int:
    env = load_env(ENV_PATH)
    token = env.get('TELEGRAM_BOT_TOKEN') or os.getenv('TELEGRAM_BOT_TOKEN')
    if not token:
        raise RuntimeError('Falta TELEGRAM_BOT_TOKEN')
    sent = []
    for kind, path, caption in FILES:
        result = send(token, kind, path, caption)
        sent.append((kind, path.name, result.get('message_id', 'unknown')))
        print(f'sent {kind} {path.name} message_id={result.get("message_id", "unknown")}')
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except Exception as exc:
        token = locals().get('token', '')
        print(f'error: {str(exc).replace(token, "[REDACTED]")}', file=sys.stderr)
        raise SystemExit(1)
