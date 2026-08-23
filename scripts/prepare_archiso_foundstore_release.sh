#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROFILE="${1:-}"

if [[ -z "$PROFILE" || ! -f "$PROFILE/packages.x86_64" || ! -d "$PROFILE/airootfs" ]]; then
  printf 'Uso: %s <perfil-archiso>\n' "$0" >&2
  exit 2
fi

# shellcheck source=foundstore-release.env
source "$ROOT/scripts/foundstore-release.env"
WORK_DIR="${DANENONE_RELEASE_CACHE:-$ROOT/build/release-cache}"
ARTIFACT="${FOUNSTORE_ARTIFACT_PATH:-$WORK_DIR/$FOUNSTORE_ASSET}"
mkdir -p "$WORK_DIR"

if [[ ! -f "$ARTIFACT" ]]; then
  curl --fail --location --retry 3 --proto '=https' --tlsv1.2 "$FOUNSTORE_URL" --output "$ARTIFACT"
fi

printf '%s  %s\n' "$FOUNSTORE_SHA256" "$ARTIFACT" | sha256sum --check --status
python3 "$ROOT/scripts/integrate_foundstore_release.py" --artifact "$ARTIFACT" --profile "$PROFILE"
