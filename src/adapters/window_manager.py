from __future__ import annotations

import os
import re
import subprocess
from typing import Any

from src.core.logging_setup import setup_logging

logger = setup_logging("danenone.wm")
WindowInfo = dict[str, Any]


class WMAdapter:
    """Safe adapter for optional X11/Wayland window enumeration."""

    def __init__(self, command_timeout: float = 2.0):
        self.command_timeout = command_timeout
        self.session_type = self._detect_session_type()
        self.backend = "none"
        self._init_backend()

    def _detect_session_type(self) -> str:
        session = os.environ.get("XDG_SESSION_TYPE", "").lower()
        if session in {"x11", "wayland"}:
            return session
        if os.environ.get("DISPLAY"):
            return "x11"
        if os.environ.get("WAYLAND_DISPLAY"):
            return "wayland"
        logger.warning("No se detectó una sesión gráfica; WMAdapter desactivado")
        return "none"

    def _init_backend(self) -> None:
        if self.session_type == "none":
            return
        command = ["wlrctl", "--help"] if self.session_type == "wayland" else ["xdotool", "--version"]
        try:
            result = subprocess.run(command, capture_output=True, text=True, timeout=self.command_timeout, check=False)
        except (FileNotFoundError, subprocess.TimeoutExpired, OSError) as exc:
            logger.warning("Backend %s no disponible: %s", self.session_type, type(exc).__name__)
            return
        if result.returncode == 0:
            self.backend = "wlrctl" if self.session_type == "wayland" else "xdotool"
            logger.info("Backend de ventanas activo: %s", self.backend)
        else:
            logger.warning("La comprobación del backend %s falló con código %s", self.session_type, result.returncode)

    def _run(self, command: list[str], timeout: float | None = None) -> subprocess.CompletedProcess[str] | None:
        try:
            return subprocess.run(
                command,
                capture_output=True,
                text=True,
                timeout=timeout or self.command_timeout,
                check=False,
            )
        except (FileNotFoundError, subprocess.TimeoutExpired, OSError) as exc:
            logger.warning("No se pudo ejecutar %s: %s", command[0], type(exc).__name__)
            return None

    def get_active_windows(self) -> list[WindowInfo]:
        if self.backend == "xdotool":
            return self._get_x11_windows()
        if self.backend == "wlrctl":
            return self._get_wayland_windows()
        return []

    def _get_x11_windows(self) -> list[WindowInfo]:
        desktop = self._run(["xdotool", "get_desktop"])
        if not desktop or desktop.returncode != 0 or not desktop.stdout.strip().isdigit():
            return []
        listed = self._run(["xdotool", "search", "--desktop", desktop.stdout.strip(), "--name", ".*"])
        if not listed or listed.returncode != 0:
            return []
        windows = []
        for window_id in listed.stdout.split():
            try:
                item = self._fetch_x11_window(window_id)
                if item:
                    windows.append(item)
            except (ValueError, OSError) as exc:
                logger.warning("Ventana X11 ignorada: %s", type(exc).__name__)
        return windows

    def _fetch_x11_window(self, window_id: str) -> WindowInfo | None:
        if not re.fullmatch(r"\d+", window_id):
            return None
        title_result = self._run(["xdotool", "getwindowname", window_id])
        if not title_result or title_result.returncode != 0:
            return None
        pid_result = self._run(["xdotool", "getwindowpid", window_id])
        try:
            pid = int(pid_result.stdout.strip()) if pid_result and pid_result.returncode == 0 else 0
        except ValueError:
            pid = 0
        geometry_result = self._run(["xdotool", "getwindowgeometry", window_id])
        x = y = width = height = 0
        if geometry_result and geometry_result.returncode == 0:
            for line in geometry_result.stdout.splitlines():
                numbers = [int(value) for value in re.findall(r"\d+", line)]
                if "Position:" in line and len(numbers) >= 2:
                    x, y = numbers[:2]
                elif "Geometry:" in line and len(numbers) >= 2:
                    width, height = numbers[:2]
        state_result = self._run(["xdotool", "getwindowstate", window_id])
        return {
            "id": window_id,
            "title": title_result.stdout.strip()[:256] or "Sin título",
            "pid": pid,
            "x": x,
            "y": y,
            "width": width,
            "height": height,
            "minimized": bool(state_result and "WINDOW_STATE_MINIMIZED" in state_result.stdout),
        }

    def _get_wayland_windows(self) -> list[WindowInfo]:
        result = self._run(["wlrctl", "window", "list"], timeout=3.0)
        if not result or result.returncode != 0:
            return []
        windows = []
        for line in result.stdout.splitlines():
            if ":" not in line:
                continue
            app_id, title = line.split(":", 1)
            app_id = app_id.strip()[:256]
            title = title.strip()[:256]
            if app_id:
                windows.append({"id": app_id, "title": title or app_id, "pid": 0, "x": 0, "y": 0, "width": 0, "height": 0, "minimized": False})
        logger.info("Obtenidas %d ventanas vía wlrctl", len(windows))
        return windows

    def focus_window(self, window_id: str) -> bool:
        if self.backend != "xdotool" or not re.fullmatch(r"\d+", window_id):
            return False
        result = self._run(["xdotool", "windowactivate", window_id])
        return bool(result and result.returncode == 0)
