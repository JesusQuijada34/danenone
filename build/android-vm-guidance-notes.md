# Ejecución de Influent Danenone en Android

## Fuentes verificadas

Termux se distribuye oficialmente desde F-Droid y GitHub. La documentación del proyecto indica que Android debe ser 7 o superior para la versión actual y advierte que todas las aplicaciones Termux y sus complementos deben proceder de la misma fuente de firma; no se debe mezclar F-Droid, GitHub y Google Play en una misma instalación.

Termux:X11 es el servidor X oficial del ecosistema Termux. Requiere Android 8 o superior y dos componentes: la aplicación Android y el paquete companion dentro de Termux. Su documentación recomienda la variante sharedUid para evitar que Android reduzca el tiempo de CPU del proceso Termux cuando Termux:X11 está en primer plano, pero esa variante exige que Termux y Termux:X11 procedan de los builds compatibles de GitHub.

El README de Termux:X11 documenta el uso con `termux-x11 :1`, `DISPLAY=:1` y el arranque de sesiones gráficas. Para entornos proot se necesita `--shared-tmp`; esto será importante si se prueba un entorno Linux auxiliar, aunque QEMU normalmente puede ejecutarse directamente desde Termux.

La documentación oficial de QEMU confirma que QEMU es un emulador de sistema y que los binarios se distribuyen mediante los gestores de paquetes de cada plataforma. La búsqueda del repositorio oficial de paquetes Termux muestra el nombre actual `qemu-system-x86_64`, además de variantes headless, pero también incidencias recientes de dependencias en algunos dispositivos. El comando debe comprobarse con `command -v qemu-system-x86_64` y no asumirse como universal.

## Conclusión técnica

La ISO de Danenone es x86_64. La mayoría de los teléfonos Android son ARM64, por lo que QEMU tendrá que emular x86_64 y será más lento que una instalación nativa. La ruta más segura es mantener Android intacto, instalar Termux y Termux:X11 desde la misma fuente, instalar QEMU, copiar la ISO desde el release v0.4.0, verificar SHA-256 y arrancarla en modo gráfico. Danenone no debe instalarse directamente sobre las particiones del teléfono como si fuera una ROM Android.

## Fuentes

1. Termux App: https://github.com/termux/termux-app
2. Termux:X11: https://github.com/termux/termux-x11
3. Termux en F-Droid: https://f-droid.org/en/packages/com.termux/
4. QEMU Downloads: https://www.qemu.org/download/
5. Termux QEMU package issues: https://github.com/termux/termux-packages/issues/30743
