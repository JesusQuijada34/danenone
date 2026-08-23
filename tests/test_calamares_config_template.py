import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
CONFIG = ROOT / "packaging" / "calamares-config"
PLASMA_PACKAGES = ROOT / "packages" / "editions" / "plasma.packages"
VM_PROTOCOL = ROOT / "docs" / "calamares-disposable-vm-validation.md"
EXEC_RESEARCH = ROOT / "docs" / "calamares-exec-module-research.md"


class CalamaresConfigTemplateTests(unittest.TestCase):
    def test_template_files_are_present(self):
        expected = {
            "PKGBUILD",
            "settings.conf",
            "branding.desc",
            "show.qml",
            "stylesheet.qss",
            "influent-stream-wallpaper.png",
            "influent-danenone-boot.png",
            "README.md",
        }
        self.assertTrue(expected.issubset({path.name for path in CONFIG.iterdir()}))

    def test_settings_are_preview_only_and_require_confirmation(self):
        settings = (CONFIG / "settings.conf").read_text(encoding="utf-8")
        self.assertIn("- exec: []", settings)
        self.assertIn("prompt-install: true", settings)
        self.assertNotIn("shellprocess", settings)
        self.assertNotIn("contextualprocess", settings)
        self.assertNotIn("webview", settings)

    def test_package_does_not_activate_etc_calamares(self):
        package = (CONFIG / "PKGBUILD").read_text(encoding="utf-8")
        self.assertIn("/usr/share/danenone/calamares-template/etc/calamares", package)
        self.assertNotIn('"$pkgdir/etc/calamares', package)

    def test_branding_is_local_and_original(self):
        branding = (CONFIG / "branding.desc").read_text(encoding="utf-8")
        slideshow = (CONFIG / "show.qml").read_text(encoding="utf-8")
        self.assertIn("Influent Danenone", branding)
        self.assertIn("type: none", branding)
        self.assertIn("INFLUENT DANENONE", slideshow)
        self.assertIn("pragma ComponentBehavior: Bound", slideshow)
        self.assertIn("required property int index", slideshow)
        self.assertNotIn("Apple", branding + slideshow)

    def test_handoff_contract_excludes_secrets_and_disk_choices(self):
        readme = (CONFIG / "README.md").read_text(encoding="utf-8")
        self.assertIn("SELECTED_LANGUAGE", readme)
        self.assertIn("SELECTED_EDITION", readme)
        self.assertIn("No debe aceptar", readme)
        self.assertIn("contraseñas", readme)
        self.assertIn("identificadores de disco", readme)

    def test_plasma_profile_does_not_activate_calamares(self):
        packages = PLASMA_PACKAGES.read_text(encoding="utf-8")
        self.assertNotIn("calamares", packages.lower())

    def test_vm_protocol_requires_disposable_qcow2_and_no_host_disks(self):
        protocol = VM_PROTOCOL.read_text(encoding="utf-8")
        self.assertIn("`qcow2`", protocol)
        self.assertIn("/dev/sd*", protocol)
        self.assertIn("/dev/nvme*", protocol)
        self.assertIn("distinta de `v0.5.0-plasma-rc1`", protocol)
        self.assertIn("sin `--skippgpcheck`", protocol)
        self.assertIn("`exec: []`", protocol)
        self.assertIn("`QT_QPA_PLATFORM=offscreen`", protocol)

    def test_execution_contract_is_documentary_until_vm_preconditions_exist(self):
        research = EXEC_RESEARCH.read_text(encoding="utf-8")
        self.assertIn("`partition`", research)
        self.assertIn("`unpackfs`", research)
        self.assertIn("`displaymanager`", research)
        self.assertIn("`bootloader`", research)
        self.assertIn("`shellprocess`", research)
        self.assertIn("no debe añadirse a `unpackfs.conf`", research)
        self.assertFalse((CONFIG / "modules").exists())


if __name__ == "__main__":
    unittest.main()
