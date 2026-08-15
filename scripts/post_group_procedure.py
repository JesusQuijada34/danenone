#!/usr/bin/env python3
from __future__ import annotations

import os
from pathlib import Path

import requests

ENV_FILE = Path('/home/ubuntu/upload/.env')
CONTENT_FILE = Path('/home/ubuntu/danenone/build/danenone-group-procedure.md')
CHAT_ID = '@jq34_ghgroup'


def load_token() -> str:
    for line in ENV_FILE.read_text(encoding='utf-8').splitlines():
        line = line.strip()
        if line.startswith('TELEGRAM_BOT_TOKEN='):
            token = line.split('=', 1)[1].strip().strip('"').strip("'")
            if token:
                return token
    token = os.environ.get('TELEGRAM_BOT_TOKEN', '')
    if not token:
        raise RuntimeError('TELEGRAM_BOT_TOKEN no está configurado')
    return token


def main() -> int:
    token = load_token()
    text = CONTENT_FILE.read_text(encoding='utf-8')
    if len(text) > 4096:
        raise RuntimeError('El procedimiento supera el límite de texto de Telegram')
    response = requests.post(
        f'https://api.telegram.org/bot{token}/sendMessage',
        data={
            'chat_id': CHAT_ID,
            'text': text,
            'parse_mode': 'Markdown',
            'disable_web_page_preview': 'true',
        },
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
