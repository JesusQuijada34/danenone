from __future__ import annotations

import json
import os
import time
from dataclasses import asdict, dataclass
from pathlib import Path


NOTIFICATION_PATH = Path("/var/lib/influent/notifications.json")


@dataclass
class Notification:
    title: str
    body: str
    source: str
    created_at: float
    duration: float = 6.0
    priority: str = "normal"
    read: bool = False


class NotificationStore:
    def __init__(self, path: Path = NOTIFICATION_PATH):
        self.path = path
        try:
            self.path.parent.mkdir(parents=True, exist_ok=True)
            self.path.touch(exist_ok=True)
        except PermissionError:
            state_home = Path(os.environ.get("XDG_STATE_HOME", Path.home() / ".local" / "state"))
            self.path = state_home / "influent" / "notifications.json"
            self.path.parent.mkdir(parents=True, exist_ok=True)

    def load(self) -> list[Notification]:
        if not self.path.exists():
            return []
        try:
            data = json.loads(self.path.read_text(encoding="utf-8"))
            return [Notification(**item) for item in data if isinstance(item, dict)]
        except (OSError, ValueError, TypeError):
            return []

    def push(self, title: str, body: str, source: str, duration: float = 6.0, priority: str = "normal") -> Notification:
        item = Notification(title, body, source, time.time(), duration, priority)
        items = self.load()
        items.append(item)
        self.path.write_text(json.dumps([asdict(value) for value in items[-100:]], ensure_ascii=False, indent=2), encoding="utf-8")
        return item

    def mark_all_read(self) -> None:
        items = self.load()
        for item in items:
            item.read = True
        self.path.write_text(json.dumps([asdict(value) for value in items], ensure_ascii=False, indent=2), encoding="utf-8")

    def clear_all(self) -> None:
        self.path.write_text("[]\n", encoding="utf-8")

    def unread_count(self) -> int:
        return sum(not item.read for item in self.load())


if __name__ == "__main__":
    print(json.dumps([asdict(item) for item in NotificationStore().load()], ensure_ascii=False, indent=2))
