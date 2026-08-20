#!/usr/bin/env python3
"""Actualizador deshabilitado para Cloud Danenone Devices.

Las actualizaciones del agente se resuelven mediante un flujo explícito de
Foundstore o Fluthin. Este módulo no descarga, extrae ni instala paquetes por
su cuenta, y no se ejecuta como un servicio en segundo plano.
"""

import sys


def main() -> int:
    print("Las actualizaciones de Cloud Danenone Devices requieren confirmación local.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
