# Influent Danenone 0.3.0 — ISO Arch + Hyprland

## Artefacto

La imagen fue construida con el perfil reproducible de `archiso` a partir de una base Arch mínima. Su etiqueta es `INFLUENT_DANENONE` y su volumen contiene el kernel Arch, live environment, Hyprland, GTK Layer Shell, Hyprpaper, PipeWire, WirePlumber, NetworkManager, greetd, el shell nativo en C y el tour inicial.

| Archivo | Tamaño / valor |
|---|---:|
| `influent-danenone-0.3.0-x86_64.iso` | 722,808,832 bytes |
| MD5 | `14cbc8c1ad3bf606afa93c3167cedd49` |
| SHA-256 | `722ec141c7f13a5d459fc5723b46ef4271897462dc9e44e7583d33f45f41767a` |

## Interfaz

La rama estable usa el shell nativo compilado en C con GTK3 y GTK Layer Shell. El perfil final no incluye `cloud-init` ni un paquete Python directo en la lista de la imagen; el runtime gráfico es nativo C/GTK/Wayland. La barra inferior se ancla como capa Wayland y utiliza reglas de blur de Hyprland. El notch se conserva como una capa superior separada, dejando el área superior reservada visualmente. El wallpaper predeterminado es `danenone-river-wallpaper.jpg`.

El tour nativo se instala como `/usr/local/bin/influent-danenone-tour`. La sesión se inicia mediante greetd con `dbus-run-session Hyprland` y el archivo de configuración del usuario en `/home/danenone/.config/hypr/hyprland.conf`.

## Pruebas realizadas

El shell C y el tour compilaron correctamente con GCC, GTK3 y GTK Layer Shell. `mkarchiso` completó la instalación de paquetes, generación de initramfs, configuración BIOS/UEFI, squashfs y creación ISO. `xorriso` detectó catálogo El Torito BIOS y UEFI.

La prueba QEMU TCG de bajo consumo alcanzó el menú de arranque Arch y cargó correctamente `vmlinuz-linux` e `initramfs-linux.img`. La sesión gráfica completa no pudo observarse en este sandbox porque el proceso QEMU fue limitado por memoria durante la prueba con framebuffer; esto no invalida la integridad de la ISO, pero la validación visual final debe hacerse en un host con al menos 2 GiB asignables a la VM.

## Ejecución en QEMU

```bash
qemu-system-x86_64 -enable-kvm -m 2048 -smp 2 \
  -cdrom influent-danenone-0.3.0-x86_64.iso \
  -boot d -device virtio-vga-gl -display gtk,gl=on
```

Si KVM no está disponible, se puede usar TCG con menos rendimiento:

```bash
qemu-system-x86_64 -m 2048 -smp 2 \
  -cdrom influent-danenone-0.3.0-x86_64.iso \
  -boot d -display gtk
```

## Ejecución en VirtualBox

Crear una VM Linux de 64 bits con al menos 2 GiB de RAM, dos CPUs virtuales y firmware EFI activado. Montar la ISO como unidad óptica, iniciar la VM y seleccionar `Arch Linux install medium (x86_64), BIOS` si se inicia en modo BIOS. Para una prueba gráfica, activar aceleración 3D solo si el host tiene controladores funcionales; si aparecen fallos visuales, desactivarla y probar con renderizado básico.

## Verificación de integridad

```bash
md5sum -c influent-danenone-0.3.0-x86_64.iso.md5
sha256sum -c influent-danenone-0.3.0-x86_64.iso.sha256
```

El Bot API de Telegram no admite subir directamente este archivo de aproximadamente 709 MiB porque su límite para videos/documentos mediante bot es inferior. Por eso la publicación pública debe incluir los checksums, estos pasos y un enlace de descarga desde un release de GitHub; no se debe fragmentar la ISO sin documentar un procedimiento de recomposición.
