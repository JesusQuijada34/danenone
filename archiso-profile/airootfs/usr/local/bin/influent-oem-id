#!/bin/sh
set -eu

STATE_DIR="${XDG_STATE_HOME:-${HOME:-/tmp}/.local/state}/influent-danenone"
SYSTEM_DIR="/var/lib/influent-danenone"
DMI_DIR="/sys/class/dmi/id"

read_value() {
    file="$1"
    if [ -r "$DMI_DIR/$file" ]; then
        tr '\n' ' ' < "$DMI_DIR/$file" | sed 's/[[:space:]]\+/ /g; s/^ //; s/ $//'
    else
        printf '%s' "unknown"
    fi
}

machine_id="unknown"
if [ -r /etc/machine-id ]; then
    machine_id=$(tr -d '\n' < /etc/machine-id)
fi

sys_vendor=$(read_value sys_vendor)
product_name=$(read_value product_name)
product_version=$(read_value product_version)
board_vendor=$(read_value board_vendor)
board_name=$(read_value board_name)
bios_vendor=$(read_value bios_vendor)
bios_version=$(read_value bios_version)
product_uuid=$(read_value product_uuid)

stable_material=$(printf '%s\n' "$machine_id" "$sys_vendor" "$product_name" "$product_version" "$board_vendor" "$board_name" "$bios_vendor" "$bios_version" "$product_uuid")
oem_id=$(printf '%s' "$stable_material" | sha256sum | awk '{print $1}')

if [ "${1:-}" = "--json" ]; then
    printf '{"oem_id":"%s","vendor":"%s","product":"%s","version":"%s","board":"%s","bios_vendor":"%s","bios_version":"%s"}\n' "$oem_id" "$sys_vendor" "$product_name" "$product_version" "$board_name" "$bios_vendor" "$bios_version"
    exit 0
fi

if [ "${1:-}" = "--refresh" ] || [ "$#" -eq 0 ]; then
    if [ "$(id -u)" -eq 0 ]; then
        output_dir="$SYSTEM_DIR"
        output_path="$output_dir/oem-id.conf"
        mode=0644
    else
        output_dir="$STATE_DIR"
        output_path="$output_dir/oem-id.conf"
        mode=0600
    fi
    umask 077
    mkdir -p "$output_dir"
    tmp="$output_path.tmp.$$"
    {
        printf 'OEM_ID=%s\n' "$oem_id"
        printf 'OEM_VENDOR=%s\n' "$sys_vendor"
        printf 'OEM_PRODUCT=%s\n' "$product_name"
        printf 'OEM_PRODUCT_VERSION=%s\n' "$product_version"
        printf 'OEM_BOARD=%s\n' "$board_name"
        printf 'OEM_BIOS_VENDOR=%s\n' "$bios_vendor"
        printf 'OEM_BIOS_VERSION=%s\n' "$bios_version"
        printf 'OEM_SERIAL_EXPOSED=false\n'
    } > "$tmp"
    chmod "$mode" "$tmp"
    mv -f "$tmp" "$output_path"
fi

printf 'OEM_ID=%s\n' "$oem_id"
printf 'OEM_VENDOR=%s\n' "$sys_vendor"
printf 'OEM_PRODUCT=%s\n' "$product_name"
printf 'OEM_PRODUCT_VERSION=%s\n' "$product_version"
printf 'OEM_BOARD=%s\n' "$board_name"
printf 'OEM_SERIAL_EXPOSED=false\n'
