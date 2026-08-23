import hashlib
import struct
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CURRENT_WALLPAPER = ROOT / "branding" / "influent-stream-wallpaper.png"
GRUB_WALLPAPER = ROOT / "archiso-profile" / "grub" / "influent-stream-wallpaper.png"
SYSLINUX_WALLPAPER = ROOT / "archiso-profile" / "syslinux" / "influent-stream-wallpaper.png"
SYSLINUX_PACKAGED_SPLASH = ROOT / "archiso-profile" / "syslinux" / "splash.png"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def png_dimensions(path: Path) -> tuple[int, int]:
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        raise ValueError(f"PNG inválido: {path}")
    return struct.unpack(">II", data[16:24])


class BootBrandingTests(unittest.TestCase):
    def test_bootloaders_use_the_current_unmodified_wallpaper(self):
        self.assertEqual(sha256(CURRENT_WALLPAPER), sha256(GRUB_WALLPAPER))
        self.assertEqual(sha256(CURRENT_WALLPAPER), sha256(SYSLINUX_WALLPAPER))
        self.assertEqual(png_dimensions(SYSLINUX_PACKAGED_SPLASH), (640, 480))

    def test_grub_and_syslinux_reference_their_current_wallpapers(self):
        grub = (ROOT / "archiso-profile" / "grub" / "grub.cfg").read_text(encoding="utf-8")
        syslinux = (ROOT / "archiso-profile" / "syslinux" / "archiso_head.cfg").read_text(encoding="utf-8")
        self.assertIn("insmod png", grub)
        self.assertIn("insmod gfxterm", grub)
        self.assertIn("terminal_output gfxterm", grub)
        self.assertIn("background_image /boot/grub/influent-stream-wallpaper.png", grub)
        self.assertIn("timeout=5", grub)
        self.assertIn("timeout_style=menu", grub)
        self.assertIn("MENU BACKGROUND splash.png", syslinux)
        self.assertIn("Plasma Edition", syslinux)


if __name__ == "__main__":
    unittest.main()
