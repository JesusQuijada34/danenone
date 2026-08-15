from __future__ import annotations

import os
import sys
from pathlib import Path

from PyQt5.QtCore import QTimer
from PyQt5.QtWidgets import QApplication

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "build" / "captures"
OUT.mkdir(parents=True, exist_ok=True)

from danenone_shell.app import InfluentWindow
from danenone_shell.notifications import NotificationStore

app = QApplication(sys.argv)
window = InfluentWindow()
window.resize(1440, 900)
window.show()

store = NotificationStore(OUT / "capture-notifications.json")
store.clear_all()
store.push("Actualización disponible", "Influent Updater encontró una versión verificada.", "Influent Updater", duration=6)
store.push("Paquete instalado", "El paquete Fluthin se registró correctamente.", "FoundStore", duration=6)


def capture():
    window.grab().save(str(OUT / "01-desktop.png"))
    window.toggle_control()
    app.processEvents()
    window.grab().save(str(OUT / "02-control-center.png"))
    window.toggle_control()
    window.notifications_panel.show()
    window.notifications_panel.raise_()
    window.notifications_panel.refresh()
    app.processEvents()
    window.grab().save(str(OUT / "03-notifications.png"))
    window.notifications_panel.hide()
    window.toggle_editing()
    app.processEvents()
    window.grab().save(str(OUT / "04-editing-mode.png"))
    app.quit()

QTimer.singleShot(1200, capture)
sys.exit(app.exec_())
