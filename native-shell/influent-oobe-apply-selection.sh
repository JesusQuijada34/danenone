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
    en_US|es_419|es_ES|pt_BR) ;;
    *) exit 2 ;;
esac

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
