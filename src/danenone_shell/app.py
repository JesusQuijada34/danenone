from __future__ import annotations

import sys
from datetime import datetime

try:
    from PyQt5.QtCore import Qt, QTimer
    from PyQt5.QtGui import QColor, QFont, QLinearGradient, QPainter, QPainterPath
    from PyQt5.QtWidgets import (
        QApplication,
        QFrame,
        QGraphicsDropShadowEffect,
        QHBoxLayout,
        QLabel,
        QPushButton,
        QStackedWidget,
        QVBoxLayout,
        QWidget,
    )
except ImportError as exc:  # pragma: no cover
    raise SystemExit("Influent Danenone necesita PyQt5 o PySide6 en la sesión gráfica") from exc


class AcrylicPanel(QFrame):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("AcrylicPanel")
        self.setAttribute(Qt.WA_TranslucentBackground)
        shadow = QGraphicsDropShadowEffect(self)
        shadow.setBlurRadius(28)
        shadow.setOffset(0, 10)
        shadow.setColor(QColor(0, 0, 0, 80))
        self.setGraphicsEffect(shadow)


class Notch(AcrylicPanel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(282, 48)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(18, 6, 18, 6)
        brand = QLabel("Influent")
        brand.setObjectName("BrandMark")
        layout.addWidget(brand)
        layout.addStretch()
        self.clock = QLabel()
        self.clock.setObjectName("Clock")
        layout.addWidget(self.clock)
        self.refresh_clock()
        timer = QTimer(self)
        timer.timeout.connect(self.refresh_clock)
        timer.start(1000)

    def refresh_clock(self):
        self.clock.setText(datetime.now().strftime("%H:%M"))


class ControlCenter(AcrylicPanel):
    def __init__(self, parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(18, 18, 18, 18)
        title = QLabel("Centro de control")
        title.setObjectName("PanelTitle")
        layout.addWidget(title)
        subtitle = QLabel("Influent Danenone")
        subtitle.setObjectName("Muted")
        layout.addWidget(subtitle)
        for label in ("Wi‑Fi  ·  Conectado", "Bluetooth  ·  Activado", "Modo oscuro  ·  Activado", "Brillo  ·  80%"):
            button = QPushButton(label)
            button.setCheckable(True)
            button.setChecked(True)
            layout.addWidget(button)
        layout.addStretch()


class DesktopPage(AcrylicPanel):
    def __init__(self, page: int, parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(56, 92, 56, 50)
        heading = QLabel("Influent Danenone")
        heading.setObjectName("PageHeading")
        layout.addWidget(heading)
        sub = QLabel(f"Página {page + 1}  ·  un escritorio compacto, claro y fluido")
        sub.setObjectName("Muted")
        layout.addWidget(sub)
        layout.addStretch()
        widget_row = QHBoxLayout()
        widget_row.setSpacing(16)
        for icon, label, detail in (
            ("▦", "Launchpad", "Aplicaciones"),
            ("▤", "Archivos", "Explorador"),
            ("◉", "FoundStore", "Aplicaciones Fluthin"),
            ("⚙", "Ajustes", "Sistema"),
        ):
            card = QPushButton(f"{icon}\n{label}\n{detail}")
            card.setObjectName("AppCard")
            card.setMinimumSize(156, 112)
            widget_row.addWidget(card)
        layout.addLayout(widget_row)


class DanenoneWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Influent Danenone")
        self.resize(1280, 800)
        self.setMinimumSize(960, 600)
        self.setObjectName("Root")
        root = QVBoxLayout(self)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(0)

        self.pages = QStackedWidget()
        for page in range(4):
            self.pages.addWidget(DesktopPage(page))
        root.addWidget(self.pages, 1)

        self.notch = Notch(self)
        self.notch.raise_()
        self.control = ControlCenter(self)
        self.control.setFixedWidth(300)
        self.control.hide()

        taskbar = AcrylicPanel()
        taskbar.setFixedHeight(86)
        bar = QHBoxLayout(taskbar)
        bar.setContentsMargins(30, 12, 30, 12)
        launch = QPushButton("◈")
        launch.setObjectName("DockButton")
        launch.setToolTip("Launchpad")
        launch.clicked.connect(lambda: self.pages.setCurrentIndex((self.pages.currentIndex() + 1) % self.pages.count()))
        bar.addWidget(launch)
        bar.addStretch()
        for text, tip in (("◌", "Ventanas"), ("◍", "Archivos"), ("▣", "FoundStore")):
            button = QPushButton(text)
            button.setObjectName("DockButton")
            button.setToolTip(tip)
            button.setFixedSize(52, 52)
            bar.addWidget(button)
        control = QPushButton("⌁")
        control.setObjectName("DockButton")
        control.setToolTip("Centro de control")
        control.setFixedSize(52, 52)
        control.clicked.connect(self.toggle_control_center)
        bar.addWidget(control)
        root.addWidget(taskbar)
        self.dots = QLabel("●  ○  ○  ○")
        self.dots.setAlignment(Qt.AlignCenter)
        self.dots.setObjectName("Dots")
        root.addWidget(self.dots)
        self.pages.currentChanged.connect(lambda index: self.dots.setText("  ".join("●" if i == index else "○" for i in range(self.pages.count()))))
        self.apply_theme()

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self.notch.move((self.width() - self.notch.width()) // 2, 12)
        self.control.move(self.width() - self.control.width() - 22, 72)

    def toggle_control_center(self):
        self.control.setVisible(not self.control.isVisible())
        if self.control.isVisible():
            self.control.raise_()

    def apply_theme(self):
        self.setStyleSheet("""
        #Root { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #080f24, stop:0.48 #172c59, stop:1 #4b2b68); color: #f8faff; }
        #AcrylicPanel { background: rgba(20, 30, 58, 0.80); border: 1px solid rgba(255,255,255,0.18); border-radius: 24px; }
        QPushButton { color: #f8faff; background: rgba(255,255,255,0.10); border: 1px solid rgba(255,255,255,0.13); border-radius: 17px; padding: 9px 14px; }
        QPushButton:hover { background: rgba(255,255,255,0.21); }
        QPushButton:checked { background: rgba(118, 190, 255, 0.38); }
        #AppCard { font-size: 13px; line-height: 1.4; }
        #DockButton { font-size: 22px; border-radius: 18px; }
        #PageHeading { font-size: 34px; font-weight: 600; }
        #PanelTitle { font-size: 19px; font-weight: 600; }
        #BrandMark { font-size: 13px; font-weight: 700; color: #ff887f; }
        #Clock { font-size: 12px; font-weight: 600; }
        #Muted, #Dots { color: rgba(255,255,255,0.68); }
        QLabel { background: transparent; }
        """)


def main() -> int:
    app = QApplication(sys.argv)
    app.setApplicationName("Influent Danenone")
    app.setOrganizationName("Influent")
    app.setFont(QFont("DejaVu Sans", 10))
    window = DanenoneWindow()
    window.show()
    return app.exec_()


if __name__ == "__main__":
    raise SystemExit(main())
