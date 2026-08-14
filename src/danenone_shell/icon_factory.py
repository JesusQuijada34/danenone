from __future__ import annotations

from PyQt5.QtCore import QByteArray, Qt
from PyQt5.QtGui import QIcon, QPainter, QPixmap
from PyQt5.QtSvg import QSvgRenderer


ICONS = {
    "start": '<path d="M8 8h14v14H8zM26 8h14v14H26zM8 26h14v14H8zM26 26h14v14H26z"/>',
    "search": '<circle cx="21" cy="21" r="11"/><path d="m30 30 9 9"/>',
    "desktop": '<rect x="7" y="8" width="34" height="24" rx="3"/><path d="M18 40h12M24 32v8"/>',
    "files": '<path d="M7 13h13l4 4h17v20H7z"/>',
    "store": '<path d="M9 19h30v22H9zM13 19l3-10h16l3 10M17 25v10M31 25v10"/>',
    "windows": '<rect x="8" y="8" width="14" height="14"/><rect x="26" y="8" width="14" height="14"/><rect x="8" y="26" width="14" height="14"/><rect x="26" y="26" width="14" height="14"/>',
    "notes": '<path d="M10 7h22l6 6v28H10zM32 7v7h6M16 23h16M16 30h16M16 37h10"/>',
    "photos": '<rect x="7" y="11" width="34" height="27" rx="4"/><circle cx="18" cy="21" r="3"/><path d="m12 34 8-8 6 6 5-5 6 7"/>',
    "terminal": '<path d="m10 15 9 9-9 9M23 33h14"/>',
    "control": '<path d="M8 12h32M8 24h32M8 36h32M18 8v8M31 20v8M14 32v8"/>',
    "installer": '<path d="M8 12h32v28H8zM14 8h20M17 20h14M17 27h14M17 34h9"/>',
}


def svg_icon(name: str, color: str = "#FFFFFF", accent: str = "#78A9FF", size: int = 32) -> QIcon:
    body = ICONS.get(name, ICONS["windows"])
    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 48 48">
    <g fill="none" stroke="{color}" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">{body}</g>
    <path d="M10 45h28" stroke="{accent}" stroke-width="3" stroke-linecap="round"/>
    </svg>'''
    renderer = QSvgRenderer(QByteArray(svg.encode("utf-8")))
    pixmap = QPixmap(size, size)
    pixmap.fill(Qt.transparent)
    painter = QPainter(pixmap)
    renderer.render(painter)
    painter.end()
    return QIcon(pixmap)
