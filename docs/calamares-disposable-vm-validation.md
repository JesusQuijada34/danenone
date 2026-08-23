# Protocolo de validación desechable de Calamares

## Propósito y límite

Este protocolo prepara las pruebas de una futura edición Danenone con Calamares. **No autoriza instalar Calamares en la ISO Plasma RC1**, no modifica su release publicada y no se ejecuta contra discos físicos. La candidata Plasma continúa con SDDM y Plasma/KWin como sesión predeterminada; Hyprland permanece como sesión avanzada.

La configuración de producción de Calamares debe vivir bajo `/etc/calamares`, separada de los ejemplos del runtime. La plantilla actual se instala deliberadamente bajo `/usr/share/danenone/calamares-template/etc/calamares`, por lo que no puede ser descubierta ni ejecutada por Calamares sin una operación explícita posterior. [1]

## Activos que deben coincidir

| Activo | Versión o ubicación | Condición previa para la VM |
| --- | --- | --- |
| Runtime | `danenone-calamares` 3.4.2-2 | Recompilado con `makepkg` y clave PGP confiada por su keyring, sin `--skippgpcheck`. |
| Plantilla | `danenone-calamares-config` 0.1.0-1 | Inspeccionada para confirmar que sólo contiene `/usr/share/danenone/calamares-template/etc/calamares`. |
| Configuración ejecutable | Futuro árbol `/etc/calamares` | Debe declarar módulos auditados para `mount`, `unpackfs`, `fstab`, `locale`, `users`, `displaymanager` y `bootloader`; nunca se deriva de forma automática de la plantilla pasiva. |
| Medio de prueba | Nueva ISO de laboratorio sin publicar | Debe ser distinta de `v0.5.0-plasma-rc1` y llevar su propio checksum. |
| Destino | Imagen QEMU `qcow2` creada para la prueba | Ningún dispositivo `/dev/sd*`, `/dev/nvme*`, USB o disco del host es admisible. |

> La plantilla existente declara `exec: []`. Por tanto, hoy sólo permite validar el paquete, el branding y las pantallas; no es un instalador funcional ni una autorización para realizar cambios de disco.

## Barreras antes del arranque

La persona que ejecute la prueba debe crear una carpeta temporal nueva, dentro de `build/` u otra ruta ignorada por Git, y usar imágenes `qcow2` que estén bajo esa carpeta. Antes de abrir QEMU, debe registrar la ruta absoluta de la imagen y rechazar la ejecución si no termina en `.qcow2` o si se resuelve fuera de la carpeta temporal. El comando de prueba no debe aceptar argumentos que representen dispositivos de bloque.

La futura ISO de laboratorio debe incorporar el runtime y una copia explícita de una configuración **completa** bajo `/etc/calamares`. La copia sólo se permite cuando los módulos ejecutores tengan sus configuraciones versionadas, revisadas y probadas. El handoff de OOBE puede proponer únicamente `SELECTED_LANGUAGE` y `SELECTED_EDITION`; Calamares debe pedir y confirmar el disco, el modo de particionado y la contraseña dentro de su propia interfaz. [1]

### Validador de precondiciones de laboratorio

Un futuro script `scripts/validate_calamares_lab.py` validará únicamente argumentos y rutas; no ejecutará QEMU, Calamares, `pacman`, operaciones de particionado ni cambios de archivos. Deberá aceptar un directorio de trabajo explícito, una ISO de laboratorio existente dentro de ese directorio, uno o más discos virtuales `qcow2` existentes dentro del mismo árbol y el SHA-256 esperado de la ISO.

| Entrada | Aceptación | Rechazo obligatorio |
| --- | --- | --- |
| Directorio de trabajo | Ruta existente, canónica y controlada para pruebas. | Ruta relativa, inexistente o fuera del espacio de pruebas indicado. |
| ISO | Archivo `.iso` regular bajo el directorio de trabajo, con SHA-256 esperado coincidente. | La RC1 `v0.5.0-plasma-rc1`, su SHA-256 conocido, un enlace fuera del árbol o un checksum distinto. |
| Disco | Uno o más archivos `.qcow2` regulares bajo el directorio de trabajo. | `/dev/*`, una ruta que escape el árbol, extensión distinta o archivo inexistente. |

La salida permitida es un resumen de las rutas validadas y del checksum observado. El validador nunca construye medios, no crea discos y no inicia la matriz de instalación descrita después.

Cuando exista una ISO de laboratorio y sus discos ya creados, la comprobación se ejecutará de forma explícita, por ejemplo:

```bash
python3 scripts/validate_calamares_lab.py \
  --workspace /ruta/aislada/de/laboratorio \
  --iso /ruta/aislada/de/laboratorio/influent-danenone-calamares-lab.iso \
  --disk /ruta/aislada/de/laboratorio/target.qcow2 \
  --expect-iso-sha256 <sha256-registrado-de-la-iso>
```

El código de salida `0` sólo confirma la precondición de rutas y checksum. No es una autorización para ejecutar QEMU, copiar la plantilla a `/etc/calamares`, instalar en discos reales ni publicar una ISO. Cualquier resultado distinto de `0` debe bloquear la prueba posterior y corregirse antes de volver a evaluar las entradas.

