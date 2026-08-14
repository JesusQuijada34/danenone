from __future__ import annotations

import json
import zipfile
from pathlib import Path

from danenone_shell.notifications import NotificationStore
from danenone_shell.updater import parse_details_xml, validate_iflapp

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / "build" / "system-fluthin-artifacts"


def main() -> None:
    packages = sorted(ARTIFACTS.glob("*.iflapp"))
    assert len(packages) == 3, packages
    for package in packages:
        metadata = validate_iflapp(package)
        assert metadata.publisher == "Influent"
        assert metadata.platform == "Danenone"
        with zipfile.ZipFile(package) as archive:
            assert "details.xml" in archive.namelist()
            assert any(name.startswith("app/") and name.endswith("/app") for name in archive.namelist())
    details = ROOT / "build" / "test-details.xml"
    details.write_text("<app><publisher>Influent</publisher><app>sample-app</app><name>Sample</name><version>1.0-26.08-21.56</version><author>JesusQuijada34</author><platform>Danenone</platform></app>", encoding="utf-8")
    assert parse_details_xml(details).publisher == "Influent"
    store = NotificationStore(ROOT / "build" / "test-notifications.json")
    store.clear_all()
    store.push("Actualización disponible", "Hay una versión verificada.", "Influent Updater", duration=4)
    assert store.unread_count() == 1
    store.mark_all_read()
    assert store.unread_count() == 0
    store.clear_all()
    print(json.dumps({"packages": [p.name for p in packages], "hardware_and_notification_tests": "passed"}, indent=2))


if __name__ == "__main__":
    main()
