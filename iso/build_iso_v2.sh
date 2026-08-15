#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD=${DANENONE_V2_BUILD_DIR:-$ROOT/build/live-build-v2}
OUTPUT="$ROOT/build/influent-danenone-v2-amd64.iso"
rm -rf "$BUILD"
mkdir -p "$BUILD"
cd "$BUILD"
lb config \
  --distribution bookworm \
  --mirror-bootstrap http://deb.debian.org/debian \
  --mirror-binary http://deb.debian.org/debian \
  --mirror-binary-security http://security.debian.org/debian-security \
  --mirror-chroot-security http://security.debian.org/debian-security \
  --archive-areas "main contrib non-free-firmware" \
  --binary-images iso \
  --bootloader grub \
  --memtest none \
  --debian-installer false \
  --initramfs live-boot \
  --initsystem systemd \
  --hdd-label INFLUENT_DANENONE \
  --iso-volume INFLUENT_DANENONE \
  --iso-application "Influent Danenone v2" \
  --iso-publisher "Influent" \
  --bootappend-live "boot=live components username=danenone hostname=danenone quiet splash" \
  --apt-recommends false
mkdir -p config/package-lists config/includes.chroot/opt/influent-danenone/src config/includes.chroot/opt/influent-danenone/branding config/includes.chroot/usr/local/bin config/includes.chroot/etc/default config/includes.chroot/etc/X11 config/includes.chroot/etc/systemd/system config/hooks/normal config/hooks
cat > config/package-lists/influent-danenone-v2.list.chroot <<'EOF'
linux-image-amd64
live-boot
systemd-sysv
python3
python3-pyqt5
python3-pil
network-manager
xorg
xinit
x11-xserver-utils
dbus-x11
nodm
calamares
EOF
cp -a "$ROOT/src/danenone_v2" config/includes.chroot/opt/influent-danenone/src/
cp "$ROOT/branding-v2/danenone-river-wallpaper.jpg" config/includes.chroot/opt/influent-danenone/branding/
cat > config/includes.chroot/usr/local/bin/influent-danenone-session <<'EOF'
#!/bin/sh
set -eu
export PYTHONPATH=/opt/influent-danenone/src
export DANENONE_BRANDING=/opt/influent-danenone/branding
export QT_QPA_PLATFORM=${QT_QPA_PLATFORM:-xcb}
exec /usr/bin/python3 -m danenone_v2.main
EOF
chmod 0755 config/includes.chroot/usr/local/bin/influent-danenone-session
cat > config/includes.chroot/etc/X11/Xwrapper.config <<'EOF'
allowed_users=anybody
needs_root_rights=yes
EOF
cat > config/includes.chroot/etc/default/nodm <<'EOF'
NODM_ENABLED=true
NODM_USER=danenone
NODM_XSESSION=/usr/local/bin/influent-danenone-session
NODM_X_OPTIONS="-nolisten tcp"
NODM_MIN_SESSION_TIME=2
EOF
cat > config/hooks/normal/9999-influent-danenone-v2.chroot <<'EOF'
#!/bin/sh
set -eu
useradd --create-home --shell /bin/bash danenone || true
usermod -aG audio,video,render,input danenone || true
mkdir -p /home/danenone
chown -R danenone:danenone /home/danenone
systemctl enable nodm || true
EOF
chmod 0755 config/hooks/normal/9999-influent-danenone-v2.chroot
lb build
mkdir -p "$ROOT/build"
cp binary.iso "$OUTPUT"
sha256sum "$OUTPUT" > "$OUTPUT.sha256"
if [ -n "${SUDO_USER:-}" ]; then
  chown "$SUDO_USER:${SUDO_USER}" "$OUTPUT" "$OUTPUT.sha256" || true
fi
printf '%s\n' "$OUTPUT"
