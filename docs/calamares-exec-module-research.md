# Investigación de módulos ejecutores Calamares

## Alcance

Este documento fija las interfaces y restricciones para una futura configuración ejecutable de Calamares. No incorpora módulos a `settings.conf`, no crea archivos bajo `/etc/calamares` y no modifica las listas de paquetes de Plasma. La plantilla `danenone-calamares-config` continúa con `exec: []` y sólo sirve de base visual y estructural.

## Hallazgos de las fuentes oficiales

Calamares separa el recorrido visible (`show`) del trabajo de instalación (`exec`). Un módulo de vista como `partition`, `locale`, `keyboard` o `users` sólo encola sus tareas si aparece también en la fase `exec` que sigue a la fase `show`; los módulos de trabajo se ejecutan en el orden declarado. [1] La configuración de distribución se debe empaquetar separada del runtime y colocarse bajo `/etc/calamares`, dejando los ejemplos de `/usr/share/calamares` sin modificar. [1]

Los módulos oficiales declaran dependencias y comparten únicamente datos de instalación a través de *global storage*, como la ruta objetivo, las particiones y el usuario. Un fallo durante `exec` aborta la instalación y permite invocar sólo módulos de emergencia de la misma fase. [2] Estas propiedades permiten auditar el orden antes de escribir configuraciones específicas de Danenone.

## Contrato de una futura fase de ejecución

| Módulo | Papel en la fase `exec` | Requisito de configuración | Restricción Danenone |
| --- | --- | --- | --- |
| `partition` | Aplica exclusivamente el esquema confirmado en su propia interfaz. | `partition.conf` con opción inicial `none` y sin selección destructiva predefinida. | No recibe discos ni opciones de borrado del OOBE. |
| `mount` | Monta el destino y los recursos necesarios para el instalador. | `mount.conf` explícito, con puntos de montaje y límites para UEFI. | Se prueba sólo con `qcow2`; no se introducen montajes del host ajenos a los ejemplos oficiales. |
| `unpackfs` | Copia el sistema live al destino. | `unpackfs.conf` que nombre un artefacto versionado y verificado de la ISO de laboratorio. | No se escribe hasta definir y verificar el origen de sistema de Archiso. |
| `fstab` | Genera la tabla de sistemas de archivos del destino. | `fstab.conf` coherente con los montajes creados. | No debe asumir UUID, disco o subvolúmenes externos a la elección confirmada. |
| `locale`, `keyboard`, `localecfg` | Aplica idioma, zona horaria y teclado dentro del destino. | Archivos de configuración locales y sin GeoIP. | El idioma del handoff sólo es una sugerencia validada y editable. |
| `users` | Crea la cuenta solicitada en el destino. | `users.conf` con nombres prohibidos, contraseña introducida en Calamares y grupos necesarios. | Nunca importa contraseñas, tokens ni identidad DaneDesk del OOBE. |
| `displaymanager` | Habilita el gestor de inicio de sesión en el destino. | `displaymanager.conf` que seleccione únicamente `sddm`. | Debe conservar Plasma/KWin como sesión predeterminada y Hyprland como alternativa avanzada. |
| `bootloader` | Instala y configura GRUB en el destino confirmado. | `bootloader.conf` con ejecutables y ruta de GRUB explícitos. | BIOS y UEFI se validan por separado sobre VM; no hay fallback de edición automática de configuración. |
| `umount` | Desmonta el destino tras éxito o fallo. | `umount.conf` marcado como emergencia. | Debe cerrar los montajes de VM incluso después de un error. |

## Controles prohibidos

No se usarán `shellprocess`, `contextualprocess`, módulos de proceso externos, `webview`, telemetría, GeoIP ni scripts con escalada de privilegios genérica para suplir módulos oficiales. La documentación de Calamares advierte que los módulos de proceso externos no son recomendables; el proyecto debe preferir módulos oficiales y su configuración explícita. [2]

