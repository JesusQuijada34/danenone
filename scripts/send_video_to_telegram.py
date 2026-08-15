#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
from pathlib import Path

import requests

ENV_PATH = Path('/home/ubuntu/upload/.env')
VIDEO_PATH = Path('/home/ubuntu/danenone/build/influent-danenone-ad-8s.mp4')
MAX_BYTES = 50 * 1024 * 1024


def load_env(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for raw in path.read_text(encoding='utf-8').splitlines():
        line = raw.strip()
        if not line or line.startswith('#') or '=' not in line:
            continue
        key, value = line.split('=', 1)
        values[key.strip()] = value.strip().strip('"').strip("'")
    return values


def main() -> int:
    env = load_env(ENV_PATH)
    token = env.get('TELEGRAM_BOT_TOKEN') or os.getenv('TELEGRAM_BOT_TOKEN')
    chat_id = '@jq34_ghchannel'
    if not token:
        raise RuntimeError('Falta TELEGRAM_BOT_TOKEN')
    if not chat_id:
        raise RuntimeError('Falta TELEGRAM_CHANNEL_ID')
    if not VIDEO_PATH.is_file():
        raise FileNotFoundError(VIDEO_PATH)
    size = VIDEO_PATH.stat().st_size
    if size > MAX_BYTES:
        raise ValueError(f'El video supera el límite de 50 MB: {size} bytes')

    url = f'https://api.telegram.org/bot{token}/sendVideo'
    caption = '*Influent Danenone*\n\nAnuncio promocional del nuevo sistema operativo.'
    session = requests.Session()
    session.trust_env = False
    with VIDEO_PATH.open('rb') as video:
        response = session.post(
            url,
            data={
                'chat_id': '@jq34_ghchannel',
                'caption': caption,
                'parse_mode': 'Markdown',
                'supports_streaming': 'true',
            },
            files={'video': (VIDEO_PATH.name, video, 'video/mp4')},
            timeout=180,
        )
    response.raise_for_status()
    payload = response.json()
    if not payload.get('ok'):
        raise RuntimeError(payload.get('description', 'Telegram rechazó el envío'))
    message_id = payload.get('result', {}).get('message_id', 'desconocido')
    print(f'video-sent message_id={message_id} chat_id=@jq34_ghchannel')
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except Exception as exc:
        safe_error = str(exc).replace(token if 'token' in locals() else '', '[REDACTED]')
        print(f'error: {safe_error}', file=sys.stderr)
        raise SystemExit(1)
