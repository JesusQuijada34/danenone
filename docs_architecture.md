# Danenone: arquitectura inicial

## Objetivo

Danenone será una distribución Linux orientada a escritorio, con un núcleo Linux estándar y una sesión gráfica propia escrita en Python con PyQt5 en el prototipo inicial. La arquitectura separa la base arrancable, la sesión visual y los servicios de usuario para que la interfaz pueda evolucionar hacia PySide6 cuando el entorno de destino lo permita.

## Capas

| Capa | Implementación inicial | Responsabilidad |
|---|---|---|
| Arranque | GRUB/UEFI + BIOS mediante una imagen ISO híbrida | Mostrar un arranque claro, cargar el kernel y el initramfs. |
| Base Linux | Debian/Ubuntu minimal dentro de una raíz squashfs | Kernel, systemd, red, usuarios, audio, almacenamiento y sesión. |
| Sesión | `danenone-shell` Python/Qt | Escritorio, lanzador, taskbar, widgets, notch, centro de control y ventanas. |
| Composición | Qt translucent widgets y colores derivados del tema | Acrílico/mica aproximado en el prototipo; compositor real en una etapa posterior. |
| Tienda | `foundstore-client` | Leer metadatos Fluthin desde releases GitHub, mostrar apps y descargar `.iflapp`. |
| Instalador | `danenone-installer` | Flujo visual para seleccionar disco, usuario, zona horaria y confirmación de escritura. |
| Integración OS | archivos `.desktop`, `autorun`, asociaciones y servicio de actualización | Registrar aplicaciones Danenone y mantener compatibilidad Fluthin. |

## Experiencia visual

La sesión usa una barra inferior compacta inspirada en Windows 11, con iconos centrados y una bandeja de sistema. El notch superior es una ventana visual pequeña que contiene reloj, conectividad y controles rápidos; no pretende modificar el hardware del panel. El escritorio se organiza en páginas horizontales con puntos de paginación, mientras el lanzador y los widgets se abren como paneles acrílicos. Las ventanas usan esquinas redondeadas, sombras suaves, barra de título compacta y estados minimizar, maximizar y mosaico.

## Seguridad y límites

El cliente FoundStore solo aceptará repositorios y releases configurados explícitamente, validará HTTPS, tamaño, SHA-256, extensión `.iflapp`, XML interno y ausencia de path traversal antes de instalar. El instalador se ejecutará en modo simulación si no tiene privilegios y nunca escribirá en un disco sin una acción explícita del usuario. El prototipo de esta fase no se considera una distribución de producción ni debe instalarse en hardware real sin revisión adicional.

## ISO reproducible

El entorno actual no dispone de `live-build`, `debootstrap`, `grub-mkrescue`, `xorriso`, `mksquashfs` ni QEMU. Por ello, la primera entrega debe separar un prototipo ejecutable de la sesión Qt y un constructor de ISO reproducible que detecte dependencias, descargue una base Linux con verificación de hash y cree una ISO cuando las herramientas estén disponibles. Si las dependencias se instalan en el sandbox, la prueba será estructural y de arranque; una prueba completa de hardware y gráficos queda fuera del entorno.

## Repositorio propuesto

El código se organizará en `danenone-shell/`, con `src/danenone_shell/`, `src/foundstore_client/`, `src/installer/`, `iso/`, `tests/`, `docs/` y `pyproject.toml`. El repositorio público o privado contendrá el manifiesto, scripts de build, checksums, documentación, capturas generadas y las notas de release.
