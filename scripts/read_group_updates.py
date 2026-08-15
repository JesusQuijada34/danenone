#!/usr/bin/env python3
from __future__ import annotations

import json
import os
from pathlib import Path

import requests

ENV_FILE = Path('/home/ubuntu/upload/.env')
OUTPUT = Path('/home/ubuntu/danenone/build/group-updates.json')
GROUP = '@jq34_ghgroup'


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
    token = load_token()
    response = requests.get(f'https://api.telegram.org/bot{token}/getUpdates', params={'limit': 100, 'timeout': 0}, timeout=60)
    response.raise_for_status()
    payload = response.json()
    if not payload.get('ok'):
        raise RuntimeError(payload.get('description', 'Telegram rechazó getUpdates'))
    selected = []
    for update in payload.get('result', []):
        message = update.get('message') or update.get('channel_post') or update.get('edited_message')
        if not message:
            continue
        chat = message.get('chat', {})
        chat_id = str(chat.get('id', ''))
        username = chat.get('username', '')
        if username == GROUP.lstrip('@') or chat_id == GROUP:
            selected.append({
                'update_id': update.get('update_id'),
                'message_id': message.get('message_id'),
                'date': message.get('date'),
                'chat': {'id': chat_id, 'type': chat.get('type'), 'username': username, 'title': chat.get('title')},
                'from': {'id': message.get('from', {}).get('id'), 'username': message.get('from', {}).get('username')},
                'text': message.get('text') or message.get('caption') or '',
            })
    OUTPUT.write_text(json.dumps(selected, ensure_ascii=False, indent=2), encoding='utf-8')
    print(f'group_messages={len(selected)} output={OUTPUT}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
