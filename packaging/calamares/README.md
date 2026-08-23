# Paquete Calamares para una futura edición Danenone

Este directorio prepara una fuente **reproducible y aislada** de Calamares para Danenone. No se incluye en `packages/editions/plasma.packages`, no se copia al perfil Archiso y no afecta el release publicado `v0.5.0-plasma-rc1`.

El PKGBUILD fija la release oficial `3.4.2`, su SHA-256 publicado y la huella de firma indicada en la release de Calamares. La construcción selecciona Qt6, QML y systemd, pero deja `INSTALL_CONFIG=OFF`: la configuración y el branding de Danenone deben distribuirse como un paquete separado bajo `/etc/calamares`, tal como recomienda la documentación del proyecto. [1] [2]

## Estado de validación aislada

La receta se compiló correctamente en un chroot Arch de prueba como `danenone-calamares 3.4.2-2`. El paquete resultante contiene el ejecutable, la política Polkit y los recursos de branding de ejemplo, y no fue instalado ni añadido a la edición Plasma RC1. Durante esta validación se incorporó `qt6-tools` como dependencia de construcción porque `Qt6LinguistTools` es requerido por el CMake de Calamares.

El tarball se comprobó frente al SHA-256 publicado. La firma PGP también fue validada en un anillo temporal: la clave se descargó desde el sitio personal publicado por el firmante, su huella primaria se contrastó con la actualización GPG de KDE y la firma del tarball verificó correctamente con el subkey `6D0837841C068A233F24127B14B6CC381BC256D6`. [3] [4]

La clave pública mínima verificada se versiona bajo `keys/`. El helper `scripts/verify-source.sh` comprueba su SHA-256, crea un keyring temporal, importa la clave, contrasta la huella primaria y la subkey, y llama a `makepkg --verifysource`. Esta ruta se ejecutó correctamente en el chroot Arch con los pseudo-dispositivos privados montados; el checksum del tarball y la firma PGP aprobaron sin `--skippgpcheck`.

Después de esa verificación, el runtime se recompiló en el mismo chroot como `danenone-calamares 3.4.2-2`. La inspección del paquete confirmó `/usr/bin/calamares`, la política Polkit y el branding de ejemplo, sin archivos bajo `/etc/calamares`. El paquete es un resultado local de validación y no se versiona, instala ni añade a la ISO Plasma RC1.

## Revisión obligatoria antes de compilar

1. Importar y verificar la clave `6D0837841C068A233F24127B14B6CC381BC256D6` desde una fuente de confianza antes de aceptar la firma.
2. Ejecutar `scripts/verify-source.sh` desde este directorio. El helper valida el checksum de la clave pública versionada, importa la clave en un `GNUPGHOME` temporal, comprueba la huella primaria y subkey de firma, y finalmente llama a `makepkg --verifysource`.
3. Construir en un contenedor Arch desechable o en el entorno Archiso, nunca en una instalación de usuario.
4. Ejecutar una compilación normal sólo después de que la verificación de fuentes haya sido correcta y revisar que coincida el SHA-256 fijado.
5. Crear un paquete independiente de configuración y branding; no modificar los ejemplos que distribuye Calamares.
6. Probar instalaciones automáticas, manuales y dual boot con discos de prueba antes de añadir el paquete a una ISO.

## Alcance intencionalmente excluido

No se ejecuta `deploycala.py`, ya que la guía oficial indica que ese script escribe directamente en rutas del sistema y no es un mecanismo de despliegue permanente. Tampoco se incorporan contraseñas, secretos de red ni decisiones de particionado procedentes del OOBE.

## Referencias

[1]: https://codeberg.org/Calamares/calamares/releases/tag/v3.4.2 "Calamares 3.4.2 release"
[2]: https://calamares.codeberg.page/docs/develop-guide "Calamares Deployer's Guide"
[3]: https://euroquis.nl/about/ "Adriaan de Groot — clave pública"
[4]: https://planet.kde.org/adriaan-de-groot-2026-02-02-gpg-update-2026/ "GPG Update 2026"
