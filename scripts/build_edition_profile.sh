#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EDITION="${1:-}"
OUTPUT="${2:-$ROOT/build/archiso-editions/$EDITION}"

if [[ -z "$EDITION" || ! -f "$ROOT/editions/$EDITION.conf" ]]; then
  printf 'Uso: %s <home|enterprise|developer|minimal|frozen-lab|plasma|plasma-lab> [directorio-salida]\n' "$0" >&2
  exit 2
fi

PACKAGE_SET="$(awk -F= '$1=="PACKAGE_SET" {print $2}' "$ROOT/editions/$EDITION.conf")"
PACKAGE_FILE="$ROOT/packages/editions/$PACKAGE_SET.packages"
if [[ ! -f "$PACKAGE_FILE" ]]; then
  printf 'No existe el conjunto de paquetes: %s\n' "$PACKAGE_FILE" >&2
  exit 3
fi

rm -rf "$OUTPUT"
mkdir -p "$(dirname "$OUTPUT")"
cp -a "$ROOT/archiso-profile" "$OUTPUT"

# El perfil base contiene el shell común. Las ediciones ligeras retiran aplicaciones pesadas.
if [[ "$EDITION" == "minimal" || "$EDITION" == "frozen-lab" ]]; then
  sed -i -E '/^(firefox|dolphin|vlc|qt6-declarative|qt6-svg)$/d' "$OUTPUT/packages.x86_64"
fi

# Los toolchains se añaden solo a Enterprise/Developer mediante su fichero declarado.
cat "$PACKAGE_FILE" >> "$OUTPUT/packages.x86_64"
install -Dm644 "$ROOT/editions/$EDITION.conf" "$OUTPUT/airootfs/etc/influent-danenone/edition.conf"
install -Dm644 "$PACKAGE_FILE" "$OUTPUT/airootfs/etc/influent-danenone/packages.list"
printf '%s\n' "EDITION_PROFILE=$EDITION" > "$OUTPUT/airootfs/etc/influent-danenone/profile-generated"
if [[ "$EDITION" == "plasma" || "$EDITION" == "plasma-lab" ]]; then
  "$ROOT/scripts/configure_plasma_edition.sh" "$OUTPUT" "$EDITION"
fi
if [[ "$EDITION" == "plasma-lab" ]]; then
  LAB_REPO="${DANENONE_CALAMARES_LAB_REPO:-}"
  if [[ -z "$LAB_REPO" || ! -f "$LAB_REPO/danenone-lab.db.tar.zst" || ! -f "$LAB_REPO/SHA256SUMS" ]]; then
    printf 'Plasma Lab requiere DANENONE_CALAMARES_LAB_REPO con danenone-lab.db.tar.zst y SHA256SUMS.\n' >&2
    exit 4
  fi
  LAB_REPO="$(realpath -e "$LAB_REPO")"
  if ! (cd "$LAB_REPO" && sha256sum -c SHA256SUMS); then
    printf 'El manifiesto SHA256 del repositorio Plasma Lab no es válido.\n' >&2
    exit 5
  fi
  sed -i '78i[danenone-lab]\nSigLevel = Optional\nServer = file://'"$LAB_REPO"'\n' "$OUTPUT/pacman.conf"
fi
"$ROOT/scripts/prepare_archiso_foundstore_release.sh" "$OUTPUT"
printf 'Perfil preparado: %s\nSalida: %s\n' "$EDITION" "$OUTPUT"
printf 'No se ejecuta mkarchiso automáticamente; la compilación requiere un entorno Arch con los paquetes disponibles.\n'
