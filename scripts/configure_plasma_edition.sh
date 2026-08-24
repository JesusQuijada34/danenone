#!/usr/bin/env bash
set -euo pipefail

PROFILE="${1:-}"
EDITION="${2:-plasma}"
if [[ -z "$PROFILE" || ! -d "$PROFILE/airootfs" || ! -f "$PROFILE/packages.x86_64" ]]; then
  printf 'Uso: %s <perfil-archiso>\n' "$0" >&2
  exit 2
fi

ROOTFS="$PROFILE/airootfs"

# La variante Plasma de laboratorio usa identidad propia. La RC publicada
# conserva su nombre y versión originales cuando se prepara la edición plasma.
if [[ "$EDITION" == "plasma-lab" ]]; then
  sed -i -E \
    -e 's/^iso_name=".*"$/iso_name="influent-danenone-plasma-lab"/' \
    -e 's/^iso_application=".*"$/iso_application="Influent Danenone Plasma Lab — KDE Plasma + Hyprland avanzado"/' \
    -e 's/^iso_version=".*"$/iso_version="0.6.0-lab"/' \
    "$PROFILE/profiledef.sh"
else
  sed -i -E \
    -e 's/^iso_name=".*"$/iso_name="influent-danenone-plasma"/' \
    -e 's/^iso_application=".*"$/iso_application="Influent Danenone Plasma — KDE Plasma + Hyprland avanzado"/' \
    -e 's/^iso_version=".*"$/iso_version="0.5.0-rc1"/' \
    "$PROFILE/profiledef.sh"
fi

# El perfil Plasma sustituye el inicio directo por greetd, pero mantiene el
# paquete y la configuración de Hyprland como una sesión avanzada elegible.
sed -i -E '/^greetd$/d' "$PROFILE/packages.x86_64"
rm -f "$ROOTFS/etc/greetd/config.toml"
rm -f "$ROOTFS/etc/systemd/system/getty@tty1.service.d/autologin.conf"

# El perfil base habilita greetd. En Plasma el paquete ya no está presente;
# eliminar esa línea evita errores de unidades inexistentes en mkarchiso.
sed -i -E '/^systemctl enable greetd\.service \|\| true$/d' "$ROOTFS/root/customize_airootfs.sh"

install -d -m 0755 "$ROOTFS/etc/sddm.conf.d" "$ROOTFS/var/lib/sddm" "$ROOTFS/etc/influent-danenone"
cat > "$ROOTFS/etc/sddm.conf.d/10-influent-danenone.conf" <<'EOF'
[Theme]
Current=breeze

[Users]
# Plasma se fija para la sesión live; Hyprland sólo se elige manualmente.
RememberLastSession=false
RememberLastUser=false
EOF

# SDDM preselecciona Plasma Wayland para la cuenta live sin ocultar el
# selector. Hyprland continúa disponible como alternativa avanzada desde su
# archivo .desktop instalado.
cat > "$ROOTFS/var/lib/sddm/state.conf" <<'EOF'
[Last]
User=danenone
Session=plasma.desktop
EOF

cat > "$ROOTFS/etc/influent-danenone/session-policy.conf" <<'EOF'
DISPLAY_MANAGER=sddm
DEFAULT_SESSION=plasma.desktop
ADVANCED_SESSION=hyprland.desktop
EOF

cat > "$ROOTFS/etc/motd" <<'EOF'
Influent Danenone Plasma — entorno live

Conectividad: NetworkManager
Escritorio predeterminado: KDE Plasma / KWin
Sesión avanzada disponible: Hyprland
Asistente: configuración inicial Danenone

Consulta la documentación del proyecto en:
https://github.com/JesusQuijada34/danenone
EOF

# La edición Plasma habilita sólo SDDM durante la construcción del sistema live.
cat >> "$ROOTFS/root/customize_airootfs.sh" <<'EOF'

# La edición Plasma usa SDDM; Hyprland queda como sesión avanzada del selector.
systemctl enable sddm.service || true
EOF
