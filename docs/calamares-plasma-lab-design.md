# Diseño de laboratorio: Plasma/KWin único

## Propósito

La futura ISO de laboratorio usa **KDE Plasma con KWin como único entorno predeterminado**. Esta decisión aprovecha el panel, lanzador de aplicaciones, gestor de tareas, administración de ventanas y módulos de configuración mantenidos por KDE, en lugar de sostener un escritorio, dock y barra propios.

Hyprland se mantiene instalado únicamente como **sesión avanzada seleccionable desde SDDM**. No existe una edición nativa paralela, ni una segunda entrada GRUB que pueda cambiar de raíz live o seleccionar una instalación. Plasma/KWin será el entorno visible por defecto para la futura validación de Calamares.

## Arquitectura de laboratorio

| Capa | Decisión | Límite de seguridad |
| --- | --- | --- |
| Perfil Archiso | `plasma-lab.conf`, derivado del conjunto Plasma y con identidad de artefacto propia `influent-danenone-plasma-lab`. | Se genera fuera del directorio de salida de RC1 y no modifica `editions/plasma.conf`. |
| Sesión predeterminada | SDDM selecciona `plasma.desktop`; Plasma/KWin es el escritorio principal. | La configuración de sesión no contiene decisiones de disco ni credenciales. |
| Sesión avanzada | Hyprland queda disponible como `hyprland.desktop`. | No sustituye la sesión predeterminada ni instala un segundo gestor de inicio. |
| Calamares | Permanece pendiente y aislado hasta incorporar una configuración activa a una ISO nueva. | No hay `settings.conf` real ni `/etc/calamares` en RC1 o en este esqueleto. |
| GRUB | Conserva un único arranque para la raíz Plasma Lab cuando se construya el artefacto. | No decide la edición instalada, no recibe secretos y no activa particionado. |

Calamares separa las páginas visibles de la fase de trabajo `exec`; la configuración de distribución debe instalarse por separado del runtime y validarse antes de realizar cambios. [1] Por ello, la futura instalación conservará confirmaciones explícitas en Calamares y se probará sólo sobre discos `qcow2` desechables.

> GRUB inicia un entorno live; Calamares recoge y confirma toda decisión de instalación posterior.

## Condiciones antes de construir

| Puerta | Evidencia requerida |
| --- | --- |
| Perfil | `plasma-lab.conf` debe conservar un conjunto de paquetes Plasma trazable y separado de RC1. |
| Raíz | El perfil debe generarse fuera de la salida de RC1; se rechaza reutilizar su artefacto o su squashfs. |
| Calamares | El runtime debe conservar la verificación PGP reproducible; la configuración sólo puede pasar de referencia a activa tras registrar el squashfs y checksum de la ISO nueva. |
| Discos | El preflight debe aceptar exclusivamente ISO nueva y unidades `qcow2`; no se permiten discos físicos del host. |
| Firmware | BIOS y UEFI se validan por separado antes de convertir cualquier artefacto de laboratorio en candidato de release. |

## Estado

Este documento no modifica `grub.cfg`, no crea una ISO, no instala Calamares en RC1 y no activa `settings.conf`. El esqueleto se limita a `plasma-lab` y permanece fuera del perfil Plasma RC1.

## Validación del esqueleto

El perfil `plasma-lab` se generó en un directorio temporal sin ejecutar `mkarchiso`. Conservó la identidad `influent-danenone-plasma-lab`, instaló la configuración SDDM y eliminó el `greetd` heredado. La raíz temporal no contenía `/etc/calamares`; se eliminó al terminar la comprobación.

No existe una plantilla de selector dual. Antes de construir el único medio Plasma Lab deben existir un squashfs nuevo, su checksum y el `archisobasedir` correspondiente; las pruebas se limitarán a `qcow2` en BIOS y UEFI.

## Referencias

[1]: https://calamares.codeberg.page/docs/deploy-configuration/ "Calamares — Configuration"
