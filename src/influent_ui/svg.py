from __future__ import annotations

from PyQt5.QtCore import QByteArray, Qt
from PyQt5.QtGui import QIcon, QPainter, QPixmap
from PyQt5.QtSvg import QSvgRenderer


ICONS = {
    "home": '<path d="M5 21V9.5L12 4l7 5.5V21H5z" fill="none" stroke="{color}" stroke-width="2" stroke-linejoin="round"/>',
    "files": '<path d="M4 6h7l2 2h7v10H4z" fill="none" stroke="{color}" stroke-width="2" stroke-linejoin="round"/>',
    "settings": '<path d="M12 8a4 4 0 1 0 0 8 4 4 0 0 0 0-8zm0-6v3m0 14v3M4.2 4.2l2.1 2.1m11.4 11.4 2.1 2.1M2 12h3m14 0h3M4.2 19.8l2.1-2.1M17.7 6.3l2.1-2.1" fill="none" stroke="{color}" stroke-width="2" stroke-linecap="round"/>',
    "store": '<path d="M5 9h14l-1 11H6L5 9zm3 0V7a4 4 0 0 1 8 0v2" fill="none" stroke="{color}" stroke-width="2" stroke-linejoin="round"/>',
    "terminal": '<path d="m5 7 5 5-5 5m7 0h7" fill="none" stroke="{color}" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>',
    "notification": '<path d="M18 9a6 6 0 0 0-12 0c0 7-3 7-3 9h18c0-2-3-2-3-9M10 21h4" fill="none" stroke="{color}" stroke-width="2" stroke-linecap="round"/>',
    "control": '<path d="M4 7h16M4 12h16M4 17h16M9 5v4m6 1v4m-5 1v4" fill="none" stroke="{color}" stroke-width="2" stroke-linecap="round"/>',
}


def icon_svg(name: str, color: str = "#F7F9FF", accent: str | None = None) -> str:
    body = ICONS.get(name, ICONS["home"]).format(color=color)
    underline = f'<path d="M8 22h8" stroke="{accent or color}" stroke-width="2.5" stroke-linecap="round"/>' if accent else ""
    return f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">{body}{underline}</svg>'


def render_svg(name: str, size: int = 22, color: str = "#F7F9FF", accent: str | None = None) -> QIcon:
    renderer = QSvgRenderer(QByteArray(icon_svg(name, color, accent).encode("utf-8")))
    pixmap = QPixmap(size, size)
    pixmap.fill(Qt.transparent)
    painter = QPainter(pixmap)
    renderer.render(painter)
    painter.end()
    return QIcon(pixmap)
