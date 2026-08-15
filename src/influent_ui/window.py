from __future__ import annotations

from PyQt5.QtCore import Qt, QRect, QPoint
from PyQt5.QtWidgets import QMainWindow, QVBoxLayout, QWidget

from .theme import ThemeTokens


class SafeArea(QWidget):
    def __init__(self, parent=None, top: int = 48, bottom: int = 84, left: int = 20, right: int = 20):
        super().__init__(parent)
        self.insets = {"top": top, "bottom": bottom, "left": left, "right": right}
        self.setObjectName("SafeArea")
        self.content = QWidget(self)
        self.layout = QVBoxLayout(self.content)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.layout.setSpacing(0)
        self._apply_geometry()

    def _apply_geometry(self):
        if self.width() and self.height():
            self.content.setGeometry(
                self.insets["left"],
                self.insets["top"],
                max(0, self.width() - self.insets["left"] - self.insets["right"]),
                max(0, self.height() - self.insets["top"] - self.insets["bottom"]),
            )

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self._apply_geometry()


class InfluentWindow(QMainWindow):
    def __init__(self, title: str = "Influent", width: int = 900, height: int = 600, tokens: ThemeTokens | None = None):
        super().__init__()
        self.tokens = tokens or ThemeTokens()
        self.setWindowTitle(title)
        self.resize(width, height)
        self.setAttribute(Qt.WA_TranslucentBackground)
        self.setWindowFlag(Qt.FramelessWindowHint, True)
        self._drag_origin: QPoint | None = None
        self.frame = QWidget(self)
        self.frame.setObjectName("InfluentWindowFrame")
        self.frame.setStyleSheet(f"""
        QWidget#InfluentWindowFrame {{ background: {self.tokens.palette.surface_strong}; border: 1px solid {self.tokens.palette.border}; border-radius: {self.tokens.radius_large}px; }}
        """)
        self.setCentralWidget(self.frame)
        self.content = QVBoxLayout(self.frame)
        self.content.setContentsMargins(18, 18, 18, 18)
        self.content.setSpacing(0)

    def set_content(self, widget: QWidget):
        self.content.addWidget(widget)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self._drag_origin = event.globalPos() - self.frameGeometry().topLeft()
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if self._drag_origin is not None and event.buttons() & Qt.LeftButton:
            self.move(event.globalPos() - self._drag_origin)
        super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event):
        self._drag_origin = None
        super().mouseReleaseEvent(event)

    def apply_safe_area(self, widget: QWidget, top: int = 48, bottom: int = 84):
        safe = SafeArea(widget, top=top, bottom=bottom)
        safe.layout.addWidget(widget)
        return safe
