from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class Palette:
    background: str = "#111827"
    surface: str = "rgba(18, 27, 43, 0.82)"
    surface_strong: str = "rgba(18, 27, 43, 0.96)"
    border: str = "rgba(255, 255, 255, 0.18)"
    text: str = "#F7F9FF"
    muted: str = "rgba(247, 249, 255, 0.68)"
    accent: str = "#6EA8FF"
    accent_strong: str = "#8E7BFF"
    success: str = "#37C88A"
    warning: str = "#FFB454"
    danger: str = "#FF6B78"


@dataclass(frozen=True)
class ThemeTokens:
    palette: Palette = Palette()
    radius_small: int = 10
    radius_medium: int = 16
    radius_large: int = 24
    spacing: int = 12
    icon_size: int = 22
    underline_height: int = 3

    def stylesheet(self) -> str:
        p = self.palette
        return f"""
        QWidget {{ color: {p.text}; font-family: 'Segoe UI', sans-serif; }}
        QToolTip {{ color: {p.text}; background: {p.surface_strong}; border: 1px solid {p.border}; padding: 6px; }}
        QScrollArea {{ border: none; background: transparent; }}
        QLineEdit {{ color: {p.text}; background: rgba(0,0,0,0.20); border: 1px solid {p.border}; border-radius: {self.radius_small}px; padding: 9px 12px; }}
        QLineEdit:focus {{ border: 1px solid {p.accent}; }}
        """
