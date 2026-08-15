#!/usr/bin/env python3
from __future__ import annotations

import os
from pathlib import Path

import requests

ENV_FILE = Path('/home/ubuntu/upload/.env')
CONTENT_FILE = Path('/home/ubuntu/danenone/build/danenone-final-group-update.md')
CHAT_ID = '@jq34_ghgroup'


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


def main() -> int:
    response = requests.post(
        f'https://api.telegram.org/bot{load_token()}/sendMessage',
        data={'chat_id': CHAT_ID, 'text': CONTENT_FILE.read_text(encoding='utf-8'), 'parse_mode': 'Markdown'},
        timeout=60,
    )
    response.raise_for_status()
    payload = response.json()
    if not payload.get('ok'):
        raise RuntimeError(payload.get('description', 'Telegram rechazó la publicación'))
    print(f"published chat={CHAT_ID} message_id={payload['result']['message_id']}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
