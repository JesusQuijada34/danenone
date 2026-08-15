from __future__ import annotations

import asyncio
import threading
from dataclasses import dataclass
from typing import Callable

from .notifications import NotificationStore


@dataclass(frozen=True)
class DesktopNotification:
    app_name: str
    summary: str
    body: str
    replaces_id: int = 0
    expire_timeout: int = 5000


class FreedesktopNotificationBridge:
    """Puente opcional para org.freedesktop.Notifications.

    No genera notificaciones sintéticas. Si no existe dbus-next o un bus de sesión,
    el puente queda inactivo y el shell conserva únicamente el historial local.
    """

    def __init__(self, store: NotificationStore | None = None, on_notification: Callable[[DesktopNotification], None] | None = None):
        self.store = store or NotificationStore()
        self.on_notification = on_notification
        self._thread: threading.Thread | None = None
        self._stop = threading.Event()

    def start(self) -> bool:
        try:
            import dbus_next  # type: ignore  # optional dependency
        except ImportError:
            return False
        self._thread = threading.Thread(target=self._run, name="influent-dbus-notifications", daemon=True)
        self._thread.start()
        return True

    def stop(self) -> None:
        self._stop.set()

    def _run(self) -> None:
        asyncio.run(self._listen())

    async def _listen(self) -> None:
        from dbus_next.aio import MessageBus  # type: ignore
        from dbus_next import BusType, Message, MessageType  # type: ignore

        try:
            bus = await MessageBus(bus_type=BusType.SESSION).connect()
        except Exception:
            return
        match = "type='signal',interface='org.freedesktop.Notifications',member='Notify'"
        try:
            await bus.add_match(match)
        except Exception:
            return

        def handler(message: Message):
            if message.message_type != MessageType.SIGNAL or len(message.body) < 8:
                return
            app_name, replaces_id, _app_icon, summary, body = message.body[:5]
            item = DesktopNotification(str(app_name), str(summary), str(body), int(replaces_id), 5000)
            self.store.push(item.summary, item.body, item.app_name, duration=max(1.0, item.expire_timeout / 1000))
            if self.on_notification:
                self.on_notification(item)

        bus.add_message_handler(handler)
        while not self._stop.is_set():
            await asyncio.sleep(0.25)
        bus.disconnect()
