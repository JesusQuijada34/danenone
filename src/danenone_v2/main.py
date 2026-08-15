from __future__ import annotations

import sys

from PyQt5.QtWidgets import QApplication

from .ui.shell import DanenoneShell


def main() -> int:
    app = QApplication(sys.argv)
    app.setApplicationName("Influent Danenone")
    app.setOrganizationName("Influent")
    app.setStyleSheet(
        """
        * { color: #F4F8FF; font-family: 'DejaVu Sans'; }
        #DanenoneShell { background: #07101F; }
        #Desktop { background: transparent; }
        #Brand { font-size: 18px; font-weight: 600; }
        #Clock { font-size: 16px; font-weight: 600; }
        #Notch { background: #05070D; border: 1px solid #222B3A; border-radius: 21px; }
        #AcrylicPanel, #Taskbar, #ControlCenter, #FileSurface { background: rgba(13, 22, 38, 232); border: 1px solid rgba(206, 226, 255, 55); border-radius: 22px; }
        #Taskbar { border-radius: 28px; }
        #AppTile, #TaskButton, #StartButton, #ControlTile, #FileCard, #NavButton, #SecondaryButton { background: rgba(16, 31, 52, 190); border: 1px solid rgba(219, 236, 255, 42); border-radius: 16px; padding: 10px; }
        #AppTile { font-size: 14px; }
        #AppTile:hover, #TaskButton:hover, #StartButton:hover, #ControlTile:hover, #FileCard:hover, #NavButton:hover, #SecondaryButton:hover { background: rgba(79, 132, 196, 210); border-color: rgba(221, 241, 255, 145); }
        #TaskButton, #StartButton { min-width: 48px; min-height: 42px; font-size: 18px; }
        #StartButton { background: rgba(93, 143, 221, 215); }
        #PanelTitle { font-size: 22px; font-weight: 600; }
        #FileGrid { background: rgba(3, 10, 20, 120); border-radius: 18px; }
        """
    )
    window = DanenoneShell()
    window.show()
    return app.exec_()


if __name__ == "__main__":
    raise SystemExit(main())
