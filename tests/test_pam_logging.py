from __future__ import annotations

import logging
from pathlib import Path


def test_logging_is_project_local_and_idempotent(tmp_path, monkeypatch):
    monkeypatch.setenv('DANENONE_LOG_DIR', str(tmp_path / 'logs'))
    import src.core.logging_setup as logging_setup
    root = logging.getLogger()
    before = len([h for h in root.handlers if getattr(h, '_danenone_marker', None)])
    first = logging_setup.setup_logging('test-logging')
    second = logging_setup.setup_logging('test-logging')
    after = len([h for h in root.handlers if getattr(h, '_danenone_marker', None)])
    assert first.name == 'test-logging'
    assert second.name == 'test-logging'
    assert after == before + 1
    first.info('mensaje de prueba')
    for handler in root.handlers:
        handler.flush()
    log_file = tmp_path / 'logs' / 'test-logging.log'
    assert log_file.exists()
    assert 'mensaje de prueba' in log_file.read_text(encoding='utf-8')


def test_empty_credentials_are_rejected(monkeypatch):
    import src.core.pam_auth as pam_auth
    assert pam_auth.authenticate('', '') is False
    assert pam_auth.authenticate('user', '') is False
    assert pam_auth.authenticate('', 'secret') is False


def test_missing_pam_does_not_use_shell_fallback(monkeypatch):
    import src.core.pam_auth as pam_auth
    monkeypatch.setattr(pam_auth, '_pam', None)
    assert pam_auth.pam_available() is False
    assert pam_auth.authenticate('nonexistent-user', 'not-a-real-password') is False
