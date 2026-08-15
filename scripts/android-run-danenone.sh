#!/data/data/com.termux/files/usr/bin/bash
set -Eeuo pipefail

# Instalador y lanzador automático de Influent Danenone para Termux + Termux:X11.
# Uso: bash android-run-danenone.sh

ISO_URL="https://github.com/JesusQuijada34/danenone/releases/download/v0.4.0/influent-danenone-0.4.0-x86_64.iso"
ISO_SHA256="7b9c234611cd67b6e26d2dcf79a4c88615331a45fcefa87aec23af5e02ae85ec"
BASE_DIR="${DANENONE_DIR:-${HOME}/danenone-v0.4.0}"
ISO_PATH="${BASE_DIR}/influent-danenone-0.4.0-x86_64.iso"
DISK_PATH="${BASE_DIR}/danenone.qcow2"
DISPLAY_NUM="${DISPLAY_NUM:-:1}"
RAM_MB="${DANENONE_RAM_MB:-2048}"
CPUS="${DANENONE_CPUS:-2}"
DISK_SIZE="${DANENONE_DISK_SIZE:-32G}"
X11_LOG="${BASE_DIR}/termux-x11.log"

log() { printf '\n[Danenone] %s\n' "$*"; }
die() { printf '\n[Danenone] ERROR: %s\n' "$*" >&2; exit 1; }
command_exists() { command -v "$1" >/dev/null 2>&1; }

cleanup() {
    if [ -n "${X11_PID:-}" ] && kill -0 "$X11_PID" 2>/dev/null; then
        kill "$X11_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

if [ -z "${PREFIX:-}" ] || [[ "$PREFIX" != */com.termux/files/usr ]]; then
    die "Este script debe ejecutarse dentro de Termux."
fi

if [ "$(id -u)" -eq 0 ]; then
    die "No ejecutes Termux como root; QEMU y Termux:X11 deben usar la sesión normal de Termux."
fi

ANDROID_ABI="$(getprop ro.product.cpu.abi 2>/dev/null || true)"
if [[ "$ANDROID_ABI" != arm64-v8a && "$ANDROID_ABI" != x86_64 && "$(uname -m)" != aarch64 && "$(uname -m)" != x86_64 ]]; then
    log "Aviso: arquitectura Android no reconocida: ${ANDROID_ABI:-desconocida}. QEMU podría no tener un binario compatible."
fi

log "Actualizando Termux y sus repositorios."
pkg update -y
pkg upgrade -y

log "Instalando utilidades de descarga y verificación."
pkg install -y curl coreutils

log "Activando el repositorio gráfico de Termux."
pkg install -y x11-repo

if ! command_exists termux-x11; then
    log "Instalando el companion de Termux:X11."
    pkg install -y termux-x11-nightly || die "No se pudo instalar termux-x11-nightly. Ejecuta 'pkg search termux-x11' para comprobar el nombre disponible en tu espejo."
fi

if ! command_exists qemu-system-x86_64; then
    log "Instalando QEMU para emulación x86_64."
    if ! pkg install -y qemu-system-x86_64; then
        die "El repositorio Termux no ofrece qemu-system-x86_64 en este dispositivo. Ejecuta 'termux-change-repo', selecciona un espejo oficial y vuelve a ejecutar el script."
    fi
fi

if ! command_exists qemu-img; then
    log "Buscando qemu-img en el paquete auxiliar del repositorio."
    pkg install -y qemu-utils 2>/dev/null || true
fi

command_exists qemu-system-x86_64 || die "QEMU quedó instalado sin el emulador qemu-system-x86_64."

mkdir -p "$BASE_DIR"

if [ ! -f "$ISO_PATH" ]; then
    log "Descargando la ISO v0.4.0."
    curl --fail --location --retry 3 --connect-timeout 20 "$ISO_URL" -o "$ISO_PATH"
else
    log "La ISO ya existe; se volverá a verificar."
fi

log "Verificando SHA-256 de la ISO."
printf '%s  %s\n' "$ISO_SHA256" "$ISO_PATH" | sha256sum -c - || die "El SHA-256 de la ISO no coincide; elimina $ISO_PATH y vuelve a ejecutar el script."

DISK_FORMAT="qcow2"
if ! command_exists qemu-img; then
    if ! command_exists truncate; then
        die "Falta qemu-img y tampoco existe truncate para crear el disco virtual."
    fi
    DISK_FORMAT="raw"
    if [ ! -f "$DISK_PATH" ]; then
        log "qemu-img no está disponible; creando un disco raw disperso de ${DISK_SIZE}."
        truncate -s "$DISK_SIZE" "$DISK_PATH"
    fi
else
    if [ ! -f "$DISK_PATH" ]; then
        log "Creando disco virtual persistente ${DISK_SIZE}."
        qemu-img create -f qcow2 "$DISK_PATH" "$DISK_SIZE"
    fi
fi

log "Abriendo la actividad Android de Termux:X11."
am start -n com.termux.x11/com.termux.x11.MainActivity >/dev/null 2>&1 || log "No se pudo abrir automáticamente la actividad; ábrela manualmente desde Android."

log "Iniciando servidor Termux:X11 en DISPLAY=${DISPLAY_NUM}."
termux-x11 "$DISPLAY_NUM" -legacy-drawing >"$X11_LOG" 2>&1 &
X11_PID=$!
sleep 2
export DISPLAY="$DISPLAY_NUM"

DISPLAY_BACKEND=""
if qemu-system-x86_64 -display help 2>&1 | grep -qE '^sdl'; then
    DISPLAY_BACKEND="sdl,gl=off"
elif qemu-system-x86_64 -display help 2>&1 | grep -qE '^gtk'; then
    DISPLAY_BACKEND="gtk,gl=off"
else
    die "La compilación instalada de QEMU no ofrece backend SDL/GTK para mostrar la VM en Termux:X11."
fi

log "Iniciando Influent Danenone en QEMU (${RAM_MB} MiB RAM, ${CPUS} CPU, ${DISPLAY_BACKEND})."
exec qemu-system-x86_64 \
    -M q35 \
    -m "$RAM_MB" \
    -smp "$CPUS" \
    -accel tcg,thread=multi \
    -display "$DISPLAY_BACKEND" \
    -vga virtio \
    -drive "file=${DISK_PATH},if=virtio,format=${DISK_FORMAT}" \
    -cdrom "$ISO_PATH" \
    -boot order=d,menu=on \
    -nic user,model=e1000
