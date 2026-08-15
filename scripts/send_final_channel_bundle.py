#!/usr/bin/env python3
from __future__ import annotations

import os
from pathlib import Path

import requests

ENV_FILE = Path('/home/ubuntu/upload/.env')
BASE = Path('/home/ubuntu/danenone')
CHAT_ID = '@jq34_ghchannel'
IMAGE = BASE / 'build/danenone-promo-wallpaper.png'
SPECS = BASE / 'build/influent-danenone-0.3.0-specifications.md'
NOTES = BASE / 'build/influent-danenone-0.3.0-release-notes.md'


def load_token() -> str:
    for line in ENV_FILE.read_text(encoding='utf-8').splitlines():
        line = line.strip()
        if line.startswith('TELEGRAM_BOT_TOKEN='):
            value = line.split('=', 1)[1].strip().strip('"').strip("'")
            if value:
                return value
    value = os.environ.get('TELEGRAM_BOT_TOKEN', '')
    if not value:
        raise RuntimeError('TELEGRAM_BOT_TOKEN no está configurado')
    return value


def call(token: str, method: str, *, data: dict[str, str], files: dict[str, object] | None = None) -> int:
    response = requests.post(f'https://api.telegram.org/bot{token}/{method}', data=data, files=files, timeout=120)
    response.raise_for_status()
    payload = response.json()
    if not payload.get('ok'):
        raise RuntimeError(payload.get('description', f'Telegram rechazó {method}'))
    return int(payload['result']['message_id'])


def main() -> int:
    token = load_token()
    with IMAGE.open('rb') as image:
        image_id = call(token, 'sendPhoto', data={
            'chat_id': CHAT_ID,
            'caption': '*Influent Danenone 0.3.0*\n\nFondo oficial con el arroyo, el cubo en el agua y la identidad Danenone. La ISO, el código y las especificaciones están en el release público:\nhttps://github.com/JesusQuijada34/danenone/releases/tag/v0.3.0',
            'parse_mode': 'Markdown',
        }, files={'photo': ('danenone-promo-wallpaper.png', image, 'image/png')})
    with SPECS.open('rb') as specs:
        specs_id = call(token, 'sendDocument', data={'chat_id': CHAT_ID, 'caption': 'Especificaciones públicas de Influent Danenone 0.3.0'}, files={'document': ('influent-danenone-0.3.0-specifications.md', specs, 'text/markdown')})
    with NOTES.open('rb') as notes:
        notes_id = call(token, 'sendDocument', data={'chat_id': CHAT_ID, 'caption': 'Notas de release, checksums y pasos de ejecución'}, files={'document': ('influent-danenone-0.3.0-release-notes.md', notes, 'text/markdown')})
    print(f'published chat={CHAT_ID} image_message={image_id} specs_message={specs_id} notes_message={notes_id}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
