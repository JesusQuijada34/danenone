import hashlib
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
VALIDATOR = ROOT / "scripts" / "validate_calamares_lab.py"


class ValidateCalamaresLabTests(unittest.TestCase):
    def make_lab(self, directory: Path) -> tuple[Path, Path, str]:
        iso = directory / "influent-danenone-calamares-lab.iso"
        disk = directory / "target.qcow2"
        iso.write_bytes(b"isolated calamares lab iso")
        disk.write_bytes(b"qcow2 placeholder")
        return iso, disk, hashlib.sha256(iso.read_bytes()).hexdigest()

    def run_validator(self, workspace: Path, iso: Path, disk: Path, checksum: str):
        return subprocess.run(
            [
                sys.executable,
                str(VALIDATOR),
                "--workspace",
                str(workspace),
                "--iso",
                str(iso),
                "--disk",
                str(disk),
                "--expect-iso-sha256",
                checksum,
            ],
            capture_output=True,
            text=True,
            check=False,
        )

    def test_accepts_new_iso_and_qcow2_inside_workspace(self):
        with tempfile.TemporaryDirectory() as temporary:
            workspace = Path(temporary)
            iso, disk, checksum = self.make_lab(workspace)
            result = self.run_validator(workspace, iso, disk, checksum)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("VALIDADO", result.stdout)
        self.assertIn("qcow2=", result.stdout)

    def test_rejects_rc1_name_even_with_matching_hash(self):
        with tempfile.TemporaryDirectory() as temporary:
            workspace = Path(temporary)
            iso, disk, checksum = self.make_lab(workspace)
            rc1_name = workspace / "influent-danenone-plasma-0.5.0-rc1-x86_64.iso"
            iso.rename(rc1_name)
            result = self.run_validator(workspace, rc1_name, disk, checksum)
        self.assertEqual(result.returncode, 2)
        self.assertIn("RC1", result.stderr)

    def test_rejects_disk_outside_workspace(self):
        with tempfile.TemporaryDirectory() as temporary, tempfile.TemporaryDirectory() as external:
            workspace = Path(temporary)
            iso, _, checksum = self.make_lab(workspace)
            disk = Path(external) / "outside.qcow2"
            disk.write_bytes(b"outside")
            result = self.run_validator(workspace, iso, disk, checksum)
        self.assertEqual(result.returncode, 2)
        self.assertIn("dentro del directorio", result.stderr)

    def test_rejects_physical_device_path(self):
        with tempfile.TemporaryDirectory() as temporary:
            workspace = Path(temporary)
            iso, _, checksum = self.make_lab(workspace)
            result = self.run_validator(workspace, iso, Path("/dev/null"), checksum)
        self.assertEqual(result.returncode, 2)
        self.assertIn("/dev", result.stderr)

    def test_rejects_symlink_that_escapes_workspace(self):
        with tempfile.TemporaryDirectory() as temporary, tempfile.TemporaryDirectory() as external:
            workspace = Path(temporary)
            iso, _, checksum = self.make_lab(workspace)
            external_disk = Path(external) / "outside.qcow2"
            external_disk.write_bytes(b"outside")
            linked_disk = workspace / "linked.qcow2"
            linked_disk.symlink_to(external_disk)
            result = self.run_validator(workspace, iso, linked_disk, checksum)
        self.assertEqual(result.returncode, 2)
        self.assertIn("dentro del directorio", result.stderr)

    def test_rejects_non_qcow2_disk(self):
        with tempfile.TemporaryDirectory() as temporary:
            workspace = Path(temporary)
            iso, disk, checksum = self.make_lab(workspace)
            wrong_suffix = disk.with_suffix(".img")
            disk.rename(wrong_suffix)
            result = self.run_validator(workspace, iso, wrong_suffix, checksum)
        self.assertEqual(result.returncode, 2)
        self.assertIn(".qcow2", result.stderr)

    def test_rejects_mismatched_iso_checksum(self):
        with tempfile.TemporaryDirectory() as temporary:
            workspace = Path(temporary)
            iso, disk, _ = self.make_lab(workspace)
            result = self.run_validator(workspace, iso, disk, "0" * 64)
        self.assertEqual(result.returncode, 2)
        self.assertIn("SHA-256", result.stderr)


if __name__ == "__main__":
    unittest.main()