La edición no debe incluir `unpackfs.conf` hasta que el origen de la copia sea un recurso de ISO de laboratorio con checksum registrado. Tampoco debe incluir `bootloader.conf` activo hasta probar el estado de SDDM, Plasma y Hyprland después de la instalación en BIOS y UEFI.

## Muestra de configuración usada como referencia

El árbol de fuentes de Calamares 3.4.2 construido de manera aislada proporciona ejemplos para `partition`, `mount`, `unpackfs`, `fstab`, `locale`, `users`, `displaymanager`, `bootloader` y `umount`. Entre las opciones relevantes, el ejemplo de particionado parte de `initialPartitioningChoice: none`, el módulo `users` permite rechazar nombres reservados y el de desmontaje se marca como emergencia. Estos ejemplos son referencias de interfaz, no archivos que se copien sin revisión para Danenone.

Una inspección de solo lectura de la ISO RC1 confirmó que su squashfs reside en `/influent/x86_64/airootfs.sfs` y que incluye el checksum vecino `airootfs.sha512`. Esta ruta describe exclusivamente el medio ya publicado: no debe añadirse a `unpackfs.conf`, porque la RC1 no incorpora Calamares. Cuando exista una ISO de laboratorio se tendrá que comprobar de nuevo su ruta, registrar su checksum y configurar `unpackfs` contra ese nuevo artefacto verificado.

## Sintaxis contrastada para referencias aisladas

Los ejemplos oficiales de `unpackfs` requieren una lista `unpack` cuyos elementos tengan `source`, `sourcefs` y `destination`; `sourcefs: squashfs` es válido, pero el origen debe ser un artefacto del medio de laboratorio. La futura referencia mantendrá ese origen como un marcador explícito no ejecutable y no reutilizará el squashfs de RC1. [3]

El módulo `mount` usa únicamente `extraMounts` para recursos necesarios dentro del destino, como `proc`, `sysfs`, `/dev` y `/run`, y aplica opciones por tipo de sistema de archivos mediante `mountOptions`. No necesita ni debe contener rutas de discos elegidas fuera de la interfaz de `partition`. `fstab` parte de los puntos de montaje que ya haya establecido ese módulo y permite fijar opciones temporales sin codificar UUID ni nombres de dispositivos. [4] [5]

`displaymanager` admite una lista de gestores y permite definir el escritorio predeterminado mediante las claves obligatorias `executable` y `desktopFile`. La referencia Danenone limitará la lista a `sddm`, declarará Plasma como predeterminado sólo tras comprobar los nombres de archivo instalados en la ISO de laboratorio y mantendrá `basicSetup: false`. [6] Para `users`, se conservarán las claves de grupos, cuenta root, autologin, requisitos de contraseña y nombres prohibidos, pero las contraseñas seguirán siendo valores recolectados exclusivamente por Calamares y nunca aparecerán en archivos de referencia. [7]

El ejemplo de `bootloader` admite `efiBootLoader: "grub"`, rutas de los ejecutables GRUB, `efiBootloaderId`, `installEFIFallback` e `installHybridGRUB`. Los valores definitivos no se fijarán hasta validar una ISO de laboratorio con firmware BIOS y UEFI separados. [8] El ejemplo de `locale` habilita GeoIP por defecto, por lo que la referencia Danenone lo desactivará de manera explícita y elegirá una zona inicial local sin efectuar solicitudes de red. [9]

## Diseño de referencias no activables

Las referencias se colocarán en `packaging/calamares-config/reference-modules/`. Sus ficheros usarán el sufijo `.conf.reference`, no el nombre que Calamares busca en `/etc/calamares/modules`, y el paquete `danenone-calamares-config` no los copiará bajo `/usr/share/danenone/calamares-template`. Por tanto, instalar el paquete no crea archivos de módulo ejecutables ni una secuencia `exec` nueva.

