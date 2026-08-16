#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DISPLAY_NUM=98
OUT="$ROOT/build/oobe-demo-local.png"
mkdir -p "$ROOT/build"
Xvfb ":$DISPLAY_NUM" -screen 0 1280x800x24 -nolisten tcp >/tmp/danenone-demo-xvfb.log 2>&1 &
XVFB_PID=$!
cleanup() {
  kill "$OOBE_PID" 2>/dev/null || true
  kill "$XVFB_PID" 2>/dev/null || true
}
trap cleanup EXIT
sleep 1
DISPLAY=":$DISPLAY_NUM" GDK_BACKEND=x11 "$ROOT/native-shell/influent-danenone-firstboot" --replay --demo >/tmp/danenone-oobe-demo.log 2>&1 &
OOBE_PID=$!
sleep 10
ffmpeg -hide_banner -loglevel error -y -f x11grab -video_size 1280x800 -framerate 1 -i ":$DISPLAY_NUM" -frames:v 1 "$OUT"
kill "$OOBE_PID" 2>/dev/null || true
cat /tmp/danenone-oobe-demo.log || true
printf '%s\n' "$OUT"
