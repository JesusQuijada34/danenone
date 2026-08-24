#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  printf 'Uso: %s <directorio-de-paquetes> <directorio-repositorio>\n' "$0" >&2
  exit 2
fi

SOURCE_DIR="$(realpath -e "$1")"
REPO_DIR="$(realpath -m "$2")"
if [[ ! -d "$SOURCE_DIR" || "$SOURCE_DIR" == "$REPO_DIR" ]]; then
  printf 'El origen debe existir y el repositorio de salida debe ser distinto.\n' >&2
  exit 3
fi

runtime=("$SOURCE_DIR"/danenone-calamares-[0-9]*.pkg.tar.zst)
template=("$SOURCE_DIR"/danenone-calamares-config-*.pkg.tar.zst)
if [[ ${#runtime[@]} -ne 1 || ! -f "${runtime[0]}" || ${#template[@]} -ne 1 || ! -f "${template[0]}" ]]; then
  printf 'Se requieren exactamente un runtime y una plantilla Calamares empaquetados.\n' >&2
  exit 4
fi

command -v repo-add >/dev/null 2>&1 || {
  printf 'repo-add debe estar disponible en el entorno Arch de construcción.\n' >&2
  exit 5
}

mkdir -p "$REPO_DIR"
cp -f -- "${runtime[0]}" "${template[0]}" "$REPO_DIR/"
(cd "$REPO_DIR" && sha256sum danenone-calamares-[0-9]*.pkg.tar.zst danenone-calamares-config-*.pkg.tar.zst > SHA256SUMS)
repo-add -R "$REPO_DIR/danenone-lab.db.tar.zst" "$REPO_DIR"/*.pkg.tar.zst
printf 'Repositorio Plasma Lab preparado: %s\n' "$REPO_DIR"
