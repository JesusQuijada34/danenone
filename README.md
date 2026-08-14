# Danenone

Danenone es un prototipo de escritorio Linux con una sesión visual Python/Qt, un cliente FoundStore compatible con releases Fluthin, un planificador de instalación seguro y un constructor de ISO reproducible.

## Ejecutar la interfaz

En un entorno con PyQt5 o PySide6 disponible:

```bash
PYTHONPATH=src python3 -m danenone_shell.app
```

La interfaz incluye escritorio de páginas horizontales, puntos de navegación, barra de tareas compacta, notch superior, centro de control y paneles translúcidos. Los efectos acrílicos son una aproximación Qt del prototipo; el compositor final deberá integrarse con Wayland.

## Ejecutar pruebas

```bash
python3 -m pytest -q
```

## FoundStore

El cliente en `src/foundstore_client.py` lee releases GitHub por HTTPS, filtra assets `.iflapp`, verifica tamaño y SHA-256 cuando está disponible, inspecciona el ZIP sin extraerlo y rechaza path traversal antes de escribir un archivo.

## ISO

```bash
PYTHONPATH=src python3 -m danenone_shell.iso_builder --manifest build/manifest
PYTHONPATH=src python3 -m danenone_shell.iso_builder --build --output build/danenone.iso
```

El primer comando siempre funciona y produce un manifiesto de capacidad. El segundo requiere `live-build`, `debootstrap`, `grub-mkrescue`, `xorriso` y `mksquashfs`; si faltan, devuelve un error explícito y no crea una ISO falsa.

## Instalador

El instalador comienza en modo simulación y valida el dispositivo, usuario, hostname y zona horaria. La escritura real de discos no está habilitada en este prototipo.

## Estado

La arquitectura, shell de escritorio, cliente FoundStore, validaciones y manifiesto de ISO están implementados. La ISO completa requiere instalar herramientas de construcción de imágenes y añadir una raíz Linux, kernel, initramfs, paquetes Qt y configuración de arranque verificables.