| Referencia | Decisión de diseño | Barrera antes de activarla |
| --- | --- | --- |
| `unpackfs.conf.reference` | Sólo contendrá una asignación comentada y un `unpack: []` activo. | Sustituir el origen por el squashfs de una ISO de laboratorio y validar su checksum. |
| `mount.conf.reference` y `fstab.conf.reference` | Declararán puntos de montaje estándar y opciones sin UUID ni discos. | Confirmar el mapa de `partition` en un `qcow2`. |
| `locale.conf.reference` | Desactivará GeoIP y no declarará URL de red. | Verificar locales disponibles en la ISO de laboratorio. |
| `users.conf.reference` | Usará nombre de cuenta, grupos y política de contraseña; no contiene secretos. | Verificar usuarios/grupos reales del destino y completar el flujo en Calamares. |
| `displaymanager.conf.reference` | Restringirá el gestor a SDDM y mantendrá Plasma como candidato predeterminado. | Confirmar binario y archivo `.desktop` después de instalar en VM. |
| `bootloader.conf.reference` | Limitado a GRUB y rutas Arch conocidas, sin tocar el medio live. | Validar BIOS y UEFI en discos virtuales. |

No se creará un `settings.conf` ejecutable de referencia. El orden futuro se describe en `execution-sequence.yaml`, que no es un nombre de configuración consumido por Calamares. La transición a archivos `settings.conf` y `modules/*.conf` reales queda bloqueada por el protocolo de VM, el validador de preflight y pruebas de instalación satisfactorias.

## Estado validado de las referencias

Las referencias se revisaron con pruebas automatizadas que exigen el sufijo `.conf.reference`, rechazan cualquier `settings.conf` dentro del árbol y comprueban la ausencia de `shellprocess`, `contextualprocess` y `webview`. El paquete `danenone-calamares-config` se reconstruyó de forma aislada e inspeccionó con `pacman -Qlp`: no contiene `reference-modules`, sólo la plantilla pasiva bajo `/usr/share/danenone/calamares-template/etc/calamares`. Por ello, este hito no instala módulos ejecutables ni modifica la ISO Plasma RC1.

El runtime recompilado `danenone-calamares 3.4.2-2` se auditó de forma pasiva con `pacman -Qlp`. Contiene los trece módulos declarados por la secuencia de referencia: `welcome`, `locale`, `keyboard`, `partition`, `users`, `summary`, `mount`, `unpackfs`, `fstab`, `displaymanager`, `bootloader`, `umount` y `finished`. El auditor rechaza que la secuencia incluya `shellprocess`, `contextualprocess` o `webview`, y no instala ni ejecuta el runtime. Esta presencia reduce un bloqueo de empaquetado, pero no elimina los bloqueos de configuración real, ISO de laboratorio y matriz de instalación en `qcow2`.

## Próximo paso seguro

Antes de crear una configuración ejecutora aislada se debe describir el layout exacto de la ISO de laboratorio, el origen de la copia y los comandos de GRUB disponibles dentro del destino. Hasta que esas precondiciones estén verificadas, los módulos permanecen en este contrato documental y la plantilla pasiva no cambia.

## Referencias

[1]: https://calamares.codeberg.page/docs/deploy-configuration/ "Calamares — Configuration"
[2]: https://github.com/calamares/calamares/blob/calamares/src/modules/README.md "Calamares modules documentation"
[3]: https://raw.githubusercontent.com/calamares/calamares/master/src/modules/unpackfs/unpackfs.conf "Calamares unpackfs.conf"
[4]: https://raw.githubusercontent.com/calamares/calamares/master/src/modules/mount/mount.conf "Calamares mount.conf"
[5]: https://raw.githubusercontent.com/calamares/calamares/master/src/modules/fstab/fstab.conf "Calamares fstab.conf"
[6]: https://raw.githubusercontent.com/calamares/calamares/master/src/modules/displaymanager/displaymanager.conf "Calamares displaymanager.conf"
[7]: https://raw.githubusercontent.com/calamares/calamares/master/src/modules/users/users.conf "Calamares users.conf"
[8]: https://raw.githubusercontent.com/calamares/calamares/master/src/modules/bootloader/bootloader.conf "Calamares bootloader.conf"
[9]: https://raw.githubusercontent.com/calamares/calamares/master/src/modules/locale/locale.conf "Calamares locale.conf"
