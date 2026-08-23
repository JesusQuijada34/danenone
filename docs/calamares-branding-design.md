# Diseño inicial de configuración y branding Calamares

## Principio de integración

La configuración de Danenone se distribuirá como un paquete separado bajo `/etc/calamares`; no se modifican los ejemplos instalados en `/usr/share/calamares` ni se añade Calamares a la ISO Plasma RC1 publicada. Esta separación está recomendada explícitamente por el proyecto Calamares. [1]

## Flujo inicial propuesto

La futura edición usará el recorrido mínimo de instalación: bienvenida, localización, teclado, particionado, usuario, resumen, instalación y finalización. Se excluyen los módulos de red, telemetría, `webview`, ejecución arbitraria y recuperación de secretos. El instalador no consume contraseñas ni decisiones destructivas del handoff actual del OOBE; el contrato sólo puede transportar idioma y edición.

| Etapa | Módulo previsto | Decisión de seguridad |
| --- | --- | --- |
| Inicio | `welcome`, `locale`, `keyboard` | Selección explícita y reversible; no hay detección de red ni GeoIP automática. |
| Almacenamiento | `partition` | Debe exigir confirmación explícita de destino; dual boot y borrado total sólo se habilitan tras pruebas con discos desechables. |
| Cuenta | `users` | El usuario escribe la contraseña directamente; nunca se importa desde OOBE. |
| Revisión | `summary` | Debe mostrar disco objetivo, modo de instalación, zona horaria y usuario antes del paso de ejecución. |
| Ejecución | `mount`, `unpackfs`, `fstab`, `locale`, `users`, `displaymanager`, `bootloader` | Se habilita sólo en una futura ISO de prueba tras validar el medio de instalación. |
| Cierre | `finished` | No informa resultados de telemetría ni activa servicios remotos. |

## Branding QML

El paquete de branding tendrá un descriptor `branding.desc`, `show.qml` y una hoja de estilo propios. El diseño empleará el acento Verdypor, jerarquía legible y movimientos moderados; no replica tipografía, iconos ni trade dress de Apple. El slideshow seguirá la API QML de Calamares y no descargará recursos remotos durante la instalación. [2]

## Validación obligatoria

La configuración se validará con Calamares compilado en chroot, pruebas de esquema y máquinas virtuales con discos de prueba antes de incluirse en `packages/editions/plasma.packages`. La validación deberá cubrir instalación manual, instalación guiada, cancelación y un escenario de arranque dual.

## Referencias

[1]: https://calamares.codeberg.page/docs/deploy-configuration/ "Calamares Configuration"
[2]: https://github.com/calamares/calamares-extensions "Calamares Branding and Module Examples"
[3]: https://calamares.io/docs/users-guide/ "Calamares User's Guide"
