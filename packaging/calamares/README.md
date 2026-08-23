# Paquete Calamares para una futura edición Danenone

Este directorio prepara una fuente **reproducible y aislada** de Calamares para Danenone. No se incluye en `packages/editions/plasma.packages`, no se copia al perfil Archiso y no afecta el release publicado `v0.5.0-plasma-rc1`.

El PKGBUILD fija la release oficial `3.4.2`, su SHA-256 publicado y la huella de firma indicada en la release de Calamares. La construcción selecciona Qt6, QML y systemd, pero deja `INSTALL_CONFIG=OFF`: la configuración y el branding de Danenone deben distribuirse como un paquete separado bajo `/etc/calamares`, tal como recomienda la documentación del proyecto. [1] [2]

## Revisión obligatoria antes de compilar

1. Importar y verificar la clave `6D0837841C068A233F24127B14B6CC381BC256D6` desde una fuente de confianza antes de aceptar la firma.
2. Construir en un contenedor Arch desechable o en el entorno Archiso, nunca en una instalación de usuario.
3. Ejecutar `makepkg --verifysource` antes de compilar y revisar que coincida el SHA-256 fijado.
4. Crear un paquete independiente de configuración y branding; no modificar los ejemplos que distribuye Calamares.
5. Probar instalaciones automáticas, manuales y dual boot con discos de prueba antes de añadir el paquete a una ISO.

## Alcance intencionalmente excluido

No se ejecuta `deploycala.py`, ya que la guía oficial indica que ese script escribe directamente en rutas del sistema y no es un mecanismo de despliegue permanente. Tampoco se incorporan contraseñas, secretos de red ni decisiones de particionado procedentes del OOBE.

## Referencias

[1]: https://codeberg.org/Calamares/calamares/releases/tag/v3.4.2 "Calamares 3.4.2 release"
[2]: https://calamares.codeberg.page/docs/develop-guide "Calamares Deployer's Guide"
