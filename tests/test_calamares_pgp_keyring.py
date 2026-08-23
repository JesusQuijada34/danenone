import hashlib
import pathlib
import shutil
import subprocess
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
PACKAGE = ROOT / "packaging" / "calamares"
KEY = PACKAGE / "keys" / "adriaan-de-groot-00ACD15E25A79FEE028B0EE57FEA3DA6169C77D6.asc"
VERIFY_SCRIPT = PACKAGE / "scripts" / "verify-source.sh"
PRIMARY_FINGERPRINT = "00ACD15E25A79FEE028B0EE57FEA3DA6169C77D6"
SIGNING_FINGERPRINT = "6D0837841C068A233F24127B14B6CC381BC256D6"
KEY_SHA256 = "9c651a9caee04c1315ce3bea2cb1ec5d8aa640b7a1ff9dc25acaeeb9798e11ca"


class CalamaresPgpKeyringTests(unittest.TestCase):
    def test_packaged_key_has_expected_checksum(self):
        self.assertTrue(KEY.is_file())
        self.assertEqual(hashlib.sha256(KEY.read_bytes()).hexdigest(), KEY_SHA256)

    def test_packaged_key_contains_verified_primary_and_signing_fingerprints(self):
        with tempfile.TemporaryDirectory() as home:
            subprocess.run(
                ["gpg", "--batch", "--homedir", home, "--import", str(KEY)],
                check=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            listing = subprocess.run(
                ["gpg", "--batch", "--homedir", home, "--with-colons", "--list-keys"],
                check=True,
                capture_output=True,
                text=True,
            ).stdout

        fingerprints = {
            line.split(":")[9]
            for line in listing.splitlines()
            if line.startswith("fpr:")
        }
        self.assertIn(PRIMARY_FINGERPRINT, fingerprints)
        self.assertIn(SIGNING_FINGERPRINT, fingerprints)

    def test_verify_script_uses_temporary_keyring_and_makepkg_verifysource(self):
        script = VERIFY_SCRIPT.read_text(encoding="utf-8")
        self.assertIn("GNUPGHOME=$(mktemp -d)", script)
        self.assertIn("gpg --batch", script)
        self.assertIn("makepkg --verifysource", script)
        self.assertNotIn("exec makepkg", script)
        self.assertNotIn("--skippgpcheck", script)
        self.assertEqual(shutil.which("sh"), "/usr/bin/sh")


if __name__ == "__main__":
    unittest.main()
