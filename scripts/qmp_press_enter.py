#!/usr/bin/env python3
from __future__ import annotations
import json, socket, time
SOCKET='/home/ubuntu/danenone/build/danenone-qmp.sock'
with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
    sock.connect(SOCKET)
    greeting=sock.recv(65536)
    print(greeting.decode(errors='replace'))
    for command in ({'execute':'qmp_capabilities'}, {'execute':'send-key','arguments':{'keys':[{'type':'qcode','data':'ret'}]}}):
        sock.sendall((json.dumps(command)+'\r\n').encode())
        time.sleep(.5)
        print(sock.recv(65536).decode(errors='replace'))
