# Influent Danenone en Termux + Termux:X11

Este método ejecuta la ISO amd64 de Influent Danenone dentro de QEMU en un teléfono Android. No instala Danenone como sistema nativo: crea una máquina virtual emulada y muestra su ventana mediante Termux:X11. En teléfonos ARM, QEMU debe traducir instrucciones amd64 con TCG, por lo que el rendimiento será considerablemente menor que en un PC.

## Requisitos

Se necesita Android 8 o posterior, Termux procedente de una fuente compatible con sus complementos y la aplicación Termux:X11. La documentación oficial indica que Termux:X11 está compuesto por una aplicación Android y un paquete complementario para Termux; ambos deben instalarse [1]. También documenta el uso del servidor con `termux-x11 :1` y la necesidad de `--shared-tmp` cuando se trabaja dentro de un entorno proot [1].

Instala y abre una vez el APK `termux-x11-universal-debug.apk` desde el [release nightly oficial](https://github.com/termux/termux-x11/releases/tag/nightly). Después, en Termux, clona o descarga este repositorio y ejecuta:

```bash
pkg update -y
pkg install -y git
mkdir -p "$HOME/src"
git clone https://github.com/JesusQuijada34/danenone "$HOME/src/danenone"
cd "$HOME/src/danenone"
chmod +x termux/run-influent-danenone.sh
./termux/run-influent-danenone.sh
```

El script habilita el repositorio gráfico, instala el paquete nightly de Termux:X11, instala QEMU headless, descarga la ISO desde el release de Danenone, inicia `termux-x11 :1` con `-legacy-drawing` y abre QEMU con la ISO como CD-ROM. Si el teléfono muestra una pantalla negra, la opción `-legacy-drawing` está contemplada por la guía oficial de Termux:X11 [1].

## Verificación opcional

Para validar la descarga antes de arrancar, ejecuta:

```bash
export INFLUENT_ISO_SHA256='6407c305909cf0dd71d2fc447cc233da83bfa9119c77246d86a91aab850575e2'
./termux/run-influent-danenone.sh
```

También puedes cambiar la memoria asignada sin editar el script:

```bash
INFLUENT_RAM=1536 ./termux/run-influent-danenone.sh
```

## Alternativa ligera sin emular la ISO

La emulación de la ISO completa es la ruta que más se parece a probar el sistema real, pero es la más lenta. Para una prueba rápida del shell Qt, la alternativa es instalar Debian mediante `proot-distro`, instalar Python/PyQt5 dentro del contenedor, compartir `/tmp` y ejecutar el shell con `DISPLAY=:1`. La documentación oficial de Termux:X11 muestra este patrón: iniciar el servidor desde Termux, entrar al contenedor con `--shared-tmp`, exportar `DISPLAY=:1` y comenzar la sesión gráfica [1]. Esta alternativa no arranca el kernel ni el bootloader de Influent Danenone.

## Limitaciones conocidas

La imagen publicada es amd64 y no una imagen ARM nativa. En Android ARM, QEMU usa traducción de CPU y el arranque puede tardar varios minutos. El proceso necesita espacio suficiente para descargar aproximadamente 500 MB de ISO y memoria libre para QEMU. La ventana se renderiza mediante Termux:X11; si el dispositivo tiene colores invertidos, la documentación oficial recomienda `-force-bgra` [1].

> Recomendación: comenzar con `INFLUENT_RAM=1536`, mantener Termux:X11 visible durante la prueba y cerrar QEMU desde Termux con `Ctrl-C` cuando termine.

## Referencias

[1]: https://github.com/termux/termux-x11 "Termux:X11 — documentación oficial y README"
