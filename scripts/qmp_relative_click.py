#!/usr/bin/env python3
from __future__ import annotations

import json
import socket
import sys
import time


def send(sock: socket.socket, payload: dict) -> str:
    sock.sendall((json.dumps(payload) + "\r\n").encode())
    time.sleep(0.35)
    return sock.recv(65536).decode(errors="replace")


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: qmp_relative_click.py SOCKET DX DY", file=sys.stderr)
        return 2
    dx, dy = (int(value) for value in sys.argv[2:])
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
        sock.connect(sys.argv[1])
        sock.recv(65536)
        send(sock, {"execute": "qmp_capabilities"})
        send(sock, {"execute": "input-send-event", "arguments": {"events": [
            {"type": "rel", "data": {"axis": "x", "value": dx}},
            {"type": "rel", "data": {"axis": "y", "value": dy}},
            {"type": "btn", "data": {"button": "left", "down": True}},
            {"type": "btn", "data": {"button": "left", "down": False}},
        ]}})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
