#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD=${DANENONE_BUILD_DIR:-$ROOT/build/live-build}
rm -rf "$BUILD"
mkdir -p "$BUILD"
cd "$BUILD"
python3 "$ROOT/iso/make_grub_xpm.py"
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
  --hdd-label INFLUENT \
  --iso-volume INFLUENT \
  --iso-application "Influent Danenone" \
  --iso-publisher "Influent" \
  --bootappend-live "boot=live components username=influent hostname=influent quiet splash" \
  --apt-recommends false
sed -i 's/^LB_KEYRING_PACKAGES=.*/LB_KEYRING_PACKAGES="debian-archive-keyring"/' config/chroot
sed -i 's/^LB_LINUX_FLAVOURS=.*/LB_LINUX_FLAVOURS="amd64"/' config/chroot
sed -i 's/^LB_LINUX_PACKAGES=.*/LB_LINUX_PACKAGES="linux-image"/' config/chroot
mkdir -p config/package-lists config/includes.chroot/opt/influent-danenone config/includes.chroot/opt/influent-danenone/branding config/includes.chroot/etc/calamares/branding/influent config/includes.chroot/etc/calamares/modules config/includes.chroot/usr/lib/influent/packages config/includes.chroot/var/lib/influent config/includes.chroot/usr/local/bin config/includes.chroot/etc/X11 config/includes.chroot/etc/default config/includes.chroot/etc/systemd/system config/includes.binary/boot/grub config/hooks/normal config/hooks
cat > config/package-lists/influent-danenone.list.chroot <<'EOF'
linux-image-amd64
live-boot
systemd-sysv
python3
python3-pyqt5
python3-requests
network-manager
calamares
xorg
xinit
x11-xserver-utils
dbus-x11
nodm
sudo
syslinux-utils
unzip
EOF
cp -a "$ROOT/src" "$ROOT/pyproject.toml" "$ROOT/README.md" config/includes.chroot/opt/influent-danenone/
cp -a "$ROOT/branding/influent-stream-wallpaper.png" config/includes.chroot/opt/influent-danenone/branding/
cp -a "$ROOT/installer/calamares/branding/influent/." config/includes.chroot/etc/calamares/branding/influent/
cp "$ROOT/installer/calamares/settings.conf" config/includes.chroot/etc/calamares/settings.conf
SYSTEM_ARTIFACTS="$ROOT/build/system-fluthin-artifacts"
for package in "$SYSTEM_ARTIFACTS"/*.iflapp; do
  [ -f "$package" ] || continue
  package_name=$(basename "$package" .iflapp)
  mkdir -p "config/includes.chroot/usr/lib/influent/packages/$package_name"
  unzip -q "$package" -d "config/includes.chroot/usr/lib/influent/packages/$package_name"
 done
cat > config/includes.chroot/var/lib/influent/installed-packages.json <<'EOF'
[
  {"package": "/usr/lib/influent/packages/Influent.danenone-shell.v0.3-26.08-21.56-Danenone", "details_xml": "/usr/lib/influent/packages/Influent.danenone-shell.v0.3-26.08-21.56-Danenone/details.xml"},
  {"package": "/usr/lib/influent/packages/Influent.influent-updater.v0.3-26.08-21.56-Danenone", "details_xml": "/usr/lib/influent/packages/Influent.influent-updater.v0.3-26.08-21.56-Danenone/details.xml"},
  {"package": "/usr/lib/influent/packages/Influent.influent-notifications.v0.3-26.08-21.56-Danenone", "details_xml": "/usr/lib/influent/packages/Influent.influent-notifications.v0.3-26.08-21.56-Danenone/details.xml"}
]
EOF
cp "$ROOT/build/influent-boot.xpm.gz" config/includes.binary/boot/grub/influent-boot.xpm.gz
cat > config/includes.chroot/usr/local/bin/influent-danenone-session <<'EOF'
#!/bin/sh
export PYTHONPATH=/opt/influent-danenone/src
export QT_QPA_PLATFORM=${QT_QPA_PLATFORM:-xcb}
exec /usr/lib/influent/packages/Influent.danenone-shell.v0.3-26.08-21.56-Danenone/app/app
EOF
chmod +x config/includes.chroot/usr/local/bin/influent-danenone-session
cat > config/includes.chroot/etc/X11/Xwrapper.config <<'EOF'
allowed_users=anybody
needs_root_rights=yes
EOF
cat > config/includes.chroot/etc/default/nodm <<'EOF'
NODM_ENABLED=true
NODM_USER=influent
NODM_XSESSION=/usr/local/bin/influent-danenone-session
NODM_X_OPTIONS="-nolisten tcp"
NODM_MIN_SESSION_TIME=2
EOF
cat > config/includes.chroot/etc/systemd/system/influent-danenone.service <<'EOF'
[Unit]
Description=Influent Danenone graphical shell
After=systemd-user-sessions.service live-config.service
Wants=network-online.target

[Service]
Type=simple
User=influent
Environment=HOME=/home/influent
Environment=PYTHONPATH=/opt/influent-danenone/src
Environment=INFLUENT_PACKAGE_ROOT=/usr/lib/influent/packages
Environment=QT_QPA_PLATFORM=xcb
ExecStart=/usr/bin/startx /usr/local/bin/influent-danenone-session -- :0 vt1 -keeptty -nolisten tcp
Restart=on-failure
RestartSec=2
TTYPath=/dev/tty1
TTYReset=yes
TTYVHangup=yes
TTYVTDisallocate=yes

[Install]
WantedBy=multi-user.target
EOF
cat > config/hooks/9999-influent-danenone.chroot <<'EOF'
#!/bin/sh
set -eu
useradd --create-home --shell /bin/bash influent || true
usermod -aG audio,video,render,input influent || true
mkdir -p /home/influent
cat > /home/influent/.bash_profile <<'PROFILE'
#!/bin/sh
if [ "$(tty 2>/dev/null || true)" = "/dev/tty1" ] && [ -z "${DISPLAY:-}" ]; then
  exec startx /usr/local/bin/influent-danenone-session -- :0 vt1 -keeptty -nolisten tcp
fi
PROFILE
chmod 0755 /home/influent/.bash_profile
chown -R influent:influent /home/influent
EOF
chmod +x config/hooks/9999-influent-danenone.chroot
cat > config/hooks/9999-influent-danenone-grub.binary <<'EOF'
#!/bin/sh
set -eu
if [ -f binary/boot/grub/menu.lst ]; then
  sed -i \
    -e 's/Debian GNU\/Linux - live/Influent Danenone - Live Session/g' \
    -e 's/Debian GNU\/Linux/Influent Danenone/g' \
    -e 's/Ubuntu/Influent/g' \
    binary/boot/grub/menu.lst
  if [ -f binary/boot/grub/influent-boot.xpm.gz ]; then
    sed -i -E 's#^splashimage[[:space:]].*#splashimage (cd)/boot/grub/influent-boot.xpm.gz#' binary/boot/grub/menu.lst || true
  fi
fi
EOF
chmod +x config/hooks/9999-influent-danenone-grub.binary
lb build
mkdir -p "$ROOT/build"
cp binary.iso "$ROOT/build/influent-danenone-0.2.0-preview-amd64.iso"
sha256sum "$ROOT/build/influent-danenone-0.2.0-preview-amd64.iso" > "$ROOT/build/influent-danenone-0.2.0-preview-amd64.iso.sha256"
if [ -n "${SUDO_USER:-}" ]; then
  chown "$SUDO_USER:${SUDO_USER}" "$ROOT/build/influent-danenone-0.2.0-preview-amd64.iso" "$ROOT/build/influent-danenone-0.2.0-preview-amd64.iso.sha256" || true
fi
