from __future__ import annotations

import glob
import os
import re
import shutil
import subprocess
from dataclasses import dataclass


@dataclass(frozen=True)
class HardwareStatus:
    wifi: str
    bluetooth: str
    battery: str
    volume: str
    brightness: str
    network: str


def _run(command: list[str], timeout: float = 1.5) -> str:
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=timeout, check=False)
        return result.stdout.strip()
    except (OSError, subprocess.SubprocessError):
        return ""


def wifi_status() -> str:
    if shutil.which("nmcli"):
        output = _run(["nmcli", "-t", "-f", "WIFI,STATE", "radio"])
        if output:
            wifi, _, state = output.partition(":")
            if wifi == "enabled":
                return "Activado" if state in {"enabled", "connected"} else "Disponible"
            return "Desactivado"
    interfaces = []
    for path in glob.glob("/sys/class/net/*/wireless"):
        interfaces.append(path.split("/")[-2])
    return "Disponible" if interfaces else "No disponible"


def network_status() -> str:
    if shutil.which("nmcli"):
        output = _run(["nmcli", "-t", "-f", "STATE,CONNECTIVITY", "general"])
        if output:
            return output.replace(":", " · ", 1).capitalize()
    if shutil.which("ip"):
        output = _run(["ip", "-o", "route", "get", "1.1.1.1"])
        if output:
            return "Conectado"
    return "No disponible"


def bluetooth_status() -> str:
    if shutil.which("bluetoothctl"):
        output = _run(["bluetoothctl", "show"])
        match = re.search(r"Powered:\s+(yes|no)", output, re.I)
        if match:
            return "Activado" if match.group(1).lower() == "yes" else "Desactivado"
    return "No disponible"


def battery_status() -> str:
    capacities = []
    statuses = []
    for capacity in glob.glob("/sys/class/power_supply/BAT*/capacity"):
        try:
            capacities.append(int(open(capacity, encoding="utf-8").read().strip()))
        except (OSError, ValueError):
            pass
    for status in glob.glob("/sys/class/power_supply/BAT*/status"):
        try:
            statuses.append(open(status, encoding="utf-8").read().strip())
        except OSError:
            pass
    if capacities:
        state = statuses[0] if statuses else ""
        suffix = " · cargando" if state.lower() == "charging" else ""
        return f"{round(sum(capacities) / len(capacities))}%{suffix}"
    if shutil.which("upower"):
        output = _run(["upower", "-e"])
        battery = next((line for line in output.splitlines() if "battery" in line.lower()), "")
        if battery:
            info = _run(["upower", "-i", battery])
            match = re.search(r"percentage:\s+(\d+%)", info, re.I)
            if match:
                return match.group(1)
    return "No disponible"


def _mixer_status() -> str:
    if shutil.which("wpctl"):
        output = _run(["wpctl", "get-volume", "@DEFAULT_AUDIO_SINK@"])
        match = re.search(r"Volume:\s+([0-9.]+)", output)
        if match:
            return f"{round(float(match.group(1)) * 100)}%"
    if shutil.which("pactl"):
        output = _run(["pactl", "get-sink-volume", "@DEFAULT_SINK@"])
        match = re.search(r"(\d+)%", output)
        if match:
            return f"{match.group(1)}%"
    return "No disponible"


def brightness_status() -> str:
    values = []
    for path in glob.glob("/sys/class/backlight/*/brightness"):
        try:
            current = int(open(path, encoding="utf-8").read().strip())
            maximum = int(open(path.replace("brightness", "max_brightness"), encoding="utf-8").read().strip())
            if maximum > 0:
                values.append(round(current * 100 / maximum))
        except (OSError, ValueError, ZeroDivisionError):
            pass
    if values:
        return f"{round(sum(values) / len(values))}%"
    if shutil.which("brightnessctl"):
        output = _run(["brightnessctl", "-m"])
        match = re.search(r",(\d+)%", output)
        if match:
            return f"{match.group(1)}%"
    return "No disponible"


def read_hardware_status() -> HardwareStatus:
    return HardwareStatus(
        wifi=wifi_status(),
        bluetooth=bluetooth_status(),
        battery=battery_status(),
        volume=_mixer_status(),
        brightness=brightness_status(),
        network=network_status(),
    )
