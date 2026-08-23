# Transición preparada: OOBE Danenone hacia Calamares/QML

## Estado y límite de esta iteración

La edición Plasma RC1 conserva su OOBE GTK4 y no incorpora Calamares. La comprobación en el entorno Arch de construcción no encontró un paquete `calamares` en los repositorios configurados, por lo que añadirlo ahora rompería la reproducibilidad de la ISO. Esta iteración establece únicamente un contrato de transición y un *handoff* seguro, preparado para una futura integración empaquetada.

La configuración de producción de Calamares debe distribuirse separada del binario del instalador, bajo `/etc/calamares`; `settings.conf` define los módulos y separa la secuencia visible de la secuencia ejecutora. [1]

## Separación de responsabilidades

| Componente | Responsabilidad actual | Responsabilidad futura |
|---|---|---|
| OOBE GTK4 | Recoge idioma, edición, red y apariencia sin formatear discos. | Conserva decisiones no destructivas y las exporta mediante un contrato mínimo. |
| Handoff del instalador | Valida y expone idioma y edición permitidos. | Entrega únicamente metadatos seguros a la sesión del instalador. |
| Calamares/QML | No se instala en Plasma RC1. | Gestiona particionado, usuarios, paquete de destino, copia del sistema, arranque y confirmaciones destructivas. |
| Branding Plasma | Identidad verdypor y recursos del escritorio. | Se instala como branding de Calamares sin copiar la identidad de otras plataformas. |

## Contrato de handoff

El contrato utiliza un archivo temporal con permisos `0600` y sólo permite los campos `SELECTED_LANGUAGE` y `SELECTED_EDITION`. No incluye contraseñas, tokens, redes Wi-Fi, identificadores DaneDesk, decisiones de particionado ni confirmaciones de borrado. Esas decisiones deben ser solicitadas, validadas y confirmadas dentro del instalador.

Los valores aceptados son las cinco ediciones actuales (`home`, `enterprise`, `developer`, `minimal`, `frozen-lab`) y un código de idioma validado frente al manifiesto local. El helper falla de forma cerrada ante valores no permitidos.

## Integración futura

Cuando exista un paquete de Calamares reproducible y auditado, la edición Plasma añadirá una configuración separada en `/etc/calamares` con las siguientes etapas: locale, teclado, usuarios, selección de edición, particionado, resumen y confirmación; después, copia al destino, aplicación de la edición, configuración de SDDM, cargador de arranque y pantalla final. La ejecución de comandos deberá usar módulos específicos y con manejo de errores, no scripts genéricos con privilegios amplios. [1]

El branding se ubicará como componente propio y el lanzador conservará el tema Plasma mediante un entorno controlado. La documentación oficial advierte que los entornos gráficos no se heredan automáticamente cuando Calamares se lanza con privilegios; esa integración debe quedar explícita y comprobada. [1]

## Artefactos aislados verificados

El repositorio ya contiene el paquete de runtime `danenone-calamares` y la plantilla `danenone-calamares-config`. La plantilla instala sus archivos exclusivamente bajo `/usr/share/danenone/calamares-template/etc/calamares`; no crea `/etc/calamares`, no incorpora un lanzador y no aparece en la lista de paquetes de Plasma RC1. Su `settings.conf` conserva sólo pantallas no destructivas y una etapa `exec` vacía.

El slideshow QML se comprobó con `qmllint` de Qt6 sin diagnósticos. El paquete de plantilla se construyó en el chroot Arch aislado y las pruebas de estructura, empaquetado y contrato OOBE aprobaron. Estas comprobaciones demuestran que la **plantilla** es coherente y pasiva; no constituyen una prueba de instalación ni autorizan incorporar Calamares a una ISO.

| Puerta de seguridad | Evidencia requerida | Estado de esta iteración |
|---|---|---|
| Fuente del runtime | Tarball oficial con checksum y firma verificados; clave configurada en el keyring de `makepkg`. | Checksum y firma revisados; falta integrar la confianza de la clave en una compilación de producción. |
| Paquete de configuración | Construcción reproducible y ruta de instalación no activa. | Superada: sólo instala bajo `/usr/share/danenone/calamares-template`. |
| Interfaz QML | Análisis estático sin errores ni avisos de alcance. | Superada con `qmllint` de Qt6. |
| Handoff OOBE | Lista permitida de idioma/edición, permisos `0600` y exclusión de secretos. | Superada por pruebas automatizadas; la plantilla no lo consume aún. |
| Integración en edición | Dependencias, copia explícita a `/etc/calamares`, políticas y lanzador controlados. | Pendiente y prohibida para Plasma RC1. |
| Instalación real | Máquinas virtuales desechables con discos de prueba y recuperación comprobada. | Pendiente. |

Antes de añadir dependencias o copiar la configuración a una futura edición, la compilación deberá verificar la firma PGP desde el keyring de `makepkg`, sin `--skippgpcheck`. Sólo después se implementarán módulos de ejecución específicos, confirmaciones explícitas y pruebas completas sobre discos virtuales desechables. La publicación de una nueva ISO se evaluará como una operación separada y no modifica la release candidate existente.

El procedimiento de aislamiento, la matriz de pruebas y la evidencia mínima se detallan en [el protocolo de validación en VM desechable](calamares-disposable-vm-validation.md).

## Criterios antes de activar Calamares

1. Empaquetar Calamares y su configuración en una fuente reproducible y auditable.
2. Ejecutar una instalación completa en una máquina virtual desechable con discos de prueba.
3. Probar rutas de instalación manual, automática y dual boot sin reutilizar código del OOBE para operar particiones.
4. Confirmar que Plasma queda predeterminado y Hyprland se mantiene como sesión avanzada tras la instalación.
5. Probar el rechazo de entradas no válidas, la ausencia de secretos en registros y la restauración ante cancelaciones.

## Referencias

[1]: https://calamares.euroquis.nl/docs/deploy-configuration "Calamares — Configuration"
