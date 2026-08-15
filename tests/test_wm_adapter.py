from __future__ import annotations

import subprocess
from types import SimpleNamespace
from unittest.mock import patch

from src.adapters.window_manager import WMAdapter


def result(stdout="", returncode=0):
    return SimpleNamespace(stdout=stdout, stderr="", returncode=returncode)


def test_no_graphical_session_degrades_cleanly():
    with patch.dict("os.environ", {}, clear=True):
        adapter = WMAdapter()
    assert adapter.session_type == "none"
    assert adapter.backend == "none"
    assert adapter.get_active_windows() == []


def test_x11_enumerates_valid_windows():
    def run(command, **kwargs):
        if command == ["xdotool", "--version"]:
            return result("xdotool version 3.2021")
        if command == ["xdotool", "get_desktop"]:
            return result("0\n")
        if command[:3] == ["xdotool", "search", "--desktop"]:
            return result("12345\n67890\n")
        if command[:2] == ["xdotool", "getwindowname"]:
            return result("Firefox\n" if command[-1] == "12345" else "Terminal\n")
        if command[:2] == ["xdotool", "getwindowpid"]:
            return result("1234\n" if command[-1] == "12345" else "5678\n")
        if command[:2] == ["xdotool", "getwindowgeometry"]:
            return result("Position: 100,200\nGeometry: 800x600\n")
        if command[:2] == ["xdotool", "getwindowstate"]:
            return result("WINDOW_STATE_NORMAL\n")
        raise AssertionError(command)

    with patch.dict("os.environ", {"DISPLAY": ":0", "XDG_SESSION_TYPE": "x11"}, clear=True), patch("subprocess.run", side_effect=run):
        adapter = WMAdapter()
        windows = adapter.get_active_windows()
    assert adapter.backend == "xdotool"
    assert len(windows) == 2
    assert windows[0]["title"] == "Firefox"
    assert windows[0]["pid"] == 1234
    assert windows[0]["width"] == 800
    assert windows[0]["height"] == 600


def test_backend_failure_degrades_to_empty():
    with patch.dict("os.environ", {"DISPLAY": ":0"}, clear=True), patch("subprocess.run", return_value=result(returncode=1)):
        adapter = WMAdapter()
    assert adapter.backend == "none"
    assert adapter.get_active_windows() == []


def test_backend_timeout_degrades_to_empty():
    with patch.dict("os.environ", {"DISPLAY": ":0"}, clear=True), patch("subprocess.run", side_effect=subprocess.TimeoutExpired("xdotool", 2)):
        adapter = WMAdapter()
    assert adapter.backend == "none"


def test_malformed_window_id_is_ignored():
    adapter = object.__new__(WMAdapter)
    adapter.backend = "xdotool"
    assert adapter._fetch_x11_window("not-a-window-id") is None


def test_focus_rejects_untrusted_window_id():
    adapter = object.__new__(WMAdapter)
    adapter.backend = "xdotool"
    with patch("subprocess.run") as run:
        assert adapter.focus_window("123; rm -rf /") is False
        run.assert_not_called()
