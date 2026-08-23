# Edición Plasma de Influent Danenone

## Decisión de arquitectura

La edición nueva será un perfil **separado** de la ISO RC actual. Su sesión predeterminada será **Plasma (Wayland)** sobre KWin y mantendrá **Hyprland** como alternativa avanzada elegible desde el gestor de inicio. La imagen RC basada en `greetd` y Hyprland no se modifica con esta transición.

| Área | Edición Plasma propuesta | Conservación de compatibilidad |
|---|---|---|
| Sesión predeterminada | Plasma (Wayland) | El perfil RC existente no cambia |
| Compositor | KWin | Hyprland continúa disponible como sesión avanzada |
| Gestor de inicio | SDDM con Plasma Wayland preseleccionado | La persona puede elegir Hyprland desde las entradas de `/usr/share/wayland-sessions/` |
| Instalador | Calamares con identidad y presentación QML propias | El OOBE heredado queda aislado mientras se migra gradualmente |
| Tema | Base Breeze/Plasma con acento Verdypor, contrastes accesibles y recursos propios | No se descargan temas de terceros sin auditoría |

Plasma Wayland es la ruta recomendada por KDE para Plasma reciente, mientras que SDDM enumera sesiones Wayland y puede iniciarlas. Por este motivo, el perfil incluye `plasma-meta`, `sddm`, `kwin`, `plasma-workspace`, los portales Plasma y la entrada de Hyprland, en lugar de iniciar directamente el compositor desde `greetd`. En Arch actual, `plasma-workspace` reemplaza al paquete histórico `plasma-wayland-session`. [1] [2] [5]

> SDDM funciona como gestor gráfico de inicio y de sesiones; las sesiones Wayland instaladas se descubren desde `/usr/share/wayland-sessions/`. [1]

Hyprland seguirá instalado, pero se aislarán sus reglas, su portal y sus autostarts en su propia sesión. Esto evita que un cambio de sintaxis o configuración avanzada de Hyprland afecte la sesión Plasma. El perfil tendrá también paquetes de portal y autenticación necesarios para los flujos Wayland de cada sesión. [3]

## Instalador y personalización

Calamares se configurará como instalador de la edición Plasma con branding local. Sus componentes de identidad permiten definir nombre, colores, imágenes, tamaño de ventana y presentación QML; los módulos deben mantenerse limitados a acciones explícitas y revisables. [4]

El alcance de esta iteración es preparar el perfil, sesiones y branding base. La migración visual completa del OOBE sólo se activará cuando Calamares, particionado, usuarios, localización y recuperación DaneDesk tengan validaciones específicas.

## Criterios de aceptación

La edición Plasma candidata deberá construir sin modificar `archiso-profile`, mostrar Plasma (Wayland) como selección predeterminada de SDDM, enumerar Hyprland como alternativa, arrancar en QEMU y conservar Foundstore como aplicación del sistema. Se verificará además que los recursos de la ISO y la configuración del gestor de inicio no incluyan claves ni datos de dispositivo.

## Referencias

1. [ArchWiki: SDDM](https://wiki.archlinux.org/title/SDDM)
2. [ArchWiki: KDE](https://wiki.archlinux.org/title/KDE)
3. [ArchWiki: Hyprland](https://wiki.archlinux.org/title/Hyprland)
4. [Calamares Extensions: Branding and Module Examples](https://github.com/calamares/calamares-extensions)
5. [Arch Linux: plasma-workspace](https://archlinux.org/packages/extra/x86_64/plasma-workspace/)
