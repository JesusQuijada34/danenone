#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/build/oobe-expanded-captures"
DISPLAY_NUM="${DISPLAY_NUM:-103}"
BIN="$ROOT/native-shell/influent-danenone-firstboot"
mkdir -p "$OUT"
[ -x "$BIN" ] || { echo "missing OOBE binary: $BIN" >&2; exit 3; }
Xvfb ":$DISPLAY_NUM" -screen 0 1280x800x24 -nolisten tcp >"$OUT/xvfb.log" 2>&1 &
XVFB_PID=$!
PIDS=()
cleanup() { for pid in "${PIDS[@]:-}"; do kill "$pid" 2>/dev/null || true; done; kill "$XVFB_PID" 2>/dev/null || true; }
trap cleanup EXIT
sleep 1
run_case() {
  local name="$1" page="$2" lang="$3"
  dbus-run-session -- env DISPLAY=":$DISPLAY_NUM" WAYLAND_DISPLAY= GDK_BACKEND=x11 XDG_SESSION_TYPE=x11 GTK_A11Y=none DANENONE_OOBE_PAGE="$page" DANENONE_TEST_LANGUAGE="$lang" "$BIN" >"$OUT/$name.log" 2>&1 &
  local pid=$!
  PIDS+=("$pid")
  sleep 6
  ffmpeg -hide_banner -loglevel error -y -f x11grab -draw_mouse 0 -video_size 1280x800 -framerate 1 -i ":$DISPLAY_NUM" -frames:v 1 "$OUT/$name.png"
  kill "$pid" 2>/dev/null || true
  wait "$pid" 2>/dev/null || true
  PIDS=()
  if grep -qE 'Gtk-CRITICAL|Segmentation fault|Assertion.*failed|ERROR' "$OUT/$name.log"; then
    echo "RUNTIME_DIAGNOSTIC=$name" >&2
    grep -E 'Gtk-CRITICAL|Segmentation fault|Assertion.*failed|ERROR' "$OUT/$name.log" >&2 || true
    return 4
  fi
}
run_case welcome-english 0 en
run_case welcome-spanish 0 es
run_case license-spanish 2 es
run_case edition-spanish 3 es
run_case storage-english 4 en
run_case network-english 5 en
run_case fluthin-english 6 en
run_case debian-english 7 en
run_case appearance-spanish 9 es
run_case notch-spanish 10 es
run_case emoji-english 12 en
run_case summary-spanish 13 es
printf '%s\n' "OOBE_EXPANDED_RUNTIME_STATUS=0"
find "$OUT" -maxdepth 1 -name '*.png' -printf '%f %s bytes\n' | sort
