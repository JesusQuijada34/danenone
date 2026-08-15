*Influent Danenone — próximo asistente de instalación*

Estamos diseñando una instalación completa y nativa en C para que Danenone no sea solo un live system, sino una experiencia de inicio a fin.

*La nueva experiencia incluirá*

• Un asistente paso a paso con tarjetas translúcidas, animaciones suaves y acabado de vidrio líquido.
• Un notch visible desde el menú de arranque y durante la sesión gráfica, como parte nativa de la identidad visual.
• Detección de Wi-Fi y conectividad para descargar paquetes firmados cuando el usuario lo permita.
• Selección segura de disco, particionado UEFI/BIOS y opciones de instalación sin borrar nada hasta confirmar explícitamente.
• Modo multiboot que detecta instalaciones existentes y conserva la partición EFI cuando sea compatible.
• Teclado, idioma, región, zona horaria y preferencias de privacidad.
• Creación del usuario principal, configuración del entorno y aplicación de cambios.
• Reinicio controlado e inicio de sesión en el usuario creado.
• Pantalla de primer arranque para terminar la personalización.
• Tour guiado acompañado por el cubito Danenone, que señalará la barra, el notch, el Centro de control, Archivos y las funciones principales.

*Arquitectura*

El instalador se implementará en C sobre GTK y Wayland/GTK Layer Shell. El proceso destructivo estará separado de la interfaz: la navegación solo construirá un plan, y las operaciones sobre discos exigirán validación del dispositivo y una confirmación final. Los paquetes se instalarán con verificación de firmas.

*Estado actual*

La ISO Arch + Hyprland 0.3.0 ya está publicada. El instalador gráfico completo y el tour de primer arranque son la siguiente fase de desarrollo; este banner presenta el diseño previsto y no una función que ya esté disponible en la ISO actual.

[Release público de Influent Danenone 0.3.0](https://github.com/JesusQuijada34/danenone/releases/tag/v0.3.0)

*Influent Danenone: una instalación clara, segura y acompañada desde el primer arranque.*
