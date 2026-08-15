from __future__ import annotations

from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtWidgets import QCheckBox, QFrame, QLineEdit, QPushButton

from .svg import render_svg
from .theme import ThemeTokens


class InfluentCard(QFrame):
    def __init__(self, parent=None, tokens: ThemeTokens | None = None):
        super().__init__(parent)
        self.tokens = tokens or ThemeTokens()
        self.setObjectName("InfluentCard")
        self.setStyleSheet(f"""
        QFrame#InfluentCard {{ background: {self.tokens.palette.surface}; border: 1px solid {self.tokens.palette.border}; border-radius: {self.tokens.radius_large}px; }}
        """)


class InfluentButton(QPushButton):
    def __init__(self, text: str = "", icon_name: str | None = None, parent=None, tokens: ThemeTokens | None = None):
        super().__init__(text, parent)
        self.tokens = tokens or ThemeTokens()
        self.setCursor(Qt.PointingHandCursor)
        if icon_name:
            self.setIcon(render_svg(icon_name, self.tokens.icon_size, self.tokens.palette.accent))
        self.setStyleSheet(f"""
        QPushButton {{ color: {self.tokens.palette.text}; background: rgba(255,255,255,0.08); border: 1px solid {self.tokens.palette.border}; border-radius: {self.tokens.radius_medium}px; padding: 9px 14px; }}
        QPushButton:hover {{ background: rgba(255,255,255,0.16); border-color: {self.tokens.palette.accent}; }}
        QPushButton:pressed {{ background: rgba(110,168,255,0.24); }}
        """)


class InfluentInput(QLineEdit):
    def __init__(self, placeholder: str = "", parent=None, tokens: ThemeTokens | None = None):
        super().__init__(parent)
        self.tokens = tokens or ThemeTokens()
        self.setPlaceholderText(placeholder)
        self.setStyleSheet(ThemeTokens().stylesheet())


class InfluentToggle(QCheckBox):
    changed = pyqtSignal(bool)

    def __init__(self, text: str = "", parent=None, tokens: ThemeTokens | None = None):
        super().__init__(text, parent)
        self.tokens = tokens or ThemeTokens()
        self.toggled.connect(self.changed.emit)
        p = self.tokens.palette
        self.setStyleSheet(f"""
        QCheckBox {{ spacing: 10px; color: {p.text}; }}
        QCheckBox::indicator {{ width: 42px; height: 24px; border-radius: 12px; background: rgba(255,255,255,0.18); border: 1px solid {p.border}; }}
        QCheckBox::indicator:checked {{ background: {p.accent}; border-color: {p.accent}; }}
        """)
