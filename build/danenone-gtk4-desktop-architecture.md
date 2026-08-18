# Arquitectura del escritorio GTK4 de Influent Danenone

## Objetivo

El escritorio se implementará como una superficie GTK4 independiente del OOBE y del shell Qt heredado. Su responsabilidad será organizar accesos directos, iconos de aplicaciones, unidades y papelera de manera determinista, reservando siempre el espacio de la barra de tareas y del notch.

## Principios de composición

El escritorio no colocará iconos con coordenadas absolutas. Cada elemento ocupará una celda de una cuadrícula adaptable calculada a partir del tamaño útil de la pantalla. El área útil se obtiene restando el margen superior del notch, el margen inferior reservado por la barra y los márgenes laterales del shell.

Los iconos se ordenan de arriba hacia abajo y después de izquierda a derecha, empezando en la esquina superior izquierda del área útil. Cada celda tendrá un ancho constante, una altura constante y una separación visible únicamente por el espacio, nunca por líneas o paneles. Si una columna alcanza el límite inferior, el siguiente elemento pasa a la columna siguiente.

## Fuentes de elementos

| Fuente | Elementos |
|---|---|
| `~/.local/share/applications` | Aplicaciones instaladas por el usuario |
| `/usr/share/applications` | Aplicaciones del sistema |
| `~/Desktop` | Accesos directos creados por el usuario |
| `~/Downloads` | Solo si el usuario activa la vista de descargas |
| `trash:///` | Papelera como elemento especial persistente |
| Volúmenes montados | Unidades y particiones detectadas mediante GIO/GVolumeMonitor |

Los archivos `.desktop` se parsean mediante `GDesktopAppInfo`/GAppInfo. Solo se muestran entradas visibles, con nombre válido e icono resoluble. Las aplicaciones sin icono no se inventan ni se sustituyen por emojis; se omiten o se muestran únicamente en la lista completa de aplicaciones.

## Reglas de orden

El orden inicial será: Papelera, aplicaciones ancladas por el sistema, accesos del usuario, unidades montadas y accesos restantes. Después de que el usuario reordene elementos, el shell guardará el orden en `~/.config/influent-danenone/desktop-layout.tsv` con el identificador estable del elemento, la columna y la fila.

Si cambia la resolución, el shell conserva el orden lineal y lo vuelve a distribuir en la cuadrícula disponible. Si desaparece un volumen o un `.desktop`, su posición queda libre y no desplaza de forma impredecible el resto de elementos hasta la siguiente reorganización explícita.

## Interacción

Un clic abre el elemento. Un clic secundario muestra un menú contextual GTK4 con abrir, fijar a la barra, mover a la papelera cuando corresponda y propiedades. El arrastre solo modifica el modelo de orden cuando el usuario suelta el elemento dentro de una celda válida. El modo de edición mostrará una marca visual de selección, pero no añadirá bordes permanentes al escritorio.

## Reserva de pantalla

La barra de tareas se implementará como superficie `gtk4-layer-shell` anclada al borde inferior con zona exclusiva. El escritorio no debe colocar iconos dentro de esa zona. El notch se anclará al borde superior sin contenido y el área útil comenzará por debajo de su margen de seguridad. Las ventanas maximizadas deben respetar la zona exclusiva del panel; el escritorio no intentará corregirlo con coordenadas manuales.

## Estilo

La superficie será plana con estética UWP: fondo real, tipografía legible, iconos SVG FreeDesktop, estados hover/pressed discretos, selección con acento verde y animaciones cortas de entrada y reorganización. El desenfoque se limitará a paneles flotantes cuyo fondo sea conocido por el shell; no se simulará la captura de ventanas externas.

## Persistencia y seguridad

El archivo de distribución de iconos se escribirá de forma atómica. Los `.desktop` no se ejecutan por comandos concatenados: se lanzan mediante `GAppInfo` o `g_app_info_launch()`. Las rutas de volúmenes se obtienen de GIO y no se construyen a partir de texto sin validar.
