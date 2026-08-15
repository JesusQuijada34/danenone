from __future__ import annotations

from datetime import datetime
from pathlib import Path

from PyQt5.QtCore import QEasingCurve, QPropertyAnimation, QRect, QSize, Qt, QTimer, pyqtSignal
from PyQt5.QtGui import QColor, QFont, QIcon, QPainter, QPainterPath, QPixmap
from PyQt5.QtWidgets import (
    QApplication, QFrame, QGridLayout, QHBoxLayout, QLabel, QMainWindow,
    QPushButton, QSizePolicy, QVBoxLayout, QWidget,
)

ROOT = Path(__file__).resolve().parents[3]
WALLPAPER = ROOT / "branding-v2" / "danenone-river-wallpaper.jpg"


def button(text: str, object_name: str) -> QPushButton:
    item = QPushButton(text)
    item.setObjectName(object_name)
    item.setCursor(Qt.PointingHandCursor)
    return item


class AcrylicPanel(QFrame):
    def __init__(self, parent=None, object_name="AcrylicPanel"):
        super().__init__(parent)
        self.setObjectName(object_name)
        self.setAttribute(Qt.WA_TranslucentBackground)


class Notch(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(260, 42)
        self.setObjectName("Notch")
        self.setToolTip("Influent Dynamic Island")


class ControlCenter(AcrylicPanel):
    closed = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent, "ControlCenter")
        self.setFixedSize(360, 420)
        root = QVBoxLayout(self)
        root.setContentsMargins(22, 20, 22, 22)
        title = QLabel("Centro de control")
        title.setObjectName("PanelTitle")
        root.addWidget(title)
        grid = QGridLayout()
        grid.setSpacing(12)
        for index, label in enumerate(("Wi‑Fi", "Bluetooth", "Sonido", "Brillo")):
            tile = button(label, "ControlTile")
            tile.setMinimumHeight(72)
            grid.addWidget(tile, index // 2, index % 2)
        root.addLayout(grid)
        root.addStretch()
        close = button("Cerrar", "SecondaryButton")
        close.clicked.connect(self.closed)
        root.addWidget(close)


class FileSurface(AcrylicPanel):
    closed = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent, "FileSurface")
        self.setFixedSize(760, 500)
        root = QVBoxLayout(self)
        root.setContentsMargins(24, 20, 24, 24)
        top = QHBoxLayout()
        title = QLabel("Archivos")
        title.setObjectName("PanelTitle")
        top.addWidget(title)
        top.addStretch()
        close = button("Cerrar", "SecondaryButton")
        close.clicked.connect(self.closed)
        top.addWidget(close)
        root.addLayout(top)
        body = QHBoxLayout()
        sidebar = QVBoxLayout()
        for item in ("Inicio", "Escritorio", "Documentos", "Descargas", "Imágenes"):
            sidebar.addWidget(button(item, "NavButton"))
        sidebar.addStretch()
        body.addLayout(sidebar, 1)
        content = QFrame()
        content.setObjectName("FileGrid")
        files = QGridLayout(content)
        files.setSpacing(14)
        for index, label in enumerate(("Documentos", "Descargas", "Imágenes", "Música", "Videos", "Proyecto")):
            card = button("▣\n" + label, "FileCard")
            card.setMinimumSize(130, 92)
            files.addWidget(card, index // 3, index % 3)
        body.addWidget(content, 4)
        root.addLayout(body)


class DanenoneShell(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Influent Danenone")
        self.setMinimumSize(1100, 700)
        self.setObjectName("DanenoneShell")
        self.control = ControlCenter(self)
        self.files = FileSurface(self)
        self.control.hide()
        self.files.hide()
        self._build_ui()
        self._apply_wallpaper()
        self._clock = QTimer(self)
        self._clock.timeout.connect(self._update_clock)
        self._clock.start(1000)
        self._update_clock()

    def _build_ui(self):
        central = QWidget()
        central.setObjectName("Desktop")
        self.setCentralWidget(central)
        root = QVBoxLayout(central)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(0)
        status = QHBoxLayout()
        status.setContentsMargins(26, 16, 26, 0)
        brand = QLabel("Influent Danenone")
        brand.setObjectName("Brand")
        status.addWidget(brand)
        status.addStretch()
        self.clock = QLabel()
        self.clock.setObjectName("Clock")
        status.addWidget(self.clock)
        root.addLayout(status)

        self.notch = Notch(central)
        self.notch.move((self.width() - self.notch.width()) // 2, 8)
        self.notch.raise_()

        workspace = QWidget()
        workspace_layout = QVBoxLayout(workspace)
        workspace_layout.setContentsMargins(52, 68, 52, 24)
        workspace_layout.addStretch()
        apps = QGridLayout()
        apps.setHorizontalSpacing(20)
        apps.setVerticalSpacing(18)
        for index, name in enumerate(("Archivos", "FoundStore", "Ajustes", "Terminal", "Paquetes", "Fotos")):
            item = button("◈\n" + name, "AppTile")
            item.setMinimumSize(138, 112)
            if name == "Archivos":
                item.clicked.connect(self.show_files)
            apps.addWidget(item, index // 3, index % 3)
        workspace_layout.addLayout(apps)
        root.addWidget(workspace, 1)

        taskbar = AcrylicPanel(central, "Taskbar")
        taskbar.setFixedHeight(72)
        taskbar_layout = QHBoxLayout(taskbar)
        taskbar_layout.setContentsMargins(18, 12, 18, 12)
        start = button("◈", "StartButton")
        start.setFixedSize(48, 48)
        taskbar_layout.addWidget(start)
        taskbar_layout.addStretch()
        for name in ("◈", "▣", "◌"):
            taskbar_layout.addWidget(button(name, "TaskButton"))
        taskbar_layout.addStretch()
        control = button("☰", "TaskButton")
        control.clicked.connect(self.show_control)
        taskbar_layout.addWidget(control)
        root.addWidget(taskbar)

    def _apply_wallpaper(self):
        if WALLPAPER.exists():
            pixmap = QPixmap(str(WALLPAPER))
            self._wallpaper = pixmap
            self.centralWidget().update()
        self.centralWidget().setStyleSheet("#Desktop { background: rgba(8, 18, 30, 0.28); }")

    def paintEvent(self, event):
        super().paintEvent(event)
        if not hasattr(self, "_wallpaper"):
            return
        painter = QPainter(self)
        painter.setRenderHint(QPainter.SmoothPixmapTransform)
        target = self.centralWidget().geometry()
        scaled = self._wallpaper.scaled(target.size(), Qt.KeepAspectRatioByExpanding, Qt.SmoothTransformation)
        x = target.x() + (target.width() - scaled.width()) // 2
        y = target.y() + (target.height() - scaled.height()) // 2
        painter.drawPixmap(x, y, scaled)
        painter.fillRect(target, QColor(6, 15, 28, 86))

    def resizeEvent(self, event):
        super().resizeEvent(event)
        if hasattr(self, "notch"):
            self.notch.move((self.width() - self.notch.width()) // 2, 8)

    def _update_clock(self):
        self.clock.setText(datetime.now().strftime("%H:%M"))

    def _show_panel(self, panel):
        panel.adjustSize()
        panel.move(self.width() - panel.width() - 28, 76)
        panel.show()
        panel.raise_()
        animation = QPropertyAnimation(panel, b"windowOpacity", panel)
        animation.setDuration(220)
        animation.setStartValue(0.0)
        animation.setEndValue(1.0)
        animation.setEasingCurve(QEasingCurve.OutCubic)
        animation.start(QPropertyAnimation.DeleteWhenStopped)

    def show_control(self):
        self.files.hide()
        self._show_panel(self.control)

    def show_files(self):
        self.control.hide()
        self._show_panel(self.files)
