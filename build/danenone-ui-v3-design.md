# Danenone UI v3 — contrato visual nativo

## Cubito Danenone

El cubito será un componente GTK/Cairo independiente con una máquina de estados: `IDLE`, `TALKING`, `WHISPER`, `THINKING`, `POINTING` y `CELEBRATING`. Cada estado cambia el sprite dibujado, el ritmo de parpadeo, la inclinación y el tipo de burbuja. El texto se presenta como texto GTK accesible; los susurros y pensamientos no dependen de audio ni de caracteres especiales.

La animación usa un temporizador GLib de 60 Hz aproximados y un `GtkDrawingArea`. El componente no bloquea el hilo principal, no crea procesos externos y puede reutilizarse en el tour y en el primer arranque.

## Gestor de ventanas

El gestor nativo separará la ventana activa de las superficies auxiliares. La ventana enfocada conservará una superficie clara u oscura completamente legible, mientras que las ventanas no activas usarán una capa translúcida con blur del compositor. El estado depende de la preferencia `light`/`dark` y no modifica el contenido de las aplicaciones.

En Wayland, la barra, el notch y los overlays usarán Layer Shell. El gestor no simulará posiciones de otras ventanas si el compositor no ofrece esa información; en ese caso mostrará únicamente el estado visual disponible.

## Configuración

El menú se organizará en categorías: Apariencia, Escritorio, Ventanas, Notificaciones, Red y sistema. Apariencia incluirá modo claro/oscuro, intensidad del vidrio, radio de esquinas, color de acento y familia tipográfica. Los cambios se escribirán en un archivo de preferencias versionado bajo `$XDG_CONFIG_HOME/influent-danenone/settings.conf` y se aplicarán sin guardar contraseñas.

## Accesibilidad y seguridad

Toda burbuja tendrá una etiqueta GTK legible por lectores de pantalla. El usuario podrá avanzar, volver, omitir o cerrar el tour. El estado se persiste en `$XDG_STATE_HOME/influent-danenone/`. Ninguna animación debe impedir el acceso al botón de cierre ni capturar el teclado de forma permanente.
