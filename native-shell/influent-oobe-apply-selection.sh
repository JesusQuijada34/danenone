#!/bin/sh
set -eu

selection_file="${OOBE_SELECTION_FILE:-${XDG_STATE_HOME:-${HOME:-/tmp}/.local/state}/influent-danenone/oobe-selection.conf}"
target_root="${DANENONE_TARGET_ROOT:-/}"
source_root="${DANENONE_SOURCE_ROOT:-/usr/share/influent-danenone/editions}"

[ -r "$selection_file" ] || exit 0
edition=$(sed -n 's/^EDITION=//p' "$selection_file" | head -1)
language=$(sed -n 's/^LANGUAGE=//p' "$selection_file" | head -1)
case "$edition" in
    home|enterprise|developer|minimal|frozen-lab) ;;
    *) exit 2 ;;
esac
case "$language" in
    en|[A-Za-z0-9_-][A-Za-z0-9_-]*) ;;
    *) exit 2 ;;
esac

language_manifest="${DANENONE_LANGUAGE_MANIFEST:-/usr/share/influent/languages/manifest.tsv}"
if [ ! -r "$language_manifest" ] && [ -r "$(dirname "$source_root")/languages/manifest.tsv" ]; then
    language_manifest="$(dirname "$source_root")/languages/manifest.tsv"
fi
if [ "$language" != "en" ] && { ! printf '%s\n' "$language" | grep -Eq '^[A-Za-z0-9_-]{2,16}$' || ! grep -q "^${language}|" "$language_manifest" 2>/dev/null; }; then
    exit 2
fi

source_file="$source_root/$edition.conf"
target_dir="$target_root/etc/influent-danenone"
target_file="$target_dir/edition.conf"
[ -r "$source_file" ] || exit 3
mkdir -p "$target_dir"
tmp="$target_file.tmp.$$"
{
    cat "$source_file"
    printf 'SELECTED_LANGUAGE=%s\n' "$language"
    printf 'SELECTED_EDITION=%s\n' "$edition"
} > "$tmp"
chmod 0644 "$tmp"
mv -f "$tmp" "$target_file"
exit 0