| Comprobación previa | Resultado obligatorio | Motivo de detención |
| --- | --- | --- |
| Integridad del runtime | Checksum y firma PGP válidos dentro del flujo de `makepkg`. | No existe una firma confiada o se usó una omisión de firma. |
| Aislamiento del medio | ISO de laboratorio nueva, checksum registrado y distinta de RC1. | Se intenta reutilizar o reemplazar `v0.5.0-plasma-rc1`. |
| Aislamiento de destino | Sólo una imagen `qcow2` nueva, sin discos del host adjuntos. | La ruta identifica un dispositivo de bloque o un archivo ajeno al directorio de pruebas. |
| Configuración | `settings.conf` con etapas de ejecución completas y módulos auditados. | Permanece `exec: []`, falta un módulo o se pretende ejecutar shell genérico. |
| Privacidad | Handoff limitado a idioma/edición y sin servicios de telemetría/red. | Aparecen claves, contraseñas, identificadores de hardware o decisiones destructivas heredadas. |

## Matriz de pruebas en VM

Las pruebas se ejecutan sobre imágenes creadas para cada caso y se destruyen al finalizar. QEMU sin KVM puede tardar varios minutos en alcanzar la sesión gráfica; una captura negra temprana no es evidencia suficiente de error. Se deben conservar únicamente registros de instalación que hayan sido revisados para no contener contraseñas, tokens o identificadores sensibles.

| Caso | Discos virtuales | Acción | Criterio de aceptación |
| --- | --- | --- | --- |
| Cancelación segura | Un `qcow2` vacío | Avanzar hasta el resumen y cancelar. | No se crean particiones, cuentas ni ficheros de destino. |
| Instalación guiada limpia | Un `qcow2` vacío de 32 GiB o mayor | Seleccionar borrado completo y confirmar dentro de Calamares. | El arranque posterior muestra SDDM con Plasma preseleccionado y Hyprland disponible. |
| Particionado manual | Un `qcow2` vacío con particiones de prueba | Definir puntos de montaje manuales y revisar el resumen. | Sólo se modifican las particiones explícitamente seleccionadas en la imagen virtual. |
| Instalación dual | Dos `qcow2`: sistema señuelo y destino de prueba | Probar detección y selección de destino. | No se modifica el sistema señuelo sin confirmación inequívoca; el cargador enumera los sistemas esperados. |
| Error recuperable | Un `qcow2` con espacio insuficiente o fallo inyectado | Forzar el fallo antes de finalizar. | Calamares informa el fallo sin secretos, ofrece salida segura y no declara éxito. |

## Evidencia mínima y decisión

Cada prueba debe archivar, fuera de Git, el checksum de la ISO de laboratorio, el hash de los paquetes, la ruta de los `qcow2`, la configuración de firmware usada (BIOS/UEFI), la ruta de prueba, el resultado y los registros saneados. Las capturas deben tomarse tras alcanzar la pantalla relevante, no durante el arranque temprano de QEMU.

La integración en una futura edición sólo puede continuar cuando la matriz se complete en ambos firmwares necesarios, los resultados sean reproducibles y se confirme el estado final de SDDM/Plasma/Hyprland. La RC1 existente nunca se reemplaza: una ISO nueva requerirá una versión, checksum, validación y publicación independientes.

## Estado de comprobaciones de esta preparación

La plantilla se construyó como paquete aislado, su estructura se validó con pruebas automatizadas y `qmllint` de Qt6 no devolvió diagnósticos para `show.qml`. También se ejecutó `qmlscene` con `QT_QPA_PLATFORM=offscreen` y backend de software durante cinco segundos: el proceso permaneció activo hasta el límite, sin escribir errores QML. Este resultado verifica que la escena persistente puede cargarse en el entorno aislado; no sustituye una revisión visual dentro de Calamares.

Como preflight de disposición de archivos, ambos paquetes se instalaron con `pacman` bajo un root temporal `tmpfs` dentro del chroot. El runtime quedó en `/usr/bin/calamares`; la plantilla quedó en `/usr/share/danenone/calamares-template/etc/calamares/settings.conf`, mantuvo `exec: []` y no creó `/etc/calamares`. El root temporal se desmontó al concluir y las bases de datos del chroot y de Plasma RC1 no recibieron esos paquetes. Esta comprobación deliberadamente omitió dependencias porque el root de laboratorio no posee repositorios ni bibliotecas runtime; valida rutas de paquete, no una ejecución de Calamares.

Persisten dos bloqueos deliberados. La plantilla declara `exec: []`; aún no existen configuraciones auditadas para los módulos que escriben en el disco. El runtime ya fue recompilado después de validar checksum y firma PGP con un keyring temporal de `makepkg`, sin omitir la comprobación de firma. Por último, no existe una ISO de laboratorio ni un conjunto de `qcow2` para la matriz anterior. Mientras alguno de esos bloqueos siga abierto, la configuración no debe copiarse a `/etc/calamares`, añadirse a `plasma.packages` ni aparecer en una ISO publicada.

## Referencias

[1]: https://calamares.codeberg.page/docs/deploy-configuration/ "Calamares Configuration"
