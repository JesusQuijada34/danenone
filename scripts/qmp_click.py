#!/usr/bin/env python3
from __future__ import annotations

import json
import socket
import sys
import time
from pathlib import Path


def send(sock: socket.socket, name: str, arguments: dict | None = None) -> None:
    payload = {"execute": name}
    if arguments:
        payload["arguments"] = arguments
    sock.sendall((json.dumps(payload) + "\r\n").encode())
    time.sleep(0.35)
    sock.recv(65536)


def click(sock: socket.socket, x: int, y: int) -> None:
    send(sock, "input-send-event", {"events": [
        {"type": "abs", "data": {"axis": "x", "value": x}},
        {"type": "abs", "data": {"axis": "y", "value": y}},
        {"type": "btn", "data": {"button": "left", "down": True}},
        {"type": "btn", "data": {"button": "left", "down": False}},
    ]})


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: qmp_click.py SOCKET X Y [X Y ...]", file=sys.stderr)
        return 2
    socket_path = Path(sys.argv[1])
    coords = [int(value) for value in sys.argv[2:]]
    if len(coords) % 2:
        print("coordinates must be pairs", file=sys.stderr)
        return 2
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
        sock.connect(str(socket_path))
        sock.recv(65536)
        send(sock, "qmp_capabilities")
        for x, y in zip(coords[::2], coords[1::2]):
            click(sock, x, y)
            time.sleep(1.0)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
