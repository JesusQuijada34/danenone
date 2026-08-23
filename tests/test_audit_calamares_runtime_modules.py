import importlib.util
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "audit_calamares_runtime_modules.py"
SEQUENCE = ROOT / "packaging" / "calamares-config" / "reference-modules" / "execution-sequence.yaml"

SPEC = importlib.util.spec_from_file_location("audit_calamares_runtime_modules", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class AuditCalamaresRuntimeModulesTests(unittest.TestCase):
    def test_reference_sequence_requires_expected_modules(self):
        expected = {
            "welcome",
            "locale",
            "keyboard",
            "partition",
            "users",
            "summary",
            "mount",
            "unpackfs",
            "fstab",
            "displaymanager",
            "bootloader",
            "umount",
            "finished",
        }
        self.assertEqual(MODULE.reference_modules(SEQUENCE), expected)

    def test_audit_accepts_all_required_modules(self):
        reference = MODULE.reference_modules(SEQUENCE)
        missing, forbidden = MODULE.audit(reference, reference | {"shellprocess"})
        self.assertEqual(missing, set())
        self.assertEqual(forbidden, set())

    def test_audit_rejects_missing_or_forbidden_modules(self):
        missing, forbidden = MODULE.audit({"mount", "shellprocess"}, {"mount"})
        self.assertEqual(missing, {"shellprocess"})
        self.assertEqual(forbidden, {"shellprocess"})

    def test_package_listing_extracts_module_directories(self):
        listing = "\n".join(
            [
                "pkg /usr/lib/calamares/modules/mount/main.py",
                "pkg /usr/lib/calamares/modules/users/main.py",
                "pkg /usr/bin/calamares",
            ]
        )
        self.assertEqual(MODULE.package_modules(listing), {"mount", "users"})


if __name__ == "__main__":
    unittest.main()
