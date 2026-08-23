import importlib.util
import tempfile
import unittest
import zipfile
from pathlib import Path


MODULE_PATH = Path(__file__).resolve().parents[1] / "scripts" / "integrate_foundstore_release.py"
SPEC = importlib.util.spec_from_file_location("integrate_foundstore_release", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class FoundstoreReleaseIntegrationTests(unittest.TestCase):
    def _artifact(self, root: Path, unsafe: bool = False) -> Path:
        artifact = root / "Influent.foundstore.v1.2-26.08-22.20-Danenone.iflapp"
        with zipfile.ZipFile(artifact, "w", zipfile.ZIP_DEFLATED) as archive:
            archive.writestr("details.xml", "<app><publisher>Influent</publisher><app>foundstore</app><version>v1.2-26.08-22.20</version><platform>Danenone</platform></app>")
            archive.writestr("bin/foundstore", b"binary")
            archive.writestr("app/app-icon.png", b"icon")
            if unsafe:
                archive.writestr("../escape", b"blocked")
        return artifact

    def _profile(self, root: Path) -> Path:
        profile = root / "profile"
        (profile / "airootfs").mkdir(parents=True)
        (profile / "packages.x86_64").write_text("base\n", encoding="utf-8")
        return profile

    def test_valid_foundstore_artifact_creates_launcher_desktop_entry_and_catalog_copy(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            profile = self._profile(root)
            artifact = self._artifact(root)
            package_dir = MODULE.integrate(artifact, profile)

            self.assertTrue((package_dir / "bin" / "foundstore").is_file())
            self.assertTrue((profile / "airootfs/usr/share/influent/packages" / artifact.name).is_file())
            self.assertIn("Exec=foundstore", (profile / "airootfs/usr/share/applications/influent-foundstore.desktop").read_text(encoding="utf-8"))
            self.assertTrue((profile / "airootfs/usr/local/bin/foundstore").is_file())

    def test_rejects_unsafe_archive_members(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with self.assertRaisesRegex(ValueError, "insegura"):
                MODULE.integrate(self._artifact(root, unsafe=True), self._profile(root))


if __name__ == "__main__":
    unittest.main()
