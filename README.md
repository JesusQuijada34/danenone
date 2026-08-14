# Influent Danenone

Influent Danenone es un prototipo de sistema operativo Linux con una sesión visual Python/Qt inspirada en la claridad espacial de iOS: escritorio compacto por páginas, notch superior, centro de control, barra inferior translúcida, widgets y una organización de aplicaciones orientada a pantallas táctiles.

## Imagen arrancable

La receta `iso/build_iso.sh` construye una ISO Debian bookworm con kernel Linux amd64, live-boot, GRUB, Xorg, Nodm, Python 3, PyQt5, requests y NetworkManager. La imagen usa el volumen `INFLUENT`, muestra las entradas `Influent Danenone - Live Session` en el bootloader y emplea un fondo visual Influent propio. Nodm inicia automáticamente el usuario `influent` y ejecuta el shell Qt.

```bash
sudo ./iso/build_iso.sh
```

La ISO publicada `influent-danenone-0.2.0-preview-amd64.iso` es una imagen amd64. SHA-256 actual: `6407c305909cf0dd71d2fc447cc233da83bfa9119c77246d86a91aab850575e2`.

## Ejecutar el shell en un entorno gráfico existente

```bash
PYTHONPATH=src python3 -m danenone_shell.app
```

## FoundStore e instalador

El cliente FoundStore lee releases GitHub, filtra assets `.iflapp`, valida tamaños y SHA-256 cuando existe, inspecciona el ZIP sin extraerlo y rechaza path traversal. El instalador comienza en modo simulación y valida dispositivo, usuario, hostname y zona horaria; la escritura real de discos aún no está habilitada.

## Termux + Termux:X11

Para probar la ISO desde Android sin root, instala Termux y Termux:X11, abre Termux:X11 una vez y ejecuta:

```bash
git clone https://github.com/JesusQuijada34/danenone "$HOME/src/danenone"
cd "$HOME/src/danenone"
chmod +x termux/run-influent-danenone.sh
INFLUENT_ISO_SHA256='6407c305909cf0dd71d2fc447cc233da83bfa9119c77246d86a91aab850575e2' \\
  ./termux/run-influent-danenone.sh
```

El script instala QEMU para Termux, inicia `termux-x11 :1`, descarga la ISO del release y ejecuta la máquina amd64 con traducción TCG. En teléfonos ARM será lento porque no es una imagen ARM nativa. Consulta [README_TERMUX.md](README_TERMUX.md) para requisitos, solución de pantalla negra y alternativa ligera con proot-distro.

## Pruebas

```bash
PYTHONPATH=src python3 -m pytest -q
bash -n termux/run-influent-danenone.sh
python3 -m py_compile src/danenone_shell/app.py
```

## Estado

El prototipo ya arranca el kernel Linux, presenta un bootloader con identidad Influent Danenone y llega automáticamente al escritorio Qt dentro de QEMU. La experiencia visual es funcional y verificable, pero todavía no es una distribución de producción: faltan un compositor Wayland propio, instalación real de disco, persistencia de usuarios y una implementación completa del administrador de ventanas.
