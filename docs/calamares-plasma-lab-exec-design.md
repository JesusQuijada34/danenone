# Configuración ejecutable de Calamares para Plasma Lab

## Alcance

La configuración ejecutable se generará **solamente** dentro del perfil temporal `plasma-lab`. Nunca se escribirá en `archiso-profile/airootfs/etc/calamares`, la plantilla pasiva empaquetada ni la ISO Plasma RC1. Su propósito es preparar una ISO de laboratorio distinta que, tras preflight, pueda ensayar una instalación sobre un disco `qcow2` desechable.

## Barreras obligatorias

| Barrera | Regla | Resultado ante incumplimiento |
| --- | --- | --- |
| Edición | El generador acepta exclusivamente `plasma-lab`. | Finaliza sin escribir archivos. |
| Consentimiento de build | Requiere `DANENONE_CALAMARES_LAB_ENABLE=qcow2-only`. | Finaliza sin escribir archivos. |
| Perfil generado | Sólo puede escribir bajo `<perfil-temporal>/airootfs/etc/calamares`. | Rechaza rutas fuera del perfil generado. |
| Fuente live | `unpackfs` sólo referencia `/run/archiso/bootmnt/influent/x86_64/airootfs.sfs`, documentada desde el initramfs y los parámetros de RC1 como referencia de layout. | Nunca usa la ISO RC1, su checksum ni un path de host como fuente. |
| Medio de prueba | El validador externo debe aceptar una ISO nueva y un disco `qcow2` antes de iniciar QEMU. | No se permite un disco físico ni una ISO RC1. |
| Módulos | La secuencia se limita a `partition`, `mount`, `unpackfs`, `fstab`, `locale`, `users`, `displaymanager`, `bootloader` y `umount`. | Se rechazan `shellprocess`, `contextualprocess` y `webview`. |
| Decisiones sensibles | El particionado permanece interactivo en Calamares; el OOBE no aporta disco, contraseña, red ni secretos. | No se generan flags destructivos ni automatización de borrado. |

La configuración conserva `prompt-install: true`. Las pantallas de particionado, usuario y resumen permanecen visibles; ninguna preferencia de OOBE se convierte en una orden de instalación. El handoff sólo puede aportar idioma y edición previamente validados.

> El prefijo `qcow2-only` protege el proceso de construcción. La seguridad de ejecución se completa con el preflight del host y una máquina QEMU sin dispositivos físicos adjuntos.

## Resultado esperado del generador

El generador copiará las opciones de branding ya auditadas y escribirá `settings.conf` más las configuraciones específicas de módulo únicamente en el perfil Plasma Lab temporal. El runtime y la plantilla pasiva entran al rootfs mediante el repositorio local reproducible; el generador agrega la configuración activa sólo cuando todas las barreras están presentes.

Antes de producir una ISO, se validará la estructura YAML, que no existan módulos prohibidos, que la fuente live sea relativa al entorno Archiso y que ninguna ruta activa se filtre a RC1.
