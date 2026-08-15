#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

from PyQt5.QtWidgets import QApplication
from PyQt5.QtCore import QTimer

ROOT = Path('/home/ubuntu/danenone')
OUT = ROOT / 'build' / 'shell-captures'
OUT.mkdir(parents=True, exist_ok=True)

from danenone_shell.app import InfluentWindow
from danenone_shell.freedesktop_notifications import DesktopNotification

app = QApplication(sys.argv)
window = InfluentWindow()
window.resize(1280, 800)
window.show()
app.processEvents()
window.grab().save(str(OUT / '01-desktop.png'))
window.open_file_manager()
app.processEvents()
window.file_manager.grab().save(str(OUT / '02-file-manager.png'))
window.file_manager.hide()
window.toggle_control()
app.processEvents()
window.control.grab().save(str(OUT / '03-control-center.png'))
window.handle_desktop_notification(DesktopNotification('Influent Danenone', 'Demo', 'Notificación desde el puente Freedesktop', expire_timeout=4000))
app.processEvents()
window.notification_banner.grab().save(str(OUT / '04-notification-banner.png'))
window.close()
app.quit()
print('\n'.join(str(path) for path in sorted(OUT.glob('*.png'))))
