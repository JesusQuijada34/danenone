# Fuentes verificadas para Termux:X11

## Documentación oficial

Fuente: https://github.com/termux/termux-x11

La documentación oficial describe Termux:X11 como un servidor X optimizado para Termux. Requiere Android 8 o posterior y dos componentes: la aplicación Android y el paquete complementario de Termux. Indica instalar `x11-repo` y `termux-x11-nightly`, iniciar el servidor con `termux-x11 :1`, y usar `DISPLAY=:1` para las aplicaciones. Para proot recomienda iniciar el contenedor con `--shared-tmp`. También documenta `-legacy-drawing` para dispositivos que muestran una pantalla negra y `-force-bgra` para colores intercambiados.

La guía de Danenone usa estos datos únicamente para el launcher no-root de Termux. La ISO de Influent Danenone es amd64, por lo que QEMU debe traducir instrucciones en teléfonos ARM y el rendimiento es limitado.

## Repositorios de referencia

Termux:X11: https://github.com/termux/termux-x11

Release nightly de Termux:X11: https://github.com/termux/termux-x11/releases/tag/nightly

PRoot-Distro: https://github.com/termux/proot-distro
