#!/usr/bin/env python3
from __future__ import annotations

import json
import socket
import sys
import time


def command(sock: socket.socket, name: str, arguments: dict | None = None) -> str:
    payload = {"execute": name}
    if arguments:
        payload["arguments"] = arguments
    sock.sendall((json.dumps(payload) + "\r\n").encode())
    time.sleep(0.4)
    return sock.recv(65536).decode(errors="replace")


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: qmp_add_tablet.py SOCKET", file=sys.stderr)
        return 2
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
        sock.connect(sys.argv[1])
        sock.recv(65536)
        print(command(sock, "qmp_capabilities"), end="")
        print(command(sock, "device_add", {"driver": "usb-tablet", "id": "danenone-tablet"}), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
