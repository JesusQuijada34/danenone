import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PACKAGE = ROOT / "packaging" / "calamares" / "PKGBUILD"


class CalamaresPackagingTests(unittest.TestCase):
    def test_pins_official_release_and_integrity(self):
        package = PACKAGE.read_text(encoding="utf-8")
        self.assertIn("pkgver=3.4.2", package)
        self.assertIn("733bbbb00dc9f84874bd5c22960952f317ea2537565431179fa2152b2fbfdccc", package)
        self.assertIn("6D0837841C068A233F24127B14B6CC381BC256D6", package)
        self.assertIn("calamares-${pkgver}.tar.gz.asc", package)

    def test_uses_qt6_qml_systemd_and_external_configuration(self):
        package = PACKAGE.read_text(encoding="utf-8")
        self.assertIn("-DWITH_QT6=ON", package)
        self.assertIn("-DWITH_QML=ON", package)
        self.assertIn("-DUSE_services=systemd", package)
        self.assertIn("-DINSTALL_CONFIG=OFF", package)
        self.assertIn("'qt6-tools'", package)
        self.assertNotRegex(package, re.compile(r"packages/editions/plasma\.packages"))


if __name__ == "__main__":
    unittest.main()
