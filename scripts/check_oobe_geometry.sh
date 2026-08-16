#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DISPLAY_NUM=97
Xvfb ":$DISPLAY_NUM" -screen 0 1280x800x24 -nolisten tcp >/tmp/danenone-geometry-xvfb.log 2>&1 &
XVFB_PID=$!
cleanup() {
  kill "$OOBE_PID" 2>/dev/null || true
  kill "$XVFB_PID" 2>/dev/null || true
}
trap cleanup EXIT
sleep 1
DISPLAY=":$DISPLAY_NUM" GDK_BACKEND=x11 "$ROOT/native-shell/influent-danenone-firstboot" --replay >/tmp/danenone-geometry-oobe.log 2>&1 &
OOBE_PID=$!
sleep 3
if command -v xwininfo >/dev/null 2>&1; then
  DISPLAY=":$DISPLAY_NUM" xwininfo -root -tree | head -80
else
  echo "xwininfo-missing"
fi
printf '%s\n' '--- oobe log ---'
cat /tmp/danenone-geometry-oobe.log || true
