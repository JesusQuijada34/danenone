import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class PlasmaLabPackageTests(unittest.TestCase):
    @staticmethod
    def package_names(path: pathlib.Path) -> set[str]:
        return {
            line.strip()
            for line in path.read_text(encoding="utf-8").splitlines()
            if line.strip() and not line.lstrip().startswith("#")
        }

    def test_lab_package_set_contains_only_expected_calamares_packages(self):
        packages = (ROOT / "packages" / "editions" / "plasma-lab.packages").read_text(encoding="utf-8")
        self.assertIn("danenone-calamares\n", packages)
        self.assertIn("danenone-calamares-config\n", packages)
        self.assertIn("plasma-meta", packages)
        self.assertIn("hyprland", packages)

    def test_lab_keeps_rc1_applications_and_adds_only_calamares_packages(self):
        rc1 = self.package_names(ROOT / "packages" / "editions" / "plasma.packages")
        lab = self.package_names(ROOT / "packages" / "editions" / "plasma-lab.packages")
        self.assertTrue(rc1.issubset(lab))
        self.assertEqual(lab - rc1, {"danenone-calamares", "danenone-calamares-config"})

        edition = (ROOT / "editions" / "plasma-lab.conf").read_text(encoding="utf-8")
        self.assertIn("APP_SET=home", edition)
        self.assertIn("foundstore", edition)

    def test_plasma_lab_requires_hashed_local_repository(self):
        build = (ROOT / "scripts" / "build_edition_profile.sh").read_text(encoding="utf-8")
        self.assertIn("DANENONE_CALAMARES_LAB_REPO", build)
        self.assertIn("danenone-lab.db.tar.zst", build)
        self.assertIn("SHA256SUMS", build)
        self.assertIn("sha256sum -c SHA256SUMS", build)
        self.assertIn("[danenone-lab]", build)
        self.assertIn("Server = file://", build)
        self.assertIn("configure_calamares_plasma_lab.sh", build)
        generator = (ROOT / "scripts" / "configure_calamares_plasma_lab.sh").read_text(encoding="utf-8")
        self.assertIn("DANENONE_CALAMARES_LAB_ENABLE", generator)
        self.assertIn("qcow2-only", generator)

    def test_repo_generator_requires_only_expected_artifacts(self):
        helper = (ROOT / "scripts" / "prepare_plasma_lab_repo.sh").read_text(encoding="utf-8")
        self.assertIn("danenone-calamares-[0-9]*.pkg.tar.zst", helper)
        self.assertIn("danenone-calamares-config-*.pkg.tar.zst", helper)
        self.assertIn("repo-add -R", helper)
        self.assertIn("SHA256SUMS", helper)
        self.assertIn("sha256sum danenone-calamares-[0-9]*.pkg.tar.zst", helper)
        self.assertNotIn("--skippgpcheck", helper)

    def test_rc1_plasma_package_set_stays_unmodified(self):
        rc1 = (ROOT / "packages" / "editions" / "plasma.packages").read_text(encoding="utf-8")
        self.assertNotIn("danenone-calamares", rc1)


if __name__ == "__main__":
    unittest.main()
