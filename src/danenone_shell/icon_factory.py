from __future__ import annotations

from influent_ui.svg import icon_svg, render_svg


ALIASES = {
    "grid": "home",
    "folder": "files",
    "store": "store",
    "settings": "settings",
    "control": "control",
    "notification": "notification",
    "terminal": "terminal",
}


def svg_icon(name: str, size: int = 22, color: str = "#F7F9FF", accent: str = "#6EA8FF"):
    return render_svg(ALIASES.get(name, name), size=size, color=color, accent=accent)


__all__ = ["icon_svg", "render_svg", "svg_icon"]
