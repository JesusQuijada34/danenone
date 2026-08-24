import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONFIGURATOR = ROOT / "scripts" / "configure_calamares_plasma_lab.sh"


class ConfigureCalamaresPlasmaLabTests(unittest.TestCase):
    def create_profile(self, workdir: Path) -> Path:
        profile = workdir / "profile"
        (profile / "airootfs").mkdir(parents=True)
        (profile / "profiledef.sh").write_text(
            'iso_name="influent-danenone-plasma-lab"\n', encoding="utf-8"
        )
        return profile

    def test_requires_lab_edition_and_explicit_qcow2_consent(self):
        with tempfile.TemporaryDirectory() as directory:
            profile = self.create_profile(Path(directory))

            denied = subprocess.run(
                [str(CONFIGURATOR), str(profile), "plasma-lab"],
                capture_output=True,
                text=True,
            )
            self.assertEqual(denied.returncode, 3)
            self.assertFalse((profile / "airootfs" / "etc" / "calamares").exists())

            env = os.environ | {"DANENONE_CALAMARES_LAB_ENABLE": "qcow2-only"}
            wrong_edition = subprocess.run(
                [str(CONFIGURATOR), str(profile), "plasma"],
                capture_output=True,
                text=True,
                env=env,
            )
            self.assertEqual(wrong_edition.returncode, 2)
            self.assertFalse((profile / "airootfs" / "etc" / "calamares").exists())

    def test_generates_confined_active_lab_configuration(self):
        with tempfile.TemporaryDirectory() as directory:
            profile = self.create_profile(Path(directory))
            env = os.environ | {"DANENONE_CALAMARES_LAB_ENABLE": "qcow2-only"}
            subprocess.run(
                [str(CONFIGURATOR), str(profile), "plasma-lab"],
                check=True,
                env=env,
            )

            calamares = profile / "airootfs" / "etc" / "calamares"
            settings = (calamares / "settings.conf").read_text(encoding="utf-8")
            unpackfs = (calamares / "modules" / "unpackfs.conf").read_text(encoding="utf-8")
            locale = (calamares / "modules" / "locale.conf").read_text(encoding="utf-8")
            users = (calamares / "modules" / "users.conf").read_text(encoding="utf-8")

            self.assertTrue(calamares.is_dir())
            self.assertIn("prompt-install: true", settings)
            self.assertIn("  - partition\n  - mount\n  - unpackfs", settings)
            self.assertIn("  - bootloader\n  - umount", settings)
            self.assertNotIn("shellprocess", settings)
            self.assertNotIn("contextualprocess", settings)
            self.assertNotIn("webview", settings)
            self.assertIn('/run/archiso/bootmnt/influent/x86_64/airootfs.sfs', unpackfs)
            self.assertIn('etc/polkit-1/rules.d/49-influent-live-calamares.rules', unpackfs)
            self.assertNotIn("rc1", unpackfs.lower())
            self.assertNotIn("geoip", locale.lower())
            self.assertIn("setRootPassword: false", users)
            self.assertTrue((calamares / "branding" / "influent-danenone" / "show.qml").is_file())
            self.assertFalse((profile / "etc" / "calamares").exists())

    def test_refuses_to_overwrite_existing_configuration(self):
        with tempfile.TemporaryDirectory() as directory:
            profile = self.create_profile(Path(directory))
            target = profile / "airootfs" / "etc" / "calamares"
            target.mkdir(parents=True)
            env = os.environ | {"DANENONE_CALAMARES_LAB_ENABLE": "qcow2-only"}
            result = subprocess.run(
                [str(CONFIGURATOR), str(profile), "plasma-lab"],
                capture_output=True,
                text=True,
                env=env,
            )
            self.assertEqual(result.returncode, 5)
            self.assertIn("no se sobrescribirá", result.stderr)


if __name__ == "__main__":
    unittest.main()
