# Protocolo de validación desechable de Calamares

## Propósito y límite

Este protocolo prepara las pruebas de una futura edición Danenone con Calamares. **No autoriza instalar Calamares en la ISO Plasma RC1**, no modifica su release publicada y no se ejecuta contra discos físicos. La candidata Plasma continúa con SDDM y Plasma/KWin como sesión predeterminada; Hyprland permanece como sesión avanzada.

La configuración de producción de Calamares debe vivir bajo `/etc/calamares`, separada de los ejemplos del runtime. La plantilla actual se instala deliberadamente bajo `/usr/share/danenone/calamares-template/etc/calamares`, por lo que no puede ser descubierta ni ejecutada por Calamares sin una operación explícita posterior. [1]

## Activos que deben coincidir

| Activo | Versión o ubicación | Condición previa para la VM |
| --- | --- | --- |
| Runtime | `danenone-calamares` 3.4.2-2 | Recompilado con `makepkg` y clave PGP confiada por su keyring, sin `--skippgpcheck`. |
| Plantilla | `danenone-calamares-config` 0.1.0-1 | Inspeccionada para confirmar que sólo contiene `/usr/share/danenone/calamares-template/etc/calamares`. |
| Configuración ejecutable | Árbol generado en `<perfil-plasma-lab>/airootfs/etc/calamares` | Sólo `configure_calamares_plasma_lab.sh` puede crearlo con `DANENONE_CALAMARES_LAB_ENABLE=qcow2-only`; declara módulos auditados y nunca modifica la plantilla pasiva ni RC1. |
| Medio de prueba | Nueva ISO de laboratorio sin publicar | Debe ser distinta de `v0.5.0-plasma-rc1` y llevar su propio checksum. |
| Destino | Imagen QEMU `qcow2` creada para la prueba | Ningún dispositivo `/dev/sd*`, `/dev/nvme*`, USB o disco del host es admisible. |

> La plantilla existente declara `exec: []`. Por tanto, hoy sólo permite validar el paquete, el branding y las pantallas; no es un instalador funcional ni una autorización para realizar cambios de disco.

## Barreras antes del arranque

La persona que ejecute la prueba debe crear una carpeta temporal nueva, dentro de `build/` u otra ruta ignorada por Git, y usar imágenes `qcow2` que estén bajo esa carpeta. Antes de abrir QEMU, debe registrar la ruta absoluta de la imagen y rechazar la ejecución si no termina en `.qcow2` o si se resuelve fuera de la carpeta temporal. El comando de prueba no debe aceptar argumentos que representen dispositivos de bloque.

La futura ISO de laboratorio debe incorporar el runtime y una configuración **completa** bajo `/etc/calamares`, creada sólo dentro de su perfil temporal. El generador exige la edición `plasma-lab`, el consentimiento de construcción `DANENONE_CALAMARES_LAB_ENABLE=qcow2-only`, un perfil Archiso válido y rechaza sobrescribir una configuración existente. El handoff de OOBE puede proponer únicamente `SELECTED_LANGUAGE` y `SELECTED_EDITION`; Calamares debe pedir y confirmar el disco, el modo de particionado y la contraseña dentro de su propia interfaz. [1]

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

El bloqueo de configuración ejecutable se resolvió de forma limitada y aislada: `scripts/configure_calamares_plasma_lab.sh` crea `settings.conf`, los módulos necesarios y el branding únicamente bajo un perfil temporal `plasma-lab`. Requiere `DANENONE_CALAMARES_LAB_ENABLE=qcow2-only`, no acepta otras ediciones, no sobrescribe un árbol existente y conserva la confirmación de particionado y usuario dentro de Calamares. La fuente `unpackfs` apunta al layout Archiso `/run/archiso/bootmnt/influent/x86_64/airootfs.sfs`; antes de arrancar se debe comprobar que la ISO Lab recién construida conserva ese `install_dir` y registrar su checksum propio.

Una validación estática generó un perfil temporal con el repositorio local verificado de los dos paquetes Calamares. Confirmó que el árbol activo sólo aparece en el perfil temporal, que RC1 y `archiso-profile` siguen sin `/etc/calamares`, y que no se ejecutó `mkarchiso`. La comprobación no instaló, particionó ni arrancó nada.

Persisten los bloqueos materiales: no existe todavía una ISO Plasma Lab, no hay checksum de ese medio y no se han creado discos `qcow2` para las pruebas. Por ello siguen prohibidos el arranque de QEMU, cualquier prueba de particionado y toda publicación. Una vez autorizada la fase material, el orden obligatorio será el siguiente:

### Ejecución Plasma Lab 0.6.0-lab

Tras la autorización, se creó una ISO **separada y no publicada** `influent-danenone-plasma-lab-0.6.0-lab-x86_64.iso`. Su SHA-256 de laboratorio es `c76a9e4dae5c16d58d6fb44ceea979fa46aa28eb7715b9232b3d728c2b997274`. El preflight aceptó ese archivo y dos destinos nuevos de 32 GiB `qcow2`, uno para BIOS y otro para UEFI; no se adjuntó ningún dispositivo de bloque físico.

La inspección estática confirmó que la imagen lleva Foundstore/Fluthin, Plasma/KWin, Hyprland como alternativa, el árbol activo de Calamares y la fuente `unpackfs` propia del layout Archiso. También se actualizó SDDM para declarar `Session=plasma.desktop` y desactivar la retención de la última sesión. Las capturas QEMU con TCG alcanzaron el firmware UEFI y en una ejecución mostraron SDDM, pero las repeticiones BIOS/UEFI posteriores devolvieron framebuffer negro tras ISOLINUX o durante la transición gráfica. Por ello **no se declara validada la sesión por firmware**, no se inicia Calamares, no se particiona y no se publica la ISO. RC1 continúa sin cambios.

| Paso | Acción permitida | Evidencia requerida antes de continuar |
| --- | --- | --- |
| 1 | Reconstruir runtime con `verify-source.sh` y PGP, y reconstruir la plantilla. | Firma y checksum válidos, sin `--skippgpcheck`. |
| 2 | Crear un repositorio temporal con exactamente los dos paquetes y su manifiesto. | `SHA256SUMS` aprobado y base Pacman local. |
| 3 | Generar `plasma-lab` con `DANENONE_CALAMARES_LAB_ENABLE=qcow2-only` y construir una ISO nueva en un directorio ignorado. | ISO distinta de RC1, `install_dir=influent` inspeccionado y SHA-256 registrado. |
| 4 | Crear los destinos virtuales y ejecutar `validate_calamares_lab.py`. | ISO y todos los discos son archivos regulares `.qcow2` dentro del workspace. |
| 5 | Arrancar QEMU en BIOS y UEFI por separado. | Resultados saneados de cancelación, instalación limpia, particionado manual y recuperación de error. |
| 6 | Inspeccionar el sistema instalado. | SDDM inicia `plasma.desktop`, Hyprland permanece como sesión avanzada y GRUB opera en el firmware correspondiente. |

## Referencias

[1]: https://calamares.codeberg.page/docs/deploy-configuration/ "Calamares Configuration"
