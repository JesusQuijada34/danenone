from __future__ import annotations

from pathlib import Path

from PyQt5.QtWidgets import QApplication

from src.danenone_v2.ui.shell import DanenoneShell, WALLPAPER


def test_clean_shell_has_required_surfaces(qtbot):
    window = DanenoneShell()
    qtbot.addWidget(window)
    window.show()
    qtbot.wait(20)
    assert window.windowTitle() == "Influent Danenone"
    assert window.notch.isVisible() is True
    assert WALLPAPER.exists()
    window.show_files()
    assert window.files.isVisible() is True
    window.show_control()
    assert window.control.isVisible() is True
    assert window.files.isVisible() is False


def test_wallpaper_is_project_asset():
    assert Path(WALLPAPER).is_file()
