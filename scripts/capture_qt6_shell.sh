#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/build/captures/shell"
DISPLAY_NUM="${DISPLAY_NUM:-102}"
mkdir -p "$OUT"
BINARY="$ROOT/native-shell/qt_shell/build/influent-danenone-qt-shell"
[ -x "$BINARY" ] || { echo "Qt6 shell binary not found: $BINARY" >&2; exit 3; }
command -v Xvfb >/dev/null 2>&1 || { echo "Xvfb unavailable" >&2; exit 2; }
command -v ffmpeg >/dev/null 2>&1 || { echo "ffmpeg unavailable" >&2; exit 2; }
Xvfb ":$DISPLAY_NUM" -screen 0 1280x800x24 -nolisten tcp >"$OUT/xvfb.log" 2>&1 &
XVFB_PID=$!
SHELL_PID=""
cleanup() {
  if [ -n "$SHELL_PID" ]; then kill "$SHELL_PID" 2>/dev/null || true; fi
  kill "$XVFB_PID" 2>/dev/null || true
}
trap cleanup EXIT
sleep 1
dbus-run-session -- env DISPLAY=":$DISPLAY_NUM" QT_QPA_PLATFORM=xcb DANENONE_REDUCED_MOTION=1 DANENONE_REDUCED_TRANSPARENCY=0 "$BINARY" >"$OUT/shell.log" 2>&1 &
SHELL_PID=$!
sleep 5
ffmpeg -hide_banner -loglevel error -y -f x11grab -draw_mouse 0 -video_size 1280x800 -framerate 1 -i ":$DISPLAY_NUM" -frames:v 1 "$OUT/desktop.png"
kill "$SHELL_PID" 2>/dev/null || true
wait "$SHELL_PID" 2>/dev/null || true
SHELL_PID=""
printf '%s\n' "$OUT/desktop.png"
