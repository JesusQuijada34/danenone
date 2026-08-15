#!/usr/bin/env python3
from __future__ import annotations

import json
import socket
import time
from pathlib import Path

SOCKET = Path('/home/ubuntu/danenone/build/danenone-v2-qmp.sock')
OUTPUT = Path('/home/ubuntu/danenone/build/influent-danenone-v2-qemu.ppm')

with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
    sock.connect(str(SOCKET))
    sock.recv(65536)
    for command in ({'execute': 'qmp_capabilities'}, {'execute': 'screendump', 'arguments': {'filename': str(OUTPUT)}}):
        sock.sendall((json.dumps(command) + '\r\n').encode())
        time.sleep(0.8)
        sock.recv(65536)
print(OUTPUT)
