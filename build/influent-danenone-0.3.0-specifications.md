# Influent Danenone 0.3.0 — Especificaciones públicas

## Identidad del producto

**Influent Danenone** es una imagen Linux x86_64 construida sobre Arch Linux y empaquetada con Archiso. La experiencia gráfica utiliza Hyprland como compositor Wayland y un shell nativo escrito en C con GTK3 y GTK Layer Shell. La interfaz conserva el notch superior, la barra inferior centrada y la estética acrílica inspirada en Windows 11 e iOS.

| Área | Especificación |
|---|---|
| Arquitectura | x86_64 |
| Base | Arch Linux live environment |
| Constructor | Archiso |
| Compositor | Hyprland 0.56.2 |
| Toolkit del shell | C, GTK3, GTK Layer Shell |
| Sesión | greetd + `dbus-run-session Hyprland` |
| Wallpaper | Arroyo de montaña con cubo transparente y marca Danenone |
| Audio | PipeWire + WirePlumber |
| Red | NetworkManager |
| Portales | xdg-desktop-portal-hyprland |
| Terminal | foot |
| Tour | `/usr/local/bin/influent-danenone-tour` |
| Shell | `/usr/local/bin/influent-danenone-shell` |
| Boot | BIOS Syslinux + UEFI systemd-boot |

## Interfaz

El shell C crea capas Wayland independientes para la barra inferior y el notch. La barra se centra en el borde inferior y está preparada para aplicar blur mediante reglas del compositor. El notch reserva el espacio superior y evita que la composición visual se dibuje detrás del recorte conceptual. El fondo oficial combina el arroyo proporcionado, el cubo que emerge del agua y el texto Danenone.

## Sesión y usuario live

La imagen crea el usuario live `danenone` con sesión gráfica preparada por greetd. La configuración de Hyprland se instala en `/home/danenone/.config/hypr/hyprland.conf`. El tour es un ejecutable nativo independiente y puede reemplazarse o actualizarse sin reescribir el shell principal.

## Seguridad y alcance

La imagen es un medio live de demostración y desarrollo. No se incluye una contraseña predeterminada para el usuario live. El runtime gráfico no depende de Python ni de PyQt. El perfil final no incluye `cloud-init` ni un paquete Python directo en su lista de paquetes; los componentes visuales de Danenone son C/GTK/Wayland.

## Artefacto y verificación

| Archivo | Valor |
|---|---:|
| ISO | 722,808,832 bytes |
| MD5 | `14cbc8c1ad3bf606afa93c3167cedd49` |
| SHA-256 | `722ec141c7f13a5d459fc5723b46ef4271897462dc9e44e7583d33f45f41767a` |

Verificación local:

```bash
md5sum -c influent-danenone-0.3.0-x86_64.iso.md5
sha256sum -c influent-danenone-0.3.0-x86_64.iso.sha256
```

## Requisitos recomendados para VM

Para probar la sesión gráfica se recomiendan al menos 2 GiB de RAM, dos vCPU, firmware EFI opcional y aceleración gráfica disponible. En QEMU puede iniciarse con:

```bash
qemu-system-x86_64 -enable-kvm -m 2048 -smp 2 \
  -cdrom influent-danenone-0.3.0-x86_64.iso -boot d
```

En hosts sin KVM se puede retirar `-enable-kvm`, aunque el arranque será más lento. La validación realizada en el sandbox confirmó BIOS/UEFI, el menú de arranque y la carga del kernel e initramfs; la validación visual completa requiere un host que no finalice la VM por presión de memoria.
