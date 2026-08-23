import os
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HELPER = ROOT / "archiso-profile" / "airootfs" / "usr" / "local" / "bin" / "influent-oobe-export-installer-handoff"
MANIFEST = ROOT / "native-shell" / "languages" / "manifest.tsv"


class OobeInstallerHandoffTests(unittest.TestCase):
    def run_helper(self, selection: str):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            selection_file = root / "oobe-selection.conf"
            selection_file.write_text(selection, encoding="utf-8")
            env = {
                **os.environ,
                "OOBE_SELECTION_FILE": str(selection_file),
                "DANENONE_TARGET_ROOT": str(root / "target"),
                "DANENONE_LANGUAGE_MANIFEST": str(MANIFEST),
            }
            result = subprocess.run([str(HELPER)], env=env, capture_output=True, text=True)
            target = root / "target" / "etc" / "influent-danenone" / "installer-handoff.conf"
            exists = target.exists()
            contents = target.read_text(encoding="utf-8") if exists else None
            mode = stat.S_IMODE(target.stat().st_mode) if exists else None
            return result, exists, contents, mode

    def test_exports_only_validated_non_secret_decisions(self):
        result, exists, contents, mode = self.run_helper("LANGUAGE=es\nEDITION=developer\nNETWORK_SECRET=not-exported\nPASSWORD=not-exported\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(exists)
        self.assertEqual(contents, "SELECTED_LANGUAGE=es\nSELECTED_EDITION=developer\n")
        self.assertEqual(mode, 0o600)

    def test_rejects_unknown_edition(self):
        result, exists, _, _ = self.run_helper("LANGUAGE=en\nEDITION=erase-all\n")
        self.assertEqual(result.returncode, 2)
        self.assertFalse(exists)

    def test_rejects_language_missing_from_manifest(self):
        result, exists, _, _ = self.run_helper("LANGUAGE=not-a-language\nEDITION=home\n")
        self.assertEqual(result.returncode, 2)
        self.assertFalse(exists)


if __name__ == "__main__":
    unittest.main()
