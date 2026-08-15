#!/usr/bin/env python3
from __future__ import annotations
import json, socket, time
SOCKET='/home/ubuntu/danenone/build/danenone-qmp.sock'
with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
    sock.connect(SOCKET)
    sock.recv(65536)
    for command in ({'execute':'qmp_capabilities'}, {'execute':'send-key','arguments':{'keys':[{'type':'qcode','data':'RET'}]}}):
        sock.sendall((json.dumps(command)+'\r\n').encode())
        time.sleep(.4)
        sock.recv(65536)
time.sleep(1)
