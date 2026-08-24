# Fuente de paquetes Calamares para Plasma Lab

## Objetivo

`plasma-lab` necesita consumir únicamente los artefactos locales ya verificados de `danenone-calamares` y `danenone-calamares-config`. El runtime se compila después de ejecutar `makepkg --verifysource` con su keyring temporal; la plantilla sigue pasiva y se instala bajo `/usr/share/danenone/calamares-template`, no bajo `/etc/calamares`.

La construcción de Archiso puede usar un repositorio local definido en el `pacman.conf` del perfil de construcción. ArchWiki indica que ese repositorio debe estar accesible para el proceso `mkarchiso`, que la configuración de construcción no se incorpora automáticamente al entorno live y que los repositorios locales deben declararse antes de los repositorios oficiales si se necesita prioridad. [1] Pacman requiere una base de repositorio generada por `repo-add` y permite una ubicación `file://` en la directiva `Server`. [2]

## Diseño propuesto

| Elemento | Diseño de Plasma Lab | Prohibición |
| --- | --- | --- |
| Paquetes de entrada | Un runtime `danenone-calamares-3.4.2-2-x86_64.pkg.tar.zst` reconstruido tras PGP y una plantilla `danenone-calamares-config-0.1.0-1-any.pkg.tar.zst`. | No se descargan ni se compilan paquetes mientras corre `mkarchiso`. |
| Repositorio temporal | Directorio fuera del repositorio Git, con ambos paquetes, `danenone-lab.db` creado por `repo-add` y un manifiesto SHA-256. | No se versionan paquetes, bases `.db`, cachés ni resultados bajo `build/`. |
| Perfil de construcción | `plasma-lab` recibirá un `pacman.conf` generado que añade `[danenone-lab]` sobre `core` y `extra`, con `Server = file:///ruta/validada`. | No se modifica `archiso-profile/pacman.conf` ni el perfil RC1. |
| Entorno live | El repositorio de construcción no se copia a `airootfs/etc/pacman.conf`; sólo los paquetes quedan instalados en la raíz live de laboratorio. | No se concede al medio live una fuente de paquetes local de confianza amplia. |
| Configuración Calamares | Se conserva como plantilla pasiva bajo `/usr/share/danenone/calamares-template`. | No se crea `/etc/calamares`, `settings.conf` ejecutable, `unpackfs` activo ni lanzador de instalación. |

## Puertas de implementación

La automatización debe rechazar rutas relativas, paquetes que no coincidan con los dos nombres esperados, archivos sin hash y un repositorio sin base `danenone-lab.db`. Antes de invocar `mkarchiso`, una prueba debe comprobar que `plasma-lab` conserva SDDM, Plasma/KWin y la sesión avanzada Hyprland, que RC1 no ha cambiado y que no existe configuración Calamares activa.

El siguiente paso puede implementar el generador del repositorio temporal y el perfil de paquetes `plasma-lab`; todavía no puede compilar una ISO ni iniciar Calamares. La activación exige una configuración de instalación separada, checksum y ruta live de una ISO nueva, además del preflight y de pruebas BIOS/UEFI sobre `qcow2`.

## Validación del repositorio temporal

El generador preparó un repositorio temporal dentro del chroot Arch con los dos paquetes esperados, una base `danenone-lab.db.tar.zst` y un manifiesto de dos hashes SHA-256. El perfil `plasma-lab` generado contra esa fuente incluyó las solicitudes de paquete `danenone-calamares` y `danenone-calamares-config`, y agregó el repositorio únicamente a su `pacman.conf` de construcción. La raíz generada no contenía `/etc/calamares`.

La creación de la base requirió montajes privados de `/dev` y `/proc` dentro del chroot para que `repo-add` pudiera usar `/dev/fd`; esos montajes se desmontaron al finalizar. Esta condición pertenece al entorno de construcción, no a la ISO ni al sistema anfitrión. Los paquetes, la base, el manifiesto y los perfiles temporales permanecen fuera de Git.

Antes de compilar la ISO aún se exige: reconstruir ambos paquetes para la ejecución concreta, regenerar y verificar el manifiesto, comprobar la configuración ejecutora independiente de Calamares, ejecutar el preflight contra una ISO nueva y limitar cualquier arranque a `qcow2` en BIOS y UEFI.

## Referencias

[1]: https://wiki.archlinux.org/title/Archiso "ArchWiki — Archiso: custom local repository"

[2]: https://pacman.archlinux.page/pacman.conf.5.html "pacman.conf(5) — Using Your Own Repository"
