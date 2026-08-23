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

## Criterios antes de activar Calamares

1. Empaquetar Calamares y su configuración en una fuente reproducible y auditable.
2. Ejecutar una instalación completa en una máquina virtual desechable con discos de prueba.
3. Probar rutas de instalación manual, automática y dual boot sin reutilizar código del OOBE para operar particiones.
4. Confirmar que Plasma queda predeterminado y Hyprland se mantiene como sesión avanzada tras la instalación.
5. Probar el rechazo de entradas no válidas, la ausencia de secretos en registros y la restauración ante cancelaciones.

## Referencias

[1]: https://calamares.euroquis.nl/docs/deploy-configuration "Calamares — Configuration"
