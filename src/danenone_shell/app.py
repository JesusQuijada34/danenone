from __future__ import annotations

import sys
from pathlib import Path

try:
    from PyQt5.QtCore import QEasingCurve, QPropertyAnimation, Qt, QTimer
    from PyQt5.QtGui import QColor, QFont, QPainter, QLinearGradient
    from PyQt5.QtWidgets import QApplication, QFrame, QHBoxLayout, QLabel, QPushButton, QStackedWidget, QVBoxLayout, QWidget
except ImportError as exc:  # pragma: no cover
    raise SystemExit("Danenone necesita PyQt5 o PySide6 en la sesión gráfica") from exc


class AcrylicPanel(QFrame):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("AcrylicPanel")
        self.setAttribute(Qt.WA_TranslucentBackground)


class Notch(AcrylicPanel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(230, 38)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(16, 4, 16, 4)
        layout.addWidget(QLabel("Danenone"))
        layout.addStretch()
        layout.addWidget(QLabel("⌁  09:41"))


class ControlCenter(AcrylicPanel):
    def __init__(self, parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)
        title = QLabel("Centro de control")
        title.setObjectName("PanelTitle")
        layout.addWidget(title)
        for label in ("Wi‑Fi  ·  Conectado", "Bluetooth  ·  Activado", "Modo oscuro  ·  Activado", "Brillo  ·  80%"):
            button = QPushButton(label)
            button.setCheckable(True)
            button.setChecked(True)
            layout.addWidget(button)


class DesktopPage(AcrylicPanel):
    def __init__(self, page: int, parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(48, 82, 48, 96)
        heading = QLabel(f"Página {page + 1}")
        heading.setObjectName("PageHeading")
        layout.addWidget(heading)
        sub = QLabel("Tus aplicaciones y widgets, organizados horizontalmente")
        sub.setObjectName("Muted")
        layout.addWidget(sub)
        layout.addStretch()
        widget_row = QHBoxLayout()
        for icon, label in (("▦", "Launchpad"), ("▤", "Archivos"), ("◉", "FoundStore"), ("⚙", "Ajustes")):
            card = QPushButton(f"{icon}\n{label}")
            card.setMinimumSize(132, 92)
            widget_row.addWidget(card)
        layout.addLayout(widget_row)


class DanenoneWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Danenone")
        self.resize(1280, 800)
        self.setMinimumSize(960, 600)
        self.setObjectName("Root")
        root = QVBoxLayout(self)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(0)

        self.pages = QStackedWidget()
        self.pages.addWidget(DesktopPage(0))
        self.pages.addWidget(DesktopPage(1))
        self.pages.addWidget(DesktopPage(2))
        root.addWidget(self.pages, 1)

        self.notch = Notch(self)
        self.notch.raise_()
        self.control = ControlCenter(self)
        self.control.setFixedWidth(280)
        self.control.hide()

        taskbar = AcrylicPanel()
        taskbar.setFixedHeight(78)
        bar = QHBoxLayout(taskbar)
        bar.setContentsMargins(28, 12, 28, 12)
        launch = QPushButton("◈")
        launch.setToolTip("Launchpad")
        launch.clicked.connect(lambda: self.pages.setCurrentIndex((self.pages.currentIndex() + 1) % self.pages.count()))
        bar.addWidget(launch)
        bar.addStretch()
        for text in ("◌", "◍", "▣"):
            button = QPushButton(text)
            button.setFixedSize(48, 48)
            bar.addWidget(button)
        control = QPushButton("⌁")
        control.setFixedSize(48, 48)
        control.clicked.connect(self.toggle_control_center)
        bar.addWidget(control)
        root.addWidget(taskbar)
        self.dots = QLabel("●  ○  ○")
        self.dots.setAlignment(Qt.AlignCenter)
        self.dots.setObjectName("Dots")
        root.addWidget(self.dots)
        self.pages.currentChanged.connect(lambda index: self.dots.setText("  ".join("●" if i == index else "○" for i in range(self.pages.count()))))
        self.apply_theme()

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self.notch.move((self.width() - self.notch.width()) // 2, 10)
        self.control.move(self.width() - self.control.width() - 22, 62)

    def toggle_control_center(self):
        if self.control.isVisible():
            self.control.hide()
            return
        self.control.show()
        self.control.raise_()

    def apply_theme(self):
        self.setStyleSheet("""
        #Root { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #101827, stop:0.52 #243755, stop:1 #5b4672); color: #f7f9ff; }
        #AcrylicPanel { background: rgba(24, 32, 52, 0.76); border: 1px solid rgba(255,255,255,0.16); border-radius: 22px; }
        QPushButton { color: #f7f9ff; background: rgba(255,255,255,0.10); border: 1px solid rgba(255,255,255,0.12); border-radius: 16px; padding: 8px 14px; }
        QPushButton:hover { background: rgba(255,255,255,0.20); }
        QPushButton:checked { background: rgba(123, 192, 255, 0.34); }
        #PageHeading { font-size: 32px; font-weight: 600; }
        #PanelTitle { font-size: 18px; font-weight: 600; }
        #Muted, #Dots { color: rgba(255,255,255,0.68); }
        QLabel { background: transparent; }
        """)


def main() -> int:
    app = QApplication(sys.argv)
    app.setApplicationName("Danenone")
    app.setFont(QFont("Inter", 10))
    window = DanenoneWindow()
    window.show()
    return app.exec_()


if __name__ == "__main__":
    raise SystemExit(main())
