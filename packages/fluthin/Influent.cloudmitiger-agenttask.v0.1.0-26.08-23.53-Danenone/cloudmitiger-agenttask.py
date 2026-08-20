#!/usr/bin/env python3
"""Punto de entrada de Cloud Danenone Devices para Danenone.

El paquete delega en el cliente local DaneDesk. No recoge ubicación ni transmite
identificadores de hardware sin transformar: el cliente deriva una huella
SHA-256 opaca y exige HTTPS para interactuar con el servicio configurado.
"""

from app.danedesk_client import main as danedesk_main


def main() -> int:
    return danedesk_main()


if __name__ == "__main__":
    raise SystemExit(main())
