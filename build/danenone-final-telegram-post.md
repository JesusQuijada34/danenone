*Danenone — avance en vivo 6*

La captura gráfica llegó al escritorio y mostró el cubito, pero reveló que el tour se estaba lanzando al mismo tiempo que el OOBE, por lo que el tour cubría la pantalla de configuración.

La causa estaba en dos `exec-once` independientes de Hyprland. Ya se corrigieron ambos perfiles para ejecutar primero `influent-danenone-firstboot`, esperar a que termine y solo después iniciar `influent-danenone-tour`.

La próxima ISO se reconstruirá con esta secuencia. Así podremos ver el OOBE completo, crear el usuario y, al finalizar, entrar al tour y al escritorio sin ventanas superpuestas.


*Danenone — avance en vivo 7*

La corrección de permisos funcionó: la nueva ISO ya muestra el OOBE real, con el cubo y la distribución de dos columnas, y el tour ya no se adelanta.

Al probar el paso de conectividad, el asistente consultó NetworkManager de forma real y no permitió continuar porque el servicio no estaba habilitado en la imagen. Esto confirma que el bloqueo de idioma antes de Wi-Fi sí funciona, pero también revela una corrección necesaria: habilitar `NetworkManager.service` durante la personalización de Archiso para que el adaptador de la VM y el hardware físico sean gestionados desde el arranque.

Se aplicará esa corrección, se reconstruirá la ISO y se repetirá el flujo hasta usuario, reinicio de greetd, tour y escritorio.


*Danenone — OOBE de referencia implementado*

Se comparó el resumen de Gemini con el video y se confirmó que describen la misma introducción: panel centrado de vidrio, fondo dinámico, diseño de una/dos columnas, progreso inferior, tipografía sans-serif, transiciones suaves y reinicio con spinner.

Implementación completada en C/GTK:

- Arroyo desenfocado alrededor de la ventana.
- Panel central oscuro y translúcido con bordes redondeados.
- Logo PNG real del cubo, sin usar el cubo SVG genérico como identidad.
- Iconos SVG underlined para continuar, atrás, red, confirmación y cierre.
- Paleta azul profunda/violeta inspirada en la referencia.
- Fade-in del panel y pantalla final “Preparando tu escritorio”.
- `librsvg` añadido al perfil para renderizar SVG en la ISO.
- Captura validada dentro de QEMU con la ISO real.

El cambio quedó publicado en `main` del repositorio público `JesusQuijada34/danenone`.
Commit: `da8a83e` — `Redesign OOBE with reference glass visual system`.
