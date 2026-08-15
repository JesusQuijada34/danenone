#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
from pathlib import Path

import requests

ENV_PATH = Path('/home/ubuntu/upload/.env')
VIDEO_PATH = Path('/home/ubuntu/danenone/build/influent-danenone-ad-8s.mp4')
CHAT_ID = '7736662759'
MAX_BYTES = 50 * 1024 * 1024


def load_env(path: Path) -> dict[str, str]:
    values = {}
    for raw in path.read_text(encoding='utf-8').splitlines():
        line = raw.strip()
        if line and not line.startswith('#') and '=' in line:
            key, value = line.split('=', 1)
            values[key.strip()] = value.strip().strip('"').strip("'")
    return values


def main() -> int:
    env = load_env(ENV_PATH)
    token = env.get('TELEGRAM_BOT_TOKEN') or os.getenv('TELEGRAM_BOT_TOKEN')
    if not token:
        raise RuntimeError('Falta TELEGRAM_BOT_TOKEN')
    if not VIDEO_PATH.is_file():
        raise FileNotFoundError(VIDEO_PATH)
    if VIDEO_PATH.stat().st_size > MAX_BYTES:
        raise ValueError('El video supera el límite de 50 MB')
    with VIDEO_PATH.open('rb') as video:
        response = requests.post(
            f'https://api.telegram.org/bot{token}/sendVideo',
            data={'chat_id': CHAT_ID, 'caption': 'Influent Danenone — anuncio promocional', 'supports_streaming': 'true'},
            files={'video': (VIDEO_PATH.name, video, 'video/mp4')},
            timeout=180,
        )
    response.raise_for_status()
    payload = response.json()
    if not payload.get('ok'):
        raise RuntimeError(payload.get('description', 'Telegram rechazó el envío'))
    print(f'video-sent message_id={payload.get("result", {}).get("message_id", "unknown")} chat_id={CHAT_ID}')
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except Exception as exc:
        token = locals().get('token', '')
        print(f'error: {str(exc).replace(token, "[REDACTED]")}', file=sys.stderr)
        raise SystemExit(1)
