# Plantilla Calamares para Influent Danenone

Este paquete es un **andamio no activado** para una edición futura. Sus archivos se instalan bajo `/usr/share/danenone/calamares-template`; por diseño no modifica `/etc/calamares`, no instala un lanzador y no puede iniciar trabajos de particionado o escritura de discos.

La plantilla contiene las pantallas de bienvenida, ubicación, teclado, particionado, usuario y resumen, pero su fase `exec` es una lista vacía. Antes de activarla se deben crear los archivos de módulo específicos para `mount`, `unpackfs`, `fstab`, `users`, `displaymanager` y `bootloader`, y validarlos en una máquina virtual con discos desechables.

El branding utiliza recursos oficiales de Danenone y un slideshow QML local. No descarga contenido durante la instalación, no sube registros y no incluye telemetría, `webview`, comandos de shell o secretos del OOBE.

## Contrato con el OOBE

El helper `influent-oobe-export-installer-handoff` puede producir `/etc/influent-danenone/installer-handoff.conf` con los campos validados `SELECTED_LANGUAGE` y `SELECTED_EDITION`. Una futura integración puede usar el idioma como valor inicial de `locale`, siempre conservando la posibilidad de modificarlo en Calamares. La edición sirve únicamente para elegir conjuntos de paquetes previamente auditados.

La plantilla no lee ese archivo todavía. No debe aceptar desde el handoff contraseñas, identificadores de disco, reglas de particionado, tokens, redes ni decisiones de borrado; estos valores deben confirmarse dentro del instalador y el particionado debe partir sin selección predeterminada.
