import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
BUILD_SCRIPT = ROOT / "scripts" / "build_edition_profile.sh"
PLASMA_SCRIPT = ROOT / "scripts" / "configure_plasma_edition.sh"
CALAMARES_LAB_SCRIPT = ROOT / "scripts" / "configure_calamares_plasma_lab.sh"


class PlasmaLabTests(unittest.TestCase):
    def test_only_plasma_lab_profile_exists_for_current_lab_scope(self):
        profile = (ROOT / "editions" / "plasma-lab.conf").read_text(encoding="utf-8")
        self.assertIn("EDITION_ID=plasma-lab", profile)
        self.assertIn("PACKAGE_SET=plasma", profile)
        self.assertIn("Plasma/KWin es el único entorno predeterminado", profile)
        self.assertIn("Hyprland permanece como sesión avanzada", profile)
        self.assertFalse((ROOT / "editions" / "native-lab.conf").exists())
        self.assertFalse((ROOT / "packages" / "editions" / "native-lab.packages").exists())

    def test_lab_build_assigns_only_plasma_identity(self):
        build = BUILD_SCRIPT.read_text(encoding="utf-8")
        plasma = PLASMA_SCRIPT.read_text(encoding="utf-8")
        self.assertIn('"plasma-lab"', build)
        self.assertNotIn('"native-lab"', build)
        self.assertIn('influent-danenone-plasma-lab', plasma)
        self.assertNotIn("native-lab", plasma)

    def test_current_profiles_do_not_contain_dual_grub_selector(self):
        design = (ROOT / "docs" / "calamares-plasma-lab-design.md").read_text(encoding="utf-8")
        self.assertIn("GRUB inicia un entorno live", design)
        self.assertIn("discos `qcow2`", design)
        self.assertFalse((ROOT / "packaging" / "calamares-config" / "lab-grub").exists())
        self.assertFalse((ROOT / "archiso-profile" / "grub" / "edition-selector.cfg").exists())

    def test_plasma_rc1_stays_free_of_calamares_configuration(self):
        packages = (ROOT / "packages" / "editions" / "plasma.packages").read_text(encoding="utf-8")
        self.assertNotIn("calamares", packages.lower())
        self.assertFalse((ROOT / "archiso-profile" / "airootfs" / "etc" / "calamares").exists())

    def test_active_calamares_generator_is_restricted_to_plasma_lab(self):
        generator = CALAMARES_LAB_SCRIPT.read_text(encoding="utf-8")
        self.assertIn('"$EDITION" != "plasma-lab"', generator)
        self.assertIn("DANENONE_CALAMARES_LAB_ENABLE", generator)
        self.assertIn("qcow2-only", generator)
        self.assertIn('"$PROFILE/airootfs/etc/calamares"', generator)
        self.assertIn("archiso/bootmnt/influent/x86_64/airootfs.sfs", generator)

    def test_sddm_locks_plasma_default_without_hiding_hyprland(self):
        plasma = PLASMA_SCRIPT.read_text(encoding="utf-8")
        self.assertIn("[Autologin]", plasma)
        self.assertIn("Session=plasma.desktop", plasma)
        self.assertIn('SDDM_DEFAULT_SESSION="/usr/share/wayland-sessions/${DEFAULT_SESSION}"', plasma)
        self.assertIn("Session=${SDDM_DEFAULT_SESSION}", plasma)
        self.assertIn("RememberLastSession=false", plasma)
        self.assertIn("chown sddm:sddm /var/lib/sddm/state.conf", plasma)
        self.assertIn("ADVANCED_SESSION=hyprland.desktop", plasma)


if __name__ == "__main__":
    unittest.main()
