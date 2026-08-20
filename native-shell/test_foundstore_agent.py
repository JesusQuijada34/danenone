#!/usr/bin/env python3
import hashlib
import hmac
import importlib.util
import json
import unittest
from datetime import datetime, timedelta, timezone
from pathlib import Path


MODULE_PATH = Path(__file__).with_name("foundstore-agent.py")
SPEC = importlib.util.spec_from_file_location("foundstore_agent", MODULE_PATH)
assert SPEC and SPEC.loader
agent = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(agent)


class FoundstoreAgentCommandTests(unittest.TestCase):
    def command(self, expires_at: datetime) -> tuple[dict, dict]:
        config = {"deviceId": "device-abcdefghijkl", "commandKey": "command-key"}
        command = {
            "id": "request-abcdefghijkl",
            "deviceId": config["deviceId"],
            "type": "install_request",
            "payload": {"publisher": "Influent", "packageSlug": "packagemaker", "localApprovalRequired": True},
            "expiresAt": expires_at.isoformat(),
        }
        canonical = json.dumps(command, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
        command["signature"] = hmac.new(config["commandKey"].encode("utf-8"), canonical, hashlib.sha256).hexdigest()
        return command, config

    def test_accepts_valid_unexpired_signed_command(self):
        command, config = self.command(datetime.now(timezone.utc) + timedelta(minutes=1))
        self.assertTrue(agent._valid_command(command, config))

    def test_rejects_tampered_or_expired_command(self):
        command, config = self.command(datetime.now(timezone.utc) + timedelta(minutes=1))
        command["payload"]["publisher"] = "Other"
        self.assertFalse(agent._valid_command(command, config))
        expired, config = self.command(datetime.now(timezone.utc) - timedelta(seconds=1))
        self.assertFalse(agent._valid_command(expired, config))


if __name__ == "__main__":
    unittest.main()
