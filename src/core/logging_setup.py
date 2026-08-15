from __future__ import annotations

import logging
import os
from logging.handlers import RotatingFileHandler
from pathlib import Path

_LOGGER_MARKER = "danenone-rotating-file"


def log_directory() -> Path:
    """Return the project-local log directory without touching system paths."""
    configured = os.environ.get("DANENONE_LOG_DIR")
    return Path(configured).expanduser() if configured else Path.home() / "danenone" / "logs"


def setup_logging(module_name: str = "danenone_shell") -> logging.Logger:
    """Configure project logging once and return a module-scoped logger.

    Logs are rotated at 5 MiB with three backups. Passwords and authentication
    secrets must never be passed as log arguments by callers.
    """
    directory = log_directory()
    directory.mkdir(parents=True, exist_ok=True)
    log_file = directory / f"{module_name}.log"
    root_logger = logging.getLogger()
    root_logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter(
        "%(asctime)s %(levelname)s %(name)s: %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )
    marker = f"{_LOGGER_MARKER}:{log_file}"
    if not any(getattr(handler, "_danenone_marker", None) == marker for handler in root_logger.handlers):
        handler = RotatingFileHandler(
            log_file,
            maxBytes=5 * 1024 * 1024,
            backupCount=3,
            encoding="utf-8",
        )
        handler._danenone_marker = marker  # type: ignore[attr-defined]
        handler.setFormatter(formatter)
        root_logger.addHandler(handler)
    if os.environ.get("DANENONE_DEBUG") == "1" and not any(
        getattr(handler, "_danenone_console", False) for handler in root_logger.handlers
    ):
        console = logging.StreamHandler()
        console._danenone_console = True  # type: ignore[attr-defined]
        console.setFormatter(formatter)
        root_logger.addHandler(console)
    return logging.getLogger(module_name)


logger = setup_logging()
