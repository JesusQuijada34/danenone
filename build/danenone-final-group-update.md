*Influent Danenone 0.3.0 — resultado de la compilación*

La ISO final ya fue generada con el perfil mínimo de Archiso y está disponible en el release público:

[Descargar Influent Danenone 0.3.0](https://github.com/JesusQuijada34/danenone/releases/tag/v0.3.0)

*Componentes incluidos*

• Arch Linux live environment.
• Hyprland + greetd para la sesión Wayland.
• Shell nativo en C con GTK3 y GTK Layer Shell.
• Barra inferior acrílica y notch superior.
• Hyprpaper con el fondo del arroyo.
• PipeWire, WirePlumber, NetworkManager y portal Hyprland.
• Tour nativo de bienvenida.
• Perfil gráfico sin `cloud-init` ni un paquete Python directo; el runtime visual es C/GTK/Wayland.

*Checksums*

`MD5 14cbc8c1ad3bf606afa93c3167cedd49`

`SHA-256 722ec141c7f13a5d459fc5723b46ef4271897462dc9e44e7583d33f45f41767a`

*Validación*

El shell C y el tour compilaron con GCC, GTK3 y GTK Layer Shell. Archiso completó el kernel, initramfs, BIOS, UEFI, squashfs y catálogo El Torito. QEMU TCG alcanzó el menú de arranque y cargó `vmlinuz-linux` e `initramfs-linux.img`.

La validación gráfica completa necesita un host con más memoria. En este sandbox, la prueba framebuffer con 2 GiB fue terminada por presión de memoria; la prueba ligera sí alcanzó el arranque del kernel. No se presenta esa limitación como un fallo confirmado de la ISO.

El notch se conserva, tal como pidió la comunidad, junto con el Centro de control inspirado en iOS. La próxima fase será probar la sesión gráfica en una VM con recursos suficientes y corregir cualquier detalle visual que aparezca.
