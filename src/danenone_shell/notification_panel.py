from __future__ import annotations

from datetime import datetime

from PyQt5.QtCore import Qt, QTimer, pyqtSignal
from PyQt5.QtWidgets import QHBoxLayout, QLabel, QPushButton, QScrollArea, QVBoxLayout, QWidget

from .notifications import NotificationStore


class NotificationPanel(QWidget):
    clear_requested = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("NotificationPanel")
        self.setFixedWidth(430)
        self.store = NotificationStore()
        root = QVBoxLayout(self)
        root.setContentsMargins(20, 18, 20, 18)
        heading = QHBoxLayout()
        title = QLabel("Notificaciones")
        title.setObjectName("PanelHeading")
        heading.addWidget(title)
        heading.addStretch()
        clear = QPushButton("Limpiar todo")
        clear.clicked.connect(self.clear_all)
        heading.addWidget(clear)
        root.addLayout(heading)
        self.scroll = QScrollArea()
        self.scroll.setWidgetResizable(True)
        self.content = QWidget()
        self.cards = QVBoxLayout(self.content)
        self.cards.setAlignment(Qt.AlignTop)
        self.scroll.setWidget(self.content)
        root.addWidget(self.scroll)
        self.refresh()

    def refresh(self):
        while self.cards.count():
            item = self.cards.takeAt(0)
            if item.widget():
                item.widget().deleteLater()
        notifications = list(reversed(self.store.load()))
        if not notifications:
            empty = QLabel("No hay notificaciones")
            empty.setObjectName("Muted")
            self.cards.addWidget(empty)
            return
        for item in notifications:
            card = QWidget()
            card.setObjectName("NotificationCard")
            layout = QVBoxLayout(card)
            layout.setContentsMargins(14, 12, 14, 12)
            top = QHBoxLayout()
            name = QLabel(item.source)
            name.setObjectName("NotificationSource")
            when = QLabel(datetime.fromtimestamp(item.created_at).strftime("%H:%M"))
            when.setObjectName("Muted")
            top.addWidget(name)
            top.addStretch()
            top.addWidget(when)
            layout.addLayout(top)
            title = QLabel(item.title)
            title.setObjectName("NotificationTitle")
            layout.addWidget(title)
            body = QLabel(item.body)
            body.setWordWrap(True)
            body.setObjectName("Muted")
            layout.addWidget(body)
            self.cards.addWidget(card)

    def clear_all(self):
        self.store.clear_all()
        self.refresh()
        self.clear_requested.emit()
