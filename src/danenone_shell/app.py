from __future__ import annotations

import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path

from .hardware_status import HardwareStatus, read_hardware_status
from .icon_factory import svg_icon
from .notification_panel import NotificationPanel

try:
    from PyQt5.QtCore import QEasingCurve, QPropertyAnimation, Qt, QTimer, pyqtSignal, QSize
    from PyQt5.QtGui import QColor, QFont, QPainter, QPainterPath, QPixmap
    from PyQt5.QtWidgets import (
        QApplication,
        QFrame,
        QGraphicsDropShadowEffect,
        QGridLayout,
        QHBoxLayout,
        QLabel,
        QPushButton,
        QSlider,
        QStackedWidget,
        QVBoxLayout,
        QWidget,
    )
except ImportError as exc:  # pragma: no cover
    raise SystemExit("Influent Danenone necesita PyQt5 o PySide6 en la sesión gráfica") from exc

ROOT = Path(__file__).resolve().parents[2]
WALLPAPER = ROOT / "branding" / "influent-stream-wallpaper.png"


class GlassPanel(QFrame):
    def __init__(self, parent=None, object_name="GlassPanel"):
        super().__init__(parent)
        self.setObjectName(object_name)
        self.setAttribute(Qt.WA_TranslucentBackground)
        shadow = QGraphicsDropShadowEffect(self)
        shadow.setBlurRadius(24)
        shadow.setOffset(0, 8)
        shadow.setColor(QColor(0, 0, 0, 95))
        self.setGraphicsEffect(shadow)


class StatusBar(QWidget):
    notifications_requested = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedHeight(52)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(24, 9, 24, 7)
        self.left = QLabel("Influent Danenone")
        self.left.setObjectName("StatusBrand")
        self.clock = QLabel()
        self.clock.setObjectName("StatusClock")
        self.state = QLabel("Wi-Fi   Batería")
        self.state.setObjectName("StatusState")
        self.notifications = QPushButton()
        self.notifications.setIcon(svg_icon("control", accent="#A77BFF"))
        self.notifications.setIconSize(QSize(22, 22))
        self.notifications.setObjectName("StatusNotificationButton")
        self.notifications.setToolTip("Notificaciones")
        self.notifications.clicked.connect(self.notifications_requested)
        layout.addWidget(self.left)
        layout.addStretch()
        layout.addWidget(self.clock)
        layout.addSpacing(18)
        layout.addWidget(self.state)
        layout.addWidget(self.notifications)
        timer = QTimer(self)
        timer.timeout.connect(self.update_clock)
        timer.start(1000)
        self.update_clock()

    def update_clock(self):
        self.clock.setText(datetime.now().strftime("%H:%M"))


