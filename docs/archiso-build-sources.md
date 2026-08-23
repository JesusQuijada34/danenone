# Fuentes y base de construcción Archiso

La compilación de la ISO usa un perfil Archiso personalizado y añade Foundstore como contenido de `airootfs`, siguiendo el modelo documentado para archivos y paquetes de una imagen live. La herramienta principal es `mkarchiso`; el perfil incluye `packages.x86_64`, `profiledef.sh` y `airootfs`.

> Archiso se usa para construir imágenes live de Arch Linux y su componente central es `mkarchiso`. Los paquetes se declaran en `packages.x86_64` y los archivos propios se colocan bajo `airootfs`.

La imagen bootstrap `archlinux-bootstrap-2026.08.01-x86_64.tar.zst` se descargó por HTTPS, se verificó contra el SHA-256 publicado y se comprobó su firma OpenPGP de Pierre Schmitz (fingerprint `3E80 CA1A 8B89 F69C BA57 D98A 76A5 EF90 5444 9A5C`) antes de instalar `archiso` en un chroot aislado.

El perfil preparado integra el asset `Influent.foundstore.v1.2-26.08-22.20-Danenone.iflapp` después de verificar su SHA-256 y su estructura ZIP. La ISO creada se inspeccionó extrayendo `airootfs.sfs`: contiene el artefacto, el ejecutable de la aplicación, el lanzador `/usr/local/bin/foundstore` y la entrada FreeDesktop correspondiente.

## Referencias

1. [ArchWiki: archiso](https://wiki.archlinux.org/title/Archiso)
2. [ArchWiki: instalar Arch desde otro Linux](https://wiki.archlinux.org/title/Install_Arch_Linux_from_existing_Linux)
3. [Descargas oficiales de Arch Linux, release 2026.08.01](https://archlinux.org/download/)
