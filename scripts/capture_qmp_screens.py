#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import socket
import time
from pathlib import Path


def qmp_command(sock: socket.socket, name: str, arguments: dict | None = None) -> None:
    payload: dict[str, object] = {"execute": name}
    if arguments:
        payload["arguments"] = arguments
    sock.sendall((json.dumps(payload) + "\r\n").encode())
    time.sleep(0.25)
    sock.recv(65536)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("socket", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("label")
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    output = args.output_dir / f"{args.label}.ppm"
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
        sock.connect(str(args.socket))
        sock.recv(65536)
        qmp_command(sock, "qmp_capabilities")
        qmp_command(sock, "screendump", {"filename": str(output)})
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
