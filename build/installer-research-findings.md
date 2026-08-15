# Hallazgos para el instalador nativo de Influent Danenone

## Fuentes consultadas

1. ArchWiki, [Archiso](https://wiki.archlinux.org/title/Archiso).
2. ArchWiki, [Partitioning](https://wiki.archlinux.org/title/Partitioning).
3. GTK Layer Shell, [wmww/gtk-layer-shell](https://github.com/wmww/gtk-layer-shell).
4. Wayland, [wlr-layer-shell-unstable-v1](https://wayland.app/protocols/wlr-layer-shell-unstable-v1).

## Decisiones técnicas

Archiso admite perfiles personalizados con `packages.x86_64`, archivos bajo `airootfs`, hooks de pacman, unidades systemd y configuración separada para Syslinux, GRUB y systemd-boot. La imagen final puede soportar BIOS y UEFI mediante El Torito e Isohybrid. El instalador se incluirá dentro de `airootfs` y se ejecutará únicamente en modo live hasta que el usuario confirme explícitamente la escritura en disco.

GTK Layer Shell funciona sobre Wayland y compositores que implementan `wlr-layer-shell`, incluidos Hyprland. Permite anclar superficies a bordes, seleccionar capas y definir zonas exclusivas. Esto permite mostrar un notch visual en la capa superior y el asistente en una superficie separada sin dibujarlo detrás del recorte conceptual.

Para particionado, el asistente debe detectar discos y particiones con `lsblk --json` y `blkid`, mostrar modelos/tamaños/etiquetas reales, bloquear el botón de instalación mientras el plan no esté validado y exigir una confirmación adicional antes de ejecutar `wipefs`, `sgdisk`, `mkfs`, `mount` o modificar NVRAM. El modo multiboot debe conservar la ESP existente cuando sea compatible, detectar Windows/otras instalaciones y escribir una entrada de arranque separada para Danenone.

La política por defecto será UEFI + GPT con una ESP existente o nueva, una partición raíz y una opción de home separada. BIOS/MBR y BIOS sobre GPT requieren rutas de bootloader diferentes; el instalador debe detectarlo y no prometer compatibilidad universal sin validación del firmware.

## Restricciones de seguridad

El instalador no debe ejecutar comandos destructivos durante la navegación, no debe aceptar una ruta de disco introducida como texto sin resolverla contra `/sys/block` y `/dev`, y no debe confiar en títulos o tamaños mostrados por una UI sin volver a validar el dispositivo justo antes de escribir. La descarga de paquetes debe usar pacman con firmas verificadas y registrar errores sin contraseñas ni claves privadas.

La experiencia visual puede parecerse a Windows 11/macOS mediante GTK y animaciones, pero el notch de UEFI no puede ser una superficie GTK: durante UEFI solo puede representarse mediante un menú/imagen de bootloader. La versión nativa del notch comienza cuando arranca el kernel y se inicia Hyprland/GTK Layer Shell.
