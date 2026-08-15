from __future__ import annotations

from dataclasses import dataclass

from PyQt5.QtCore import QObject, QRect, pyqtSignal
from PyQt5.QtWidgets import QWidget


@dataclass(frozen=True)
class DisplayInsets:
    top: int = 48
    bottom: int = 84
    left: int = 0
    right: int = 0


class WindowManager(QObject):
    safe_geometry_changed = pyqtSignal(QRect)

    def __init__(self, parent=None, insets: DisplayInsets | None = None):
        super().__init__(parent)
        self.insets = insets or DisplayInsets()
        self.windows: list[QWidget] = []

    def safe_geometry(self, screen_geometry: QRect) -> QRect:
        rect = QRect(screen_geometry)
        rect.adjust(self.insets.left, self.insets.top, -self.insets.right, -self.insets.bottom)
        return rect

    def register(self, window: QWidget) -> None:
        if window not in self.windows:
            self.windows.append(window)
        window.destroyed.connect(lambda: self._remove(window))

    def _remove(self, window: QWidget) -> None:
        if window in self.windows:
            self.windows.remove(window)

    def apply_fullscreen_policy(self, window: QWidget, screen_geometry: QRect, fullscreen: bool = False) -> QRect:
        target = screen_geometry if fullscreen else self.safe_geometry(screen_geometry)
        window.setGeometry(target)
        self.safe_geometry_changed.emit(target)
        return target

    def tile_left(self, window: QWidget, screen_geometry: QRect) -> QRect:
        safe = self.safe_geometry(screen_geometry)
        target = QRect(safe.x(), safe.y(), safe.width() // 2, safe.height())
        window.setGeometry(target)
        return target

    def tile_right(self, window: QWidget, screen_geometry: QRect) -> QRect:
        safe = self.safe_geometry(screen_geometry)
        target = QRect(safe.x() + safe.width() // 2, safe.y(), safe.width() - safe.width() // 2, safe.height())
        window.setGeometry(target)
        return target
