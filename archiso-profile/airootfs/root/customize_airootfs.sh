#!/usr/bin/env bash
set -euo pipefail

useradd -m -U -s /bin/bash danenone || true
usermod -aG video,audio,input,network,storage,seat danenone || true
passwd -d danenone || true
mkdir -p /home/danenone/.config/hypr /home/danenone/.config/foot
cp /etc/hypr/hyprland.conf /home/danenone/.config/hypr/hyprland.conf
cat > /home/danenone/.config/hypr/hyprpaper.conf <<'EOF'
preload = /usr/share/backgrounds/influent/danenone-river-wallpaper.jpg
wallpaper = ,/usr/share/backgrounds/influent/danenone-river-wallpaper.jpg
splash = false
EOF
cat > /home/danenone/.config/foot/foot.ini <<'EOF'
font=monospace:size=11
pad=12x10
[colors]
alpha=0.92
background=07101f
foreground=f2f6ff
EOF
chown -R danenone:danenone /home/danenone
systemctl enable NetworkManager.service || true
systemctl enable influent-oem-id.service || true
systemctl enable greetd.service || true

# Aplicar identidad propia después de instalar el paquete filesystem.
cat > /usr/lib/os-release <<'EOF'
NAME="Influent Danenone"
PRETTY_NAME="Influent Danenone"
ID=influent
ID_LIKE=arch
BUILD_ID=danenone
ANSI_COLOR="38;2;84;126;190"
HOME_URL="https://github.com/JesusQuijada34/danenone"
DOCUMENTATION_URL="https://github.com/JesusQuijada34/danenone"
SUPPORT_URL="https://github.com/JesusQuijada34/danenone/issues"
BUG_REPORT_URL="https://github.com/JesusQuijada34/danenone/issues"
PRIVACY_POLICY_URL="https://github.com/JesusQuijada34/danenone"
LOGO=influent-danenone
IMAGE_ID=influent-danenone
IMAGE_VERSION=0.4.2
EOF
