import json
from pathlib import Path

import pytest

from danenone_shell.iso_builder import write_manifest
from foundstore_client import FoundStoreClient
from installer import InstallPlan, InstallerValidationError, plan_install


def test_installer_defaults_to_simulation():
    result = plan_install(InstallPlan('/dev/sda', 'danenone', 'danenone-pc', 'America/New_York'))
    assert result['dry_run'] is True
    assert result['status'] == 'simulation-only'


def test_installer_rejects_root_device():
    with pytest.raises(InstallerValidationError):
        plan_install(InstallPlan('/', 'danenone', 'pc', 'America/New_York'))


def test_manifest_is_json(tmp_path: Path):
    path = write_manifest(tmp_path)
    manifest = json.loads(path.read_text())
    assert manifest['distribution'] == 'Danenone'
    assert 'required_tools' in manifest


def test_catalog_loader(tmp_path: Path):
    path = tmp_path / 'catalog.json'
    path.write_text('{"repositories": ["JesusQuijada34/foundstore"]}')
    assert FoundStoreClient.load_catalog(str(path)) == ['JesusQuijada34/foundstore']
