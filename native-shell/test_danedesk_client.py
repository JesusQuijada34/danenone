#!/usr/bin/env python3
import importlib.util
import json
import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

MODULE_PATH = Path(__file__).with_name("danedesk-client.py")
SPEC = importlib.util.spec_from_file_location("danedesk_client", MODULE_PATH)
assert SPEC and SPEC.loader
client = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(client)


class DaneDeskClientTests(unittest.TestCase):
    def test_rejects_non_https_server(self):
        with self.assertRaisesRegex(client.DaneDeskError, "HTTPS"):
            client.server_url("http://example.invalid")

    def test_hardware_hash_is_sha256_and_not_raw_machine_value(self):
        with patch("pathlib.Path.read_text", side_effect=["machine-value\n", "dmi-value\n"]):
            value = client.hardware_id_hash()
        self.assertRegex(value, r"^[a-f0-9]{64}$")
        self.assertNotIn("machine-value", value)
        self.assertNotIn("dmi-value", value)

    def test_recovery_uses_public_local_recovery_route_with_hardware_hash(self):
        with tempfile.TemporaryDirectory() as directory:
            config_path = Path(directory) / "device.json"
            lock_path = Path(directory) / "lock.json"
            original_config, original_lock = client.CONFIG_PATH, client.LOCK_STATE_PATH
            client.CONFIG_PATH, client.LOCK_STATE_PATH = config_path, lock_path
            try:
                client.save_json(config_path, {"server": "https://foundstore.example", "deviceId": "device"})
                with patch.object(client, "hardware_id_hash", return_value="a" * 64), patch.object(client, "trpc_call", return_value={"success": True}) as call:
                    result = client.recover(type("Arguments", (), {"otp": "123456"})())
                self.assertEqual(result, {"success": True})
                call.assert_called_once_with("https://foundstore.example", "danedesk.recoverLocal", {"hardwareIdHash": "a" * 64, "purpose": "unlock", "code": "123456"}, True)
                self.assertEqual(os.stat(config_path).st_mode & 0o777, 0o600)
            finally:
                client.CONFIG_PATH, client.LOCK_STATE_PATH = original_config, original_lock

    def test_enforce_lock_requests_visible_session_lock_and_records_reason(self):
        with tempfile.TemporaryDirectory() as directory:
            lock_path = Path(directory) / "lock.json"
            original_lock = client.LOCK_STATE_PATH
            client.LOCK_STATE_PATH = lock_path
            try:
                with patch.object(client, "check_status", return_value={"enrolled": True, "recoveryRequired": True, "status": "lost", "lockReason": "Equipo reportado perdido"}), patch.object(client, "lock_visible_session", return_value=True), patch.object(client, "notify_user") as notify:
                    result = client.enforce_lock(type("Arguments", (), {"server": None})())
                self.assertTrue(result["recoveryRequired"])
                self.assertTrue(client.load_config if False else json.loads(lock_path.read_text(encoding="utf-8"))["visibleLockRequested"])
                notify.assert_called_once_with("DaneDesk bloqueado", "Equipo reportado perdido")
            finally:
                client.LOCK_STATE_PATH = original_lock

    def test_enforce_lock_notifies_and_clears_a_revoked_link(self):
        with tempfile.TemporaryDirectory() as directory:
            config_path, lock_path = Path(directory) / "device.json", Path(directory) / "lock.json"
            original_config, original_lock = client.CONFIG_PATH, client.LOCK_STATE_PATH
            client.CONFIG_PATH, client.LOCK_STATE_PATH = config_path, lock_path
            try:
                client.save_json(config_path, {"server": "https://foundstore.example", "deviceId": "device"})
                client.save_json(lock_path, {"status": "lost"})
                with patch.object(client, "check_status", return_value={"enrolled": False, "recoveryRequired": False}), patch.object(client, "notify_user") as notify:
                    result = client.enforce_lock(type("Arguments", (), {"server": None})())
                self.assertTrue(result["unlinked"])
                self.assertFalse(config_path.exists())
                self.assertFalse(lock_path.exists())
                notify.assert_called_once()
            finally:
                client.CONFIG_PATH, client.LOCK_STATE_PATH = original_config, original_lock


if __name__ == "__main__":
    unittest.main()
