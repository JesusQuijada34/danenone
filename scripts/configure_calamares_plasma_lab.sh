#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROFILE="${1:-}"
EDITION="${2:-}"

if [[ "$EDITION" != "plasma-lab" ]]; then
  printf 'La configuración ejecutable Calamares sólo está permitida para plasma-lab.\n' >&2
  exit 2
fi

if [[ "${DANENONE_CALAMARES_LAB_ENABLE:-}" != "qcow2-only" ]]; then
  printf 'Defina DANENONE_CALAMARES_LAB_ENABLE=qcow2-only para generar Calamares Plasma Lab.\n' >&2
  exit 3
fi

if [[ -z "$PROFILE" || ! -d "$PROFILE/airootfs" || ! -f "$PROFILE/profiledef.sh" ]]; then
  printf 'Perfil Archiso inválido: %s\n' "$PROFILE" >&2
  exit 4
fi

PROFILE="$(realpath -e "$PROFILE")"
DEST="$PROFILE/airootfs/etc/calamares"
ROOTFS="$(realpath -e "$PROFILE/airootfs")"

if [[ -e "$DEST" ]]; then
  printf 'El perfil Plasma Lab ya contiene /etc/calamares y no se sobrescribirá.\n' >&2
  exit 5
fi

mkdir -p "$DEST/modules"
DEST_REAL="$(realpath -e "$DEST")"
case "$DEST_REAL" in
  "$ROOTFS"/*) ;;
  *)
    printf 'La ruta Calamares escapa del rootfs temporal: %s\n' "$DEST_REAL" >&2
    exit 6
    ;;
esac

cat > "$DEST/settings.conf" <<'EOF'
---
# Generado sólo para la ISO Plasma Lab; no copiar al perfil base ni a RC1.
modules-search: [ local ]
sequence:
- show:
  - welcome
  - locale
  - keyboard
  - partition
  - users
  - summary
- exec:
  - partition
  - mount
  - unpackfs
  - fstab
  - locale
  - users
  - displaymanager
  - bootloader
  - umount
- show:
  - finished
branding: influent-danenone
prompt-install: true
dont-chroot: false
oem-setup: false
disable-cancel: false
disable-cancel-during-exec: true
hide-back-and-next-during-exec: true
quit-at-end: false
EOF

cat > "$DEST/modules/partition.conf" <<'EOF'
---
# El usuario elige y confirma el disco dentro de Calamares.
initialPartitioningChoice: none
initialSwapChoice: none
defaultFileSystemType: "ext4"
drawNestedPartitions: false
alwaysShowPartitionLabels: true
EOF

cat > "$DEST/modules/mount.conf" <<'EOF'
---
extraMounts:
  - device: proc
    fs: proc
    mountPoint: /proc
  - device: sys
    fs: sysfs
    mountPoint: /sys
  - device: /dev
    mountPoint: /dev
    options: [ bind ]
  - device: tmpfs
    fs: tmpfs
    mountPoint: /run
  - device: /run/udev
    mountPoint: /run/udev
    options: [ bind ]
  - device: efivarfs
    fs: efivarfs
    mountPoint: /sys/firmware/efi/efivars
    efi: true
mountOptions:
  - filesystem: default
    options: [ defaults ]
  - filesystem: efi
    options: [ defaults, umask=0077 ]
EOF

cat > "$DEST/modules/unpackfs.conf" <<'EOF'
---
# Fuente Archiso de la ISO Lab, resuelta sólo por el entorno live en ejecución.
unpack:
  - source: "/run/archiso/bootmnt/influent/x86_64/airootfs.sfs"
    sourcefs: "squashfs"
    destination: ""
EOF

cat > "$DEST/modules/fstab.conf" <<'EOF'
---
crypttabOptions: luks
tmpOptions:
  default:
    tmpfs: false
    options: ""
  ssd:
    tmpfs: true
    options: "defaults,noatime,mode=1777"
EOF

cat > "$DEST/modules/locale.conf" <<'EOF'
---
# La persona escoge idioma y zona horaria sin consultas de red.
region: "America"
zone: "Havana"
EOF

cat > "$DEST/modules/users.conf" <<'EOF'
---
# Calamares recibe los datos de usuario durante el flujo; no hay secretos preconfigurados.
defaultGroups: [ users, lp, video, network, storage, wheel, audio ]
autologinGroup: autologin
doAutologin: false
sudoersGroup: wheel
setRootPassword: false
passwordRequirements:
  minLength: 8
  maxLength: -1
userShell: /bin/bash
EOF

cat > "$DEST/modules/displaymanager.conf" <<'EOF'
---
displaymanagers: [ sddm ]
basicSetup: false
sysconfigSetup: false
sddm:
  configuration_file: "/etc/sddm.conf"
EOF

cat > "$DEST/modules/bootloader.conf" <<'EOF'
---
efiBootLoader: "grub"
grubInstall: "grub-install"
grubMkconfig: "grub-mkconfig"
grubCfg: "/boot/grub/grub.cfg"
grubProbe: "grub-probe"
efiBootMgr: "efibootmgr"
installEFIFallback: true
installHybridGRUB: false
EOF

cat > "$DEST/modules/umount.conf" <<'EOF'
---
ignoreFailedMounts: false
EOF

for branding_file in branding.desc show.qml stylesheet.qss influent-stream-wallpaper.png influent-danenone-boot.png; do
  install -Dm644 "$ROOT/packaging/calamares-config/$branding_file" \
    "$DEST/branding/influent-danenone/$branding_file"
done

printf 'Configuración ejecutable Calamares creada sólo en: %s\n' "$DEST"
