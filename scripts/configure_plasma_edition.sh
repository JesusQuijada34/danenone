#!/usr/bin/env bash
set -euo pipefail

PROFILE="${1:-}"
if [[ -z "$PROFILE" || ! -d "$PROFILE/airootfs" || ! -f "$PROFILE/packages.x86_64" ]]; then
  printf 'Uso: %s <perfil-archiso>\n' "$0" >&2
  exit 2
fi

ROOTFS="$PROFILE/airootfs"

# El perfil Plasma sustituye el inicio directo por greetd, pero mantiene el
# paquete y la configuración de Hyprland como una sesión avanzada elegible.
sed -i -E '/^greetd$/d' "$PROFILE/packages.x86_64"
rm -f "$ROOTFS/etc/greetd/config.toml"
rm -f "$ROOTFS/etc/systemd/system/getty@tty1.service.d/autologin.conf"

install -d -m 0755 "$ROOTFS/etc/sddm.conf.d" "$ROOTFS/var/lib/sddm" "$ROOTFS/etc/influent-danenone"
cat > "$ROOTFS/etc/sddm.conf.d/10-influent-danenone.conf" <<'EOF'
[Theme]
Current=breeze
EOF

# SDDM preselecciona Plasma Wayland para la cuenta live sin ocultar el
# selector. Hyprland continúa disponible como alternativa avanzada desde su
# archivo .desktop instalado.
cat > "$ROOTFS/var/lib/sddm/state.conf" <<'EOF'
[Last]
User=danenone
Session=plasmawayland.desktop
EOF

cat > "$ROOTFS/etc/influent-danenone/session-policy.conf" <<'EOF'
DISPLAY_MANAGER=sddm
DEFAULT_SESSION=plasmawayland.desktop
ADVANCED_SESSION=hyprland.desktop
EOF

# El script base habilita greetd. Añadimos la transición al final para que el
# resultado sea inequívoco durante la construcción del sistema live.
cat >> "$ROOTFS/root/customize_airootfs.sh" <<'EOF'

# La edición Plasma usa SDDM; Hyprland queda como sesión avanzada del selector.
systemctl disable greetd.service || true
systemctl enable sddm.service || true
EOF
