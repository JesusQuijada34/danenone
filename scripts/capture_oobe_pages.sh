#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/build/captures/oobe"
DISPLAY_NUM="${DISPLAY_NUM:-101}"
mkdir -p "$OUT"

if ! command -v Xvfb >/dev/null 2>&1 || ! command -v ffmpeg >/dev/null 2>&1; then
  echo "capture dependencies unavailable: Xvfb and ffmpeg are required" >&2
  exit 2
fi

if [ ! -x "$ROOT/native-shell/influent-danenone-firstboot" ]; then
  echo "active OOBE binary not found: $ROOT/native-shell/influent-danenone-firstboot" >&2
  exit 3
fi

Xvfb ":$DISPLAY_NUM" -screen 0 1280x800x24 -nolisten tcp >"$OUT/xvfb.log" 2>&1 &
XVFB_PID=$!
OOBE_PID=""
cleanup() {
  if [ -n "$OOBE_PID" ]; then kill "$OOBE_PID" 2>/dev/null || true; fi
  kill "$XVFB_PID" 2>/dev/null || true
}
trap cleanup EXIT
sleep 1

capture_page() {
  page="$1"
  name="$2"
  log="$OUT/${name}.log"
  image="$OUT/${name}.png"
  dbus-run-session -- env DISPLAY=":$DISPLAY_NUM" GDK_BACKEND=x11 GTK_A11Y=none DANENONE_OOBE_PAGE="$page" "$ROOT/native-shell/influent-danenone-firstboot" >"$log" 2>&1 &
  OOBE_PID=$!
  sleep 6
  ffmpeg -hide_banner -loglevel error -y -f x11grab -draw_mouse 0 -video_size 1280x800 -framerate 1 -i ":$DISPLAY_NUM" -frames:v 1 "$image"
  kill "$OOBE_PID" 2>/dev/null || true
  wait "$OOBE_PID" 2>/dev/null || true
  OOBE_PID=""
  printf '%s\n' "$image"
}

capture_page 0 welcome
capture_page 1 language-connectivity
capture_page 2 edition
capture_page 5 user
capture_page 7 appearance
capture_page 8 notch
capture_page 10 summary
capture_page 11 installation
