#!/usr/bin/env python3
from __future__ import annotations

import json
import socket
import sys
import time
from pathlib import Path

SOCKET = Path('/home/ubuntu/danenone/build/danenone-qmp.sock')
OUT = Path('/home/ubuntu/danenone/build/danenone-screen.ppm')


def command(sock: socket.socket, name: str, args: dict | None = None):
    payload = {'execute': name}
    if args:
        payload['arguments'] = args
    sock.sendall((json.dumps(payload) + '\r\n').encode())
    time.sleep(0.25)
    return sock.recv(65536)


def main():
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
        sock.connect(str(SOCKET))
        sock.recv(65536)
        sock.sendall(b'{"execute":"qmp_capabilities"}\r\n')
        time.sleep(0.2)
        sock.recv(65536)
        command(sock, 'screendump', {'filename': str(OUT), 'device': 'VGA'})
        # Resolución QEMU estándar 1024x768: clics demostrativos en Archivos y Centro de control.
        for x, y in ((512, 730), (900, 80), (512, 730)):
            command(sock, 'input-send-event', {'events': [
                {'type': 'abs', 'data': {'axis': 'x', 'value': x}},
                {'type': 'abs', 'data': {'axis': 'y', 'value': y}},
                {'type': 'btn', 'data': {'button': 'left', 'down': True}},
                {'type': 'btn', 'data': {'button': 'left', 'down': False}},
            ]})
            time.sleep(1)
            command(sock, 'screendump', {'filename': str(OUT.with_name(f'screen-{x}-{y}.ppm')), 'device': 'VGA'})


if __name__ == '__main__':
    try:
        main()
    except Exception as exc:
        print(f'QMP error: {exc}', file=sys.stderr)
        raise SystemExit(1)
