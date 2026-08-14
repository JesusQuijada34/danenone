# Influent Danenone v0.2.0-preview

Esta versión sustituye la identidad genérica del prototipo por **Influent Danenone**. El bootloader muestra entradas Influent Danenone y usa un fondo visual propio con degradados azul-violeta, vidrio y una isla central abstracta. El volumen de la ISO es `INFLUENT`.

La imagen integra kernel Linux amd64, live-boot, GRUB, Xorg, Nodm, Python 3, PyQt5, requests y NetworkManager. Nodm inicia automáticamente el usuario `influent` y abre el shell Qt, que presenta escritorio por páginas, notch superior, centro de control, dock inferior, widgets y acceso visual a FoundStore.

La ISO fue arrancada en QEMU y llegó al escritorio Qt Influent Danenone. El asset es amd64 y mide aproximadamente 500 MB. SHA-256: `6407c305909cf0dd71d2fc447cc233da83bfa9119c77246d86a91aab850575e2`.

Se incluye `README_TERMUX.md` y `termux/run-influent-danenone.sh` para instalar las dependencias de Termux, iniciar Termux:X11 y emular la ISO con QEMU sin root. En teléfonos ARM se usa traducción TCG y el rendimiento será limitado.

Esta sigue siendo una preview: el instalador real de disco, el compositor Wayland propio y la persistencia completa aún no están habilitados.
