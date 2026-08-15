#!/usr/bin/env python3
from __future__ import annotations

import os
from pathlib import Path

import requests

ENV_FILE = Path('/home/ubuntu/upload/.env')
CHAT_ID = '@jq34_ghgroup'
REPLY_TO = '1801'
TEXT = "*Decisión de diseño confirmada*\n\nEl notch se conserva. También se mantiene el Centro de control inspirado en iOS, integrado con la barra inferior y las safe areas de Hyprland.\n\nLa nueva rama nativa en C ya reserva el espacio superior para el notch y prepara las capas Wayland para que la barra y los paneles translúcidos no se dibujen detrás de él. La ISO Arch se encuentra en fase de validación final; publicaré el resultado y sus checksums cuando termine la prueba de arranque."


def token() -> str:
    for line in ENV_FILE.read_text(encoding='utf-8').splitlines():
        if line.strip().startswith('TELEGRAM_BOT_TOKEN='):
            return line.split('=', 1)[1].strip().strip('"').strip("'")
    value = os.environ.get('TELEGRAM_BOT_TOKEN', '')
    if not value:
        raise RuntimeError('TELEGRAM_BOT_TOKEN no está configurado')
    return value


response = requests.post(
    f'https://api.telegram.org/bot{token()}/sendMessage',
    data={'chat_id': CHAT_ID, 'text': TEXT, 'parse_mode': 'Markdown', 'reply_to_message_id': REPLY_TO},
    timeout=60,
)
response.raise_for_status()
payload = response.json()
if not payload.get('ok'):
    raise RuntimeError(payload.get('description', 'Telegram rechazó la respuesta'))
print(f"replied chat={CHAT_ID} reply_to={REPLY_TO} message_id={payload['result']['message_id']}")
