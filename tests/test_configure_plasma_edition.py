import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONFIGURATOR = ROOT / "scripts" / "configure_plasma_edition.sh"


class ConfigurePlasmaEditionTests(unittest.TestCase):
    def create_profile(self, workdir: Path) -> Path:
        profile = workdir / "profile"
        (profile / "airootfs" / "etc" / "greetd").mkdir(parents=True)
        (profile / "airootfs" / "etc" / "systemd" / "system" / "getty@tty1.service.d").mkdir(parents=True)
        (profile / "airootfs" / "root").mkdir(parents=True)
        (profile / "packages.x86_64").write_text("base\ngreetd\nhyprland\n", encoding="utf-8")
        (profile / "profiledef.sh").write_text('iso_name="influent-danenone"\niso_application="Influent Danenone"\niso_version="0.4.2"\n', encoding="utf-8")
        (profile / "airootfs" / "etc" / "greetd" / "config.toml").write_text("[default_session]\n", encoding="utf-8")
        (profile / "airootfs" / "etc" / "systemd" / "system" / "getty@tty1.service.d" / "autologin.conf").write_text("[Service]\n", encoding="utf-8")
        (profile / "airootfs" / "root" / "customize_airootfs.sh").write_text("#!/usr/bin/env bash\nsystemctl enable greetd.service || true\n", encoding="utf-8")
        return profile

    def test_configures_plasma_default_and_keeps_hyprland_advanced(self):
        with tempfile.TemporaryDirectory() as directory:
            profile = self.create_profile(Path(directory))
            subprocess.run([str(CONFIGURATOR), str(profile)], check=True)

            packages = (profile / "packages.x86_64").read_text(encoding="utf-8")
            profiledef = (profile / "profiledef.sh").read_text(encoding="utf-8")
            self.assertNotIn("greetd\n", packages)
            self.assertIn("hyprland\n", packages)
            self.assertFalse((profile / "airootfs" / "etc" / "greetd" / "config.toml").exists())
            self.assertFalse((profile / "airootfs" / "etc" / "systemd" / "system" / "getty@tty1.service.d" / "autologin.conf").exists())

            sddm = (profile / "airootfs" / "etc" / "sddm.conf.d" / "10-influent-danenone.conf").read_text(encoding="utf-8")
            state = (profile / "airootfs" / "var" / "lib" / "sddm" / "state.conf").read_text(encoding="utf-8")
            policy = (profile / "airootfs" / "etc" / "influent-danenone" / "session-policy.conf").read_text(encoding="utf-8")
            motd = (profile / "airootfs" / "etc" / "motd").read_text(encoding="utf-8")
            customize = (profile / "airootfs" / "root" / "customize_airootfs.sh").read_text(encoding="utf-8")

            self.assertNotIn("[Autologin]", sddm)
            self.assertIn("RememberLastSession=false", sddm)
            self.assertIn("RememberLastUser=false", sddm)
            self.assertIn("Session=plasma.desktop", state)
            self.assertIn("DEFAULT_SESSION=plasma.desktop", policy)
            self.assertIn("ADVANCED_SESSION=hyprland.desktop", policy)
            self.assertIn("KDE Plasma / KWin", motd)
            self.assertIn("Hyprland", motd)
            self.assertNotIn("greetd.service", customize)
            self.assertIn("systemctl enable sddm.service", customize)
            self.assertIn('iso_name="influent-danenone-plasma"', profiledef)
            self.assertIn('iso_version="0.5.0-rc1"', profiledef)

    def test_rejects_profile_without_archiso_structure(self):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([str(CONFIGURATOR), directory], capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)
            self.assertIn("Uso:", result.stderr)


if __name__ == "__main__":
    unittest.main()
