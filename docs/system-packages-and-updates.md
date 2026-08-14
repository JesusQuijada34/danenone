# Paquetes del sistema y actualizaciones de Influent Danenone

Influent Danenone instala sus aplicaciones propias como paquetes `.iflapp` compilados por PackageMaker. Los artefactos actuales son `Influent.danenone-shell`, `Influent.influent-updater` e `Influent.influent-notifications`, todos con plataforma `Danenone`, publisher `Influent` y versión `0.3-26.08-21.56`.

Los paquetes se extraen en `/usr/lib/influent/packages/<nombre-del-paquete>/`. Cada carpeta contiene `details.xml`, el binario compilado `app/app`, documentación y recursos. El runtime Python de Linux se instala por separado en `/opt/influent-danenone/src`; los lanzadores compilados de PackageMaker usan ese runtime para ejecutar el shell y los servicios del sistema. Esto permite distribuir aplicaciones como Fluthin sin abandonar Python como motor de la sesión Linux.

El actualizador lee `/var/lib/influent/installed-packages.json`, abre cada `details.xml` y toma `author`, `app`, `version`, `publisher` y `platform`. Para una actualización remota consulta los releases públicos de `https://api.github.com/repos/<author>/<app>/releases`. No basta con que exista una etiqueta: el release debe ser público, no ser borrador ni pre-release, tener una versión superior y contener un asset `.iflapp` cuyo nombre corresponda a la aplicación. Antes de cualquier instalación se valida el ZIP, se exige `details.xml`, se comprueba el publisher y se bloquea cualquier ruta con path traversal.

El sistema de notificaciones conserva las entradas en `/var/lib/influent/notifications.json` cuando la sesión puede escribir allí. En una sesión sin privilegios usa `$XDG_STATE_HOME/influent/notifications.json`. Cada entrada guarda origen, título, cuerpo, hora, prioridad, duración y estado de lectura. El shell muestra un icono SVG en el status bar, un panel bajo el notch y un historial con acción `Limpiar todo`.

## Estados reales del hardware

Wi-Fi y conectividad se consultan mediante NetworkManager o interfaces de red; Bluetooth usa `bluetoothctl`; batería se lee de `/sys/class/power_supply` o UPower; volumen se obtiene de PipeWire/PulseAudio; y brillo se obtiene de `/sys/class/backlight` o `brightnessctl`. Cuando el hardware o la utilidad no están disponibles se muestra `No disponible`; el sistema no fabrica porcentajes ni estados.
