#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

# Influent Danenone: emulación no-root para Termux + Termux:X11.
# Ejecutar desde Termux, no desde Debian/proot.
ISO_URL="${INFLUENT_ISO_URL:-https://github.com/JesusQuijada34/danenone/releases/download/v0.2.0-preview/influent-danenone-0.2.0-preview-amd64.iso}"
ROOT="${PREFIX}/share/influent-danenone"
ISO="${ROOT}/influent-danenone-amd64.iso"
RAM="${INFLUENT_RAM:-2048}"

pkg update -y
pkg upgrade -y
pkg install -y x11-repo termux-x11-nightly qemu-system-x86-64-headless wget coreutils
mkdir -p "$ROOT"

if [ ! -s "$ISO" ]; then
  echo "Descargando Influent Danenone..."
  wget -O "$ISO.part" "$ISO_URL"
  mv "$ISO.part" "$ISO"
fi

if command -v sha256sum >/dev/null 2>&1 && [ -n "${INFLUENT_ISO_SHA256:-}" ]; then
  echo "${INFLUENT_ISO_SHA256}  ${ISO}" | sha256sum -c -
fi

# Termux:X11 debe estar instalado como APK desde el release oficial y abierto al menos una vez.
export XDG_RUNTIME_DIR="${TMPDIR}"
export DISPLAY=":1"
export LIBGL_ALWAYS_SOFTWARE=1
termux-x11 :1 -legacy-drawing >/dev/null 2>&1 &
X11_PID=$!
trap 'kill "$X11_PID" 2>/dev/null || true' EXIT
sleep 3

# La ISO es amd64; en teléfonos ARM QEMU usa traducción TCG y será lento.
exec qemu-system-x86_64 \
  -M pc,accel=tcg \
  -cpu max \
  -m "$RAM" \
  -smp 2 \
  -drive file="$ISO",media=cdrom,readonly=on \
  -boot d \
  -display sdl,gl=off \
  -device virtio-vga \
  -nic user,model=virtio \
  -name Influent-Danenone
