# Influent Danenone v0.3.0-preview

Esta preview integra el runtime Python sobre Linux con tres aplicaciones propias compiladas por PackageMaker como paquetes Fluthin: el shell `Influent.danenone-shell`, el actualizador `Influent.influent-updater` y el servicio `Influent.influent-notifications`. Los paquetes se instalan en `/usr/lib/influent/packages` y conservan `details.xml`, el binario `app/app`, documentación y SVGs.

El actualizador analiza `/var/lib/influent/installed-packages.json`, obtiene `author`, `app`, `version`, `publisher` y `platform` desde XML, consulta los releases de GitHub y notifica únicamente cuando encuentra una versión superior con un asset `.iflapp` verificable. La validación bloquea ZIPs incompletos y path traversal.

El centro de notificaciones dispone de icono SVG en el status bar, panel desplegable bajo el notch, historial persistente, hora, prioridad, duración, lectura y acción de limpiar todo. Los estados de Wi-Fi, red, Bluetooth, batería, brillo y volumen se leen desde el hardware o las utilidades del sistema; cuando no existe una capacidad se muestra `No disponible`.

Calamares está incluido con branding Influent Danenone, logo SVG, textos de bienvenida y colores propios. El flujo de particionado debe probarse en hardware o máquina virtual con confirmación explícita; la ISO no ejecuta operaciones destructivas automáticamente.

La ISO amd64 es arrancable mediante GRUB/El Torito. SHA-256:

```text
015f82a0272f4dbe7b13df2692b3bad76e50a8fb26da2d575a837a8e942ab793
```

El build conserva las fuentes Python, pero la sesión gráfica apunta al binario `app/app` compilado del paquete Fluthin. El binario usa Python del runtime instalado en `/opt/influent-danenone/src`, de modo que el sistema separa distribución compilada y motor Python Linux.
