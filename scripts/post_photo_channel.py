#!/usr/bin/env python3
from __future__ import annotations

import os
from pathlib import Path

import requests

ENV_FILE = Path("/home/ubuntu/upload/.env")
PHOTO_FILE = Path("/home/ubuntu/danenone/build/oobe-reference-local.png")
CAPTION_FILE = Path("/home/ubuntu/danenone/build/danenone-photo-telegram-caption.md")
CHAT_ID = "@jq34_ghchannel"


def load_token() -> str:
    for line in ENV_FILE.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line.startswith("TELEGRAM_BOT_TOKEN="):
            value = line.split("=", 1)[1].strip().strip('"').strip("'")
            if value:
                return value
    value = os.environ.get("TELEGRAM_BOT_TOKEN", "")
    if not value:
        raise RuntimeError("TELEGRAM_BOT_TOKEN no está configurado")
    return value


def main() -> int:
    token = load_token()
    caption = CAPTION_FILE.read_text(encoding="utf-8").strip()
    if len(caption) > 1024:
        raise RuntimeError("La leyenda de Telegram supera 1024 caracteres")
    with PHOTO_FILE.open("rb") as photo:
        response = requests.post(
            f"https://api.telegram.org/bot{token}/sendPhoto",
            data={"chat_id": CHAT_ID, "caption": caption, "parse_mode": "Markdown"},
            files={"photo": (PHOTO_FILE.name, photo, "image/png")},
            timeout=90,
        )
    response.raise_for_status()
    payload = response.json()
    if not payload.get("ok"):
        raise RuntimeError(payload.get("description", "Telegram rechazó la imagen"))
    print(f"published_photo chat={CHAT_ID} message_id={payload['result']['message_id']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
