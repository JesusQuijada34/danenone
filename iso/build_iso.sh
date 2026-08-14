#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD=${DANENONE_BUILD_DIR:-$ROOT/build/live-build}
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
  --hdd-label DANENONE \
  --iso-volume DANENONE \
  --bootappend-live "boot=live components username=danenone hostname=danenone" \
  --apt-recommends false
sed -i 's/^LB_KEYRING_PACKAGES=.*/LB_KEYRING_PACKAGES="debian-archive-keyring"/' config/chroot
sed -i 's/^LB_LINUX_FLAVOURS=.*/LB_LINUX_FLAVOURS="amd64"/' config/chroot
sed -i 's/^LB_LINUX_PACKAGES=.*/LB_LINUX_PACKAGES="linux-image"/' config/chroot
mkdir -p config/package-lists config/includes.chroot/opt/danenone config/includes.chroot/usr/local/bin
cat > config/package-lists/danenone.list.chroot <<'EOF'
linux-image-amd64
live-boot
systemd-sysv
python3
python3-pyqt5
python3-requests
network-manager
sudo
syslinux-utils
EOF
cp -a "$ROOT/src" "$ROOT/pyproject.toml" "$ROOT/README.md" config/includes.chroot/opt/danenone/
cat > config/includes.chroot/usr/local/bin/danenone-session <<'EOF'
#!/bin/sh
export PYTHONPATH=/opt/danenone/src
exec python3 -m danenone_shell.app
EOF
chmod +x config/includes.chroot/usr/local/bin/danenone-session
lb build
mkdir -p "$ROOT/build"
cp binary.iso "$ROOT/build/danenone-0.1.0-prototype-amd64.iso"
sha256sum "$ROOT/build/danenone-0.1.0-prototype-amd64.iso" > "$ROOT/build/danenone-0.1.0-prototype-amd64.iso.sha256"
