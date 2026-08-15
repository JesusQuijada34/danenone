from __future__ import annotations

import os
from pathlib import Path

from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QGridLayout, QHBoxLayout, QLabel, QLineEdit, QListWidget, QListWidgetItem, QPushButton, QSplitter, QVBoxLayout, QWidget

from influent_ui import InfluentCard, InfluentButton, InfluentInput


class FileManager(QWidget):
    def __init__(self, root: Path | None = None, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Archivos")
        self.resize(960, 640)
        self.current = (root or Path.home()).expanduser().resolve()
        main = QVBoxLayout(self)
        toolbar = QHBoxLayout()
        self.back = InfluentButton("Atrás", "files")
        self.back.clicked.connect(self.go_back)
        self.path = InfluentInput(str(self.current))
        self.path.returnPressed.connect(self.go_to_path)
        self.search = InfluentInput("Buscar en esta carpeta")
        self.search.textChanged.connect(self.refresh)
        toolbar.addWidget(self.back)
        toolbar.addWidget(self.path, 1)
        toolbar.addWidget(self.search)
        main.addLayout(toolbar)
        self.list = QListWidget()
        self.list.setViewMode(QListWidget.IconMode)
        self.list.setResizeMode(QListWidget.Adjust)
        self.list.itemDoubleClicked.connect(self.open_item)
        main.addWidget(self.list)
        self.refresh()

    def refresh(self):
        self.list.clear()
        query = self.search.text().strip().lower()
        try:
            entries = sorted(self.current.iterdir(), key=lambda item: (not item.is_dir(), item.name.lower()))
        except OSError as exc:
            self.list.addItem(QListWidgetItem(f"No se puede leer la carpeta: {exc}"))
            return
        for entry in entries:
            if query and query not in entry.name.lower():
                continue
            item = QListWidgetItem(("[Carpeta] " if entry.is_dir() else "[Archivo] ") + entry.name)
            item.setData(Qt.UserRole, str(entry))
            self.list.addItem(item)

    def go_back(self):
        parent = self.current.parent
        if parent != self.current:
            self.current = parent
            self.path.setText(str(self.current))
            self.refresh()

    def go_to_path(self):
        candidate = Path(self.path.text()).expanduser().resolve()
        if candidate.is_dir():
            self.current = candidate
            self.refresh()
        else:
            self.path.setText(str(self.current))

    def open_item(self, item: QListWidgetItem):
        candidate = Path(item.data(Qt.UserRole))
        if candidate.is_dir():
            self.current = candidate
            self.path.setText(str(candidate))
            self.refresh()
        elif os.access(candidate, os.R_OK):
            self.statusBarMessage(f"Archivo seleccionado: {candidate.name}")

    def statusBarMessage(self, message: str):
        self.setWindowTitle(f"Archivos — {message}")