class Notch(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(242, 42)
        self.setAttribute(Qt.WA_TransparentForMouseEvents)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        path = QPainterPath()
        path.moveTo(22, 0)
        path.cubicTo(30, 0, 30, 28, 54, 34)
        path.cubicTo(83, 41, 159, 41, 188, 34)
        path.cubicTo(212, 28, 212, 0, 220, 0)
        path.lineTo(242, 0)
        path.lineTo(242, 42)
        path.lineTo(0, 42)
        path.lineTo(0, 0)
        path.closeSubpath()
        painter.fillPath(path, QColor(5, 8, 17, 245))
        painter.setPen(QColor(255, 255, 255, 35))
        painter.drawArc(30, 22, 182, 30, 205 * 16, 130 * 16)


class AppIcon(QPushButton):
    removed = pyqtSignal(object)

    def __init__(self, icon_name: str, title: str, removable: bool, parent=None):
        super().__init__(parent)
        self.title = title
        self.removable = removable
        self.editing = False
        self.long_press = QTimer(self)
        self.long_press.setSingleShot(True)
        self.long_press.timeout.connect(self.begin_editing)
        self.setObjectName("AppIcon")
        self.setText(title)
        self.setIcon(svg_icon(icon_name))
        self.setIconSize(QSize(38, 38))
        self.setCursor(Qt.PointingHandCursor)
        self.setMinimumSize(112, 112)
        self.setMaximumSize(144, 144)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.long_press.start(520)
        super().mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        self.long_press.stop()
        super().mouseReleaseEvent(event)

    def begin_editing(self):
        self.editing = True
        self.setProperty("editing", True)
        self.style().unpolish(self)
        self.style().polish(self)
        animation = QPropertyAnimation(self, b"rotation", self)
        animation.setDuration(320)
        animation.setStartValue(-3)
        animation.setKeyValueAt(0.5, 3)
        animation.setEndValue(-3)
        animation.setLoopCount(-1)
        animation.setEasingCurve(QEasingCurve.InOutSine)
        # Qt widgets do not expose rotation; the stylesheet state gives the visual cue.
        self._wobble = animation
        self._wobble.start()

    def mouseDoubleClickEvent(self, event):
        if self.editing and self.removable:
            self.removed.emit(self)
        super().mouseDoubleClickEvent(event)


class DesktopPage(QWidget):
    def __init__(self, page: int, enter_editing):
        super().__init__()
        self.page = page
        self.enter_editing = enter_editing
        self.setObjectName("DesktopPage")
        self.layout = QVBoxLayout(self)
        self.layout.setContentsMargins(42, 45, 42, 18)
        self.layout.setSpacing(14)
        title = QLabel("Influent Danenone")
        title.setObjectName("DesktopTitle")
        subtitle = QLabel("Página %d  ·  mantén pulsado el escritorio para ordenar aplicaciones" % (page + 1))
        subtitle.setObjectName("DesktopSubtitle")
        self.layout.addWidget(title)
        self.layout.addWidget(subtitle)
        self.layout.addStretch()
        self.grid = QGridLayout()
        self.grid.setHorizontalSpacing(22)
        self.grid.setVerticalSpacing(16)
        self.grid.setAlignment(Qt.AlignHCenter | Qt.AlignBottom)
        apps = [("start", "Inicio", False), ("files", "Archivos", False), ("store", "FoundStore", False), ("control", "Ajustes", False), ("windows", "Ventanas", True), ("notes", "Notas", True), ("photos", "Fotos", True), ("terminal", "Terminal", True)]
        for index, (icon_name, name, removable) in enumerate(apps):
            icon = AppIcon(icon_name, name, removable, self)
            icon.removed.connect(self.remove_icon)
            self.grid.addWidget(icon, index // 4, index % 4)
        self.layout.addLayout(self.grid)
        self.layout.addStretch()

    def remove_icon(self, icon):
        icon.deleteLater()

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.enter_editing()
        super().mousePressEvent(event)


class ControlCenter(GlassPanel):
    def __init__(self, parent=None):
        super().__init__(parent, "ControlCenter")
        self.setFixedSize(360, 470)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(22, 22, 22, 22)
        heading = QLabel("Centro de control")
        heading.setObjectName("PanelHeading")
        layout.addWidget(heading)
        detail = QLabel("Influent Danenone · lectura del sistema")
        detail.setObjectName("Muted")
        layout.addWidget(detail)
        self.cards = {}
        row = QHBoxLayout()
        for key, label in (("wifi", "Wi‑Fi"), ("bluetooth", "Bluetooth")):
            button = QPushButton()
            button.setObjectName("ToggleCard")
            button.setEnabled(False)
            self.cards[key] = (button, label)
            row.addWidget(button)
        layout.addLayout(row)
        self.info = {}
        for key, label in (("network", "Red"), ("battery", "Batería"), ("brightness", "Brillo"), ("volume", "Volumen")):
            line = QLabel()
            line.setObjectName("HardwareLine")
            self.info[key] = (line, label)
            layout.addWidget(line)
        note = QLabel("Los estados se leen del hardware y de los servicios disponibles. No se simulan valores.")
        note.setWordWrap(True)
        note.setObjectName("Muted")
        layout.addWidget(note)
        close = QPushButton("Listo")
        close.clicked.connect(self.hide)
        layout.addWidget(close)
        layout.addStretch()
        self.refresh()
        timer = QTimer(self)
        timer.timeout.connect(self.refresh)
        timer.start(2500)

    def refresh(self):
        status: HardwareStatus = read_hardware_status()
        for key, (button, label) in self.cards.items():
            value = getattr(status, key)
            button.setText(f"{label}\\n{value}")
        for key, (line, label) in self.info.items():
            line.setText(f"{label}:  {getattr(status, key)}")


class Taskbar(GlassPanel):
    control_requested = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent, "Taskbar")
        self.setFixedHeight(78)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(18, 10, 18, 10)
        layout.setSpacing(8)
        start = QPushButton()
        start.setIcon(svg_icon("start"))
        start.setIconSize(QSize(28, 28))
        start.setObjectName("TaskButton")
        start.setToolTip("Inicio")
        layout.addWidget(start)
        search = QPushButton("Buscar aplicaciones y archivos")
        search.setIcon(svg_icon("search"))
        search.setIconSize(QSize(22, 22))
        search.setObjectName("SearchButton")
        layout.addWidget(search)
        layout.addStretch()
        for icon_name, tip in (("desktop", "Escritorio"), ("files", "Archivos"), ("store", "FoundStore"), ("installer", "Instalar Influent")):
            button = QPushButton()
            button.setIcon(svg_icon(icon_name))
            button.setIconSize(QSize(26, 26))
            button.setObjectName("TaskButton")
            button.setToolTip(tip)
            if icon_name == "installer":
                button.clicked.connect(self.launch_installer)
            layout.addWidget(button)
        layout.addStretch()
        controls = QPushButton()
        controls.setIcon(svg_icon("control"))
        controls.setIconSize(QSize(26, 26))
        controls.setObjectName("TaskButton")
        controls.setToolTip("Centro de control")
        controls.clicked.connect(self.control_requested)
        layout.addWidget(controls)
        clock = QLabel(datetime.now().strftime("%H:%M"))
        clock.setObjectName("TaskClock")
        layout.addWidget(clock)

    def launch_installer(self):
        if shutil.which("calamares"):
            subprocess.Popen(["calamares"], start_new_session=True)


class InfluentWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Influent Danenone")
        self.resize(1280, 800)
        self.setMinimumSize(960, 600)
        self.setObjectName("Root")
        self.editing = False
        root = QVBoxLayout(self)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(0)
        self.status = StatusBar(self)
        self.status.notifications_requested.connect(self.toggle_notifications)
        root.addWidget(self.status)
        self.pages = QStackedWidget()
        for page in range(4):
            self.pages.addWidget(DesktopPage(page, self.toggle_editing))
        root.addWidget(self.pages, 1)
        self.dots = QLabel("Página 1 de 4")
        self.dots.setObjectName("PageDots")
        self.dots.setAlignment(Qt.AlignCenter)
        root.addWidget(self.dots)
        self.taskbar = Taskbar(self)
        self.taskbar.control_requested.connect(self.toggle_control)
        root.addWidget(self.taskbar)
        self.pages.currentChanged.connect(self.update_dots)
        self.control = ControlCenter(self)
        self.control.hide()
        self.notifications_panel = NotificationPanel(self)
        self.notifications_panel.hide()
        self.notch = Notch(self)
        self.notch.raise_()
        self.apply_theme()

    def paintEvent(self, event):
        painter = QPainter(self)
        pixmap = QPixmap(str(WALLPAPER))
        if not pixmap.isNull():
            painter.drawPixmap(self.rect(), pixmap)
        painter.fillRect(self.rect(), QColor(6, 12, 24, 65))

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self.notch.move((self.width() - self.notch.width()) // 2, 0)
        self.control.move(self.width() - self.control.width() - 24, self.status.height() + 20)
        self.notifications_panel.move((self.width() - self.notifications_panel.width()) // 2, self.status.height() + 8)

    def update_dots(self, index):
        self.dots.setText(f"Página {index + 1} de {self.pages.count()}")

    def toggle_editing(self):
        self.editing = not self.editing
        self.setProperty("editing", self.editing)
        self.style().unpolish(self)
        self.style().polish(self)

    def toggle_control(self):
        self.control.setVisible(not self.control.isVisible())
        if self.control.isVisible():
            self.control.raise_()

    def toggle_notifications(self):
        self.notifications_panel.setVisible(not self.notifications_panel.isVisible())
        if self.notifications_panel.isVisible():
            self.notifications_panel.refresh()
            self.notifications_panel.raise_()

    def apply_theme(self):
        self.setStyleSheet("""
        #Root { color: #f7f9ff; }
        #StatusBrand { color: white; font-weight: 600; }
        #StatusClock { color: white; font-weight: 600; }
        #StatusState, #StatusBrand, #StatusClock, #StatusNotificationButton { background: rgba(12, 20, 33, 0.34); padding: 5px 10px; border-radius: 11px; }
        #DesktopPage { background: transparent; }
        #DesktopTitle { font-size: 34px; font-weight: 700; color: white; }
        #DesktopSubtitle, #Muted { color: rgba(255,255,255,0.75); }
        #GlassPanel, #Taskbar, #ControlCenter { background: rgba(21, 29, 48, 0.72); border: 1px solid rgba(255,255,255,0.22); border-radius: 20px; }
        #Taskbar { background: rgba(24, 31, 49, 0.82); }
        #AppIcon { color: white; background: rgba(18, 28, 48, 0.55); border: 1px solid rgba(255,255,255,0.20); border-radius: 20px; padding: 12px; font-size: 14px; text-align: center; }
        #AppIcon:hover { background: rgba(90, 125, 190, 0.72); }
        #AppIcon[editing="true"] { border: 2px solid rgba(255,255,255,0.82); background: rgba(94, 108, 170, 0.72); }
        #TaskButton, #SearchButton, #ToggleCard { color: white; background: rgba(255,255,255,0.12); border: 1px solid rgba(255,255,255,0.15); border-radius: 14px; padding: 10px 15px; }
        #TaskButton:hover, #SearchButton:hover, #ToggleCard:hover { background: rgba(255,255,255,0.23); }
        #TaskClock { padding: 10px 8px; font-weight: 600; }
        #PageDots { background: rgba(10,18,30,0.35); color: rgba(255,255,255,0.82); padding: 3px; }
        #PanelHeading { font-size: 22px; font-weight: 700; }
        #HardwareLine { color: rgba(255,255,255,0.90); background: rgba(255,255,255,0.08); border-radius: 10px; padding: 8px 10px; }
        #NotificationPanel { background: rgba(21, 29, 48, 0.94); border: 1px solid rgba(255,255,255,0.24); border-radius: 22px; }
        #NotificationCard { background: rgba(255,255,255,0.08); border: 1px solid rgba(255,255,255,0.12); border-radius: 14px; }
        #NotificationSource, #NotificationTitle { color: white; font-weight: 600; }
        QSlider::groove:horizontal { background: rgba(255,255,255,0.25); height: 5px; border-radius: 3px; }
        QSlider::handle:horizontal { background: #6ea4ff; width: 18px; margin: -7px 0; border-radius: 9px; }
        QPushButton:pressed { background: rgba(105, 160, 255, 0.55); }
        """)


def main() -> int:
    app = QApplication(sys.argv)
    app.setApplicationName("Influent Danenone")
    app.setOrganizationName("Influent")
    app.setFont(QFont("DejaVu Sans", 10))
    window = InfluentWindow()
    window.show()
    return app.exec_()


if __name__ == "__main__":
    raise SystemExit(main())
