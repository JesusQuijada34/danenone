*Influent Danenone 0.3.0 — ISO Arch + Hyprland disponible*

La nueva imagen de Influent Danenone ya fue construida con un perfil reproducible de Archiso.

*Incluye*

• Arch Linux live environment.
• Hyprland y sesión Wayland mediante greetd.
• Shell nativo en C con GTK3 y GTK Layer Shell.
• Barra inferior acrílica estilo Windows 11.
• Notch superior conservado como capa Wayland independiente.
• Hyprpaper con el fondo del arroyo seleccionado.
• PipeWire, WirePlumber, NetworkManager y portal de escritorio.
• Tour nativo de bienvenida para recorrer el escritorio.
• Perfil gráfico sin `cloud-init` ni un paquete Python directo; el shell es C/GTK/Wayland.

*Descarga*

[Release Influent Danenone 0.3.0](https://github.com/JesusQuijada34/danenone/releases/tag/v0.3.0)

*Integridad*

MD5: `14cbc8c1ad3bf606afa93c3167cedd49`

SHA-256: `722ec141c7f13a5d459fc5723b46ef4271897462dc9e44e7583d33f45f41767a`

*Prueba rápida*

```bash
md5sum -c influent-danenone-0.3.0-x86_64.iso.md5
sha256sum -c influent-danenone-0.3.0-x86_64.iso.sha256
qemu-system-x86_64 -enable-kvm -m 2048 -smp 2 -cdrom influent-danenone-0.3.0-x86_64.iso -boot d
```

El Bot API de Telegram no permite adjuntar directamente una ISO de aproximadamente 709 MiB. Por eso la ISO está en el release público de GitHub y aquí se publican el enlace, los checksums y los pasos reproducibles.

La prueba TCG ligera alcanzó el menú de arranque y cargó el kernel e initramfs. La validación gráfica completa requiere un host con más memoria que este sandbox, porque el proceso de VM fue terminado por presión de memoria.

*Influent Danenone: Arch, Hyprland, C nativo, notch, barra acrílica y tour integrado.*
