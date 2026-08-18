# Capturas de Influent Danenone

Estas imágenes fueron tomadas del binario OOBE GTK4 activo y del shell Qt6 estable mediante Xvfb a 1280×800. La captura es reproducible con [`scripts/capture_oobe_pages.sh`](../../scripts/capture_oobe_pages.sh) y [`scripts/capture_qt6_shell.sh`](../../scripts/capture_qt6_shell.sh).

> Estas capturas documentan el estado visual de los binarios locales. No se generó ni se reconstruyó ninguna ISO para producirlas.

## OOBE

| Pantalla | Captura |
|---|---|
| Bienvenida | [welcome.png](oobe/welcome.png) |
| Idioma y conectividad | [language-connectivity.png](oobe/language-connectivity.png) |
| Selección de edición | [edition.png](oobe/edition.png) |
| Usuario | [user.png](oobe/user.png) |
| Apariencia | [appearance.png](oobe/appearance.png) |
| Notch | [notch.png](oobe/notch.png) |
| Resumen | [summary.png](oobe/summary.png) |
| Instalación | [installation.png](oobe/installation.png) |

La pantalla de idioma aparece en el paso 2 y usa el manifiesto de idiomas disponible. La pantalla de edición aparece en el paso 3 y contiene Home, Enterprise, Developer, Minimal y Frozen Lab. El notch se mantiene vacío, sin texto ni iconos.

## Shell Qt6

| Estado | Captura |
|---|---|
| Escritorio, iconos y barra de tareas | [desktop.png](shell/desktop.png) |

La captura del shell muestra los accesos directos alineados a la izquierda, la barra de tareas anclada al borde inferior, el perfil de usuario, los accesos de Inicio y búsqueda, las aplicaciones centrales y los controles de hardware.

Para el detalle de la revisión visual, consulta [`oobe/visual-findings.md`](oobe/visual-findings.md).
