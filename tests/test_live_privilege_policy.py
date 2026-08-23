import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CUSTOMIZE = ROOT / "archiso-profile" / "airootfs" / "root" / "customize_airootfs.sh"


class LivePrivilegePolicyTests(unittest.TestCase):
    def test_live_customization_does_not_grant_unrestricted_passwordless_commands(self):
        script = CUSTOMIZE.read_text(encoding="utf-8")
        self.assertNotIn("/etc/sudoers.d/influent-firstboot", script)
        self.assertNotIn("NOPASSWD", script)
        self.assertNotIn("/usr/bin/chpasswd", script)
        self.assertNotIn("/usr/bin/sed", script)


if __name__ == "__main__":
    unittest.main()
