#!/usr/bin/env python3
from __future__ import annotations

import json
import socket
import time
from pathlib import Path

SOCKET = '/home/ubuntu/danenone/build/danenone-qmp.sock'
OUT = '/home/ubuntu/danenone/build/danenone-screen.ppm'


def recv(sock):
    sock.settimeout(3)
    try:
        data = sock.recv(65536)
    except TimeoutError:
        return b''
    print(data.decode(errors='replace'))
    return data

with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
    sock.connect(SOCKET)
    recv(sock)
    for payload in (
        {'execute': 'qmp_capabilities'},
        {'execute': 'query-status'},
        {'execute': 'screendump', 'arguments': {'filename': OUT}},
    ):
        sock.sendall((json.dumps(payload) + '\r\n').encode())
        time.sleep(0.5)
        recv(sock)
print(Path(OUT).exists(), Path(OUT).stat().st_size if Path(OUT).exists() else 0)
