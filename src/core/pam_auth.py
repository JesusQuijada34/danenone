from __future__ import annotations

import logging
from typing import Any

from .logging_setup import setup_logging

logger = setup_logging("danenone.auth")

try:
    import pam as _pam
except ImportError:  # pragma: no cover - depends on host packages
    _pam: Any | None = None
    logger.error("PAM no está disponible; la autenticación permanecerá deshabilitada")


class PAMUnavailableError(RuntimeError):
    """Raised when the optional Python PAM binding is not installed."""


def pam_available() -> bool:
    return _pam is not None


def authenticate(username: str, password: str, service: str = "login") -> bool:
    """Authenticate through PAM without storing or logging the password.

    This function deliberately has no shell fallback. The caller should show a
    clear unavailable-state message when the binding is missing.
    """
    if not username or not password:
        logger.warning("Autenticación rechazada por credenciales vacías")
        return False
    if _pam is None:
        logger.error("Autenticación rechazada: binding PAM no disponible")
        return False
    try:
        client = _pam.pam()
        result = bool(client.authenticate(username, password, service=service))
        if result:
            logger.info("Usuario autenticado mediante PAM: %s", username)
        else:
            logger.warning("Falló la autenticación PAM para el usuario: %s", username)
        return result
    except Exception:
        logger.exception("Excepción no detallada durante la autenticación PAM para %s", username)
        return False
