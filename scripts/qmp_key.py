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
    if len(sys.argv) < 3:
        print("usage: qmp_key.py SOCKET KEY [KEY ...]", file=sys.stderr)
        return 2
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
        sock.connect(sys.argv[1])
        sock.recv(65536)
        send(sock, {"execute": "qmp_capabilities"})
        for key in sys.argv[2:]:
            pieces = key.lower().split("+")
            mapped = {"enter": "ret", "return": "ret", "tab": "tab", "shift": "shift", "ctrl": "ctrl"}
            response = send(sock, {"execute": "send-key", "arguments": {"keys": [
                {"type": "qcode", "data": mapped.get(piece, piece)} for piece in pieces
            ]}})
            print(response, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
