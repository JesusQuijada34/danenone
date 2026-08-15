#!/usr/bin/env bash
# shellcheck disable=SC2034
iso_name="influent-danenone"
iso_label="INFLUENT_DANENONE"
iso_publisher="Influent <https://github.com/JesusQuijada34/danenone>"
iso_application="Influent Danenone — Linux + Hyprland"
iso_version="0.4.0"
install_dir="influent"
buildmodes=('iso')
bootmodes=('bios.syslinux' 'uefi.systemd-boot')
pacman_conf="pacman.conf"
airootfs_image_type="squashfs"
airootfs_image_tool_options=('-comp' 'xz' '-Xbcj' 'x86' '-b' '1M' '-Xdict-size' '1M')
bootstrap_tarball_compression=('zstd' '-c' '-T0' '--auto-threads=logical' '--long' '-19')
file_permissions=(
  ["/etc/shadow"]="0:0:400"
  ["/root"]="0:0:750"
  ["/root/.automated_script.sh"]="0:0:755"
  ["/usr/local/bin/influent-danenone-shell"]="0:0:755"
  ["/usr/local/bin/influent-danenone-splashboot"]="0:0:755"
  ["/usr/local/bin/influent-danenone-tour"]="0:0:755"
  ["/usr/local/bin/influent-danenone-session"]="0:0:755"
  ["/root/customize_airootfs.sh"]="0:0:755"
)
