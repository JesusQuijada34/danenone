from __future__ import annotations

import getpass
import os
import pwd
import subprocess
from dataclasses import dataclass

from PyQt5.QtCore import Qt, QTimer, pyqtSignal
from PyQt5.QtWidgets import QLabel, QLineEdit, QVBoxLayout, QWidget

from influent_ui import InfluentButton, InfluentCard, InfluentInput


@dataclass(frozen=True)
class LinuxUser:
    username: str
    display_name: str
    home: str
    uid: int


def local_users() -> list[LinuxUser]:
    users = []
    for entry in pwd.getpwall():
        if entry.pw_uid >= 1000 and entry.pw_shell not in ("/usr/sbin/nologin", "/bin/false"):
            users.append(LinuxUser(entry.pw_name, entry.pw_gecos.split(",", 1)[0] or entry.pw_name, entry.pw_dir, entry.pw_uid))
    return users


class LoginManager(QWidget):
    authenticated = pyqtSignal(str)
    rejected = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("LoginManager")
        self.users = local_users()
        layout = QVBoxLayout(self)
        layout.setAlignment(Qt.AlignCenter)
        self.card = InfluentCard(self)
        card_layout = QVBoxLayout(self.card)
        card_layout.setContentsMargins(32, 32, 32, 32)
        title = QLabel("Welcome back")
        title.setObjectName("LoginTitle")
        subtitle = QLabel("Inicia sesión para continuar con Influent Danenone")
        subtitle.setObjectName("Muted")
        self.username = InfluentInput("Nombre de usuario")
        self.password = InfluentInput("Contraseña")
        self.password.setEchoMode(QLineEdit.Password)
        self.submit = InfluentButton("Iniciar sesión")
        self.submit.clicked.connect(self.authenticate)
        self.status = QLabel()
        self.status.setObjectName("Muted")
        card_layout.addWidget(title)
        card_layout.addWidget(subtitle)
        card_layout.addSpacing(18)
        card_layout.addWidget(self.username)
        card_layout.addWidget(self.password)
        card_layout.addWidget(self.submit)
        card_layout.addWidget(self.status)
        layout.addWidget(self.card)
        if self.users:
            self.username.setText(self.users[0].username)

    def authenticate(self):
        username = self.username.text().strip()
        if not any(user.username == username for user in self.users):
            self.status.setText("Usuario local no encontrado")
            self.rejected.emit("Usuario local no encontrado")
            return
        self.submit.setEnabled(False)
        self.status.setText("Validando credenciales con el sistema")
        QTimer.singleShot(100, lambda: self._authenticate_system(username))

    def _authenticate_system(self, username: str):
        if username == getpass.getuser() and not self.password.text():
            self.submit.setEnabled(True)
            self.status.setText("Escribe la contraseña del usuario")
            self.rejected.emit("Contraseña vacía")
            return
        # El login gráfico debe delegar en PAM/DM; nunca se comparan contraseñas en Python.
        self.submit.setEnabled(True)
        self.status.setText("La validación PAM debe realizarla el gestor de sesión")
        self.rejected.emit("PAM no está conectado en esta sesión de prueba")
