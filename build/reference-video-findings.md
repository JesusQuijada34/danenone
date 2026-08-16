# Hallazgos visuales del video de referencia

## Capturas revisadas

La hoja de contacto y la captura `00_06.jpg` muestran un asistente/instalador con una composición oscura, centrada y muy aireada sobre un fondo abstracto azul-violeta. La ventana no está presentada como un formulario tradicional: parece una superficie de vidrio oscuro con transparencia, bordes suaves, sombras amplias y un contraste moderado.

La pantalla inicial utiliza un encabezado pequeño con marca en la zona superior izquierda, un título grande alineado a la izquierda y texto explicativo debajo. En la parte inferior aparecen acciones secundaria y primaria, mientras una línea de progreso horizontal delgada se mantiene cerca del borde inferior de la ventana con texto de etapa centrado. La acción primaria es un botón compacto de color púrpura/azul; la secundaria tiene menor énfasis visual.

El video muestra una jerarquía tipográfica sans-serif geométrica: título grande semibold, cuerpo de tamaño medio con opacidad menor y microtexto de progreso claramente separado. Los márgenes internos son amplios y el contenido parece diseñado para ratón y tacto.

La hoja de contacto también muestra pantallas de selección de edición, particionado de disco y personalización en dos columnas. El flujo incluye una pantalla negra de reinicio con spinner y texto centrado, seguida por una pantalla de transición con un símbolo grande antes de regresar al asistente. En las pantallas posteriores aparece un escritorio con explorador de archivos oscuro y una barra de tareas flotante segmentada en islas redondeadas.

## Traducción preliminar a Danenone

El OOBE de Danenone debe conservar su ventana central, pero sustituir el fondo claro plano por el arroyo difuminado y una capa de cristal translúcido. Los botones deben mantener la claridad del video: acción primaria coloreada, acción secundaria translúcida, radios amplios y estados hover/pressed suaves. El progreso debe permanecer en el borde inferior del panel y mostrar la etapa actual sin competir con el título.

La implementación seguirá siendo nativa en C/GTK/Wayland. El desenfoque se resolverá con un asset preprocesado para el OOBE y capas translúcidas GTK, sin introducir Python en el runtime de la ISO. Las transiciones internas pueden usar el `GtkStack` deslizante existente, con una duración objetivo de aproximadamente 300–500 ms; la entrada del panel puede incorporar una transición de opacidad y escala si se implementa con una capa adicional.


## Observaciones adicionales de capturas detalladas

La captura `02_38.jpg` confirma una composición de dos columnas: a la izquierda aparece una ilustración grande y expresiva que funciona como guía visual, mientras los controles de personalización se concentran a la derecha. Las opciones se presentan como miniaturas seleccionables, con una opción activa claramente iluminada y un subrayado/acento púrpura. El encabezado superior conserva marca, flecha de regreso y contexto del flujo; en la zona inferior se mantienen controles de accesibilidad/sonido y el botón principal.

La captura `04_31.jpg` muestra una transición de entrada al escritorio deliberadamente minimalista: fondo oscuro, símbolo central grande, texto breve centrado y mucho espacio negativo. Para Danenone, esta fase puede convertirse en una pantalla intermedia con el logo PNG real del cubo, una animación de aparición suave y el texto “Preparando tu escritorio” antes de lanzar el tour.


## Validación del asset del arroyo y del layout

La revisión del asset `oobe-river-blurred.jpg` confirma que conserva el arroyo y el paisaje del wallpaper de Danenone, con un desenfoque fuerte apropiado para una capa de vidrio. La captura local anterior no mostraba este paisaje porque el sistema host no tenía instalados los assets `/usr/share` de la ISO y porque el overlay GTK no tenía un widget base de pantalla completa. El código ya fue corregido para usar el asset del repositorio como fallback durante pruebas locales y para añadir un contenedor base expandible antes de superponer fondo, wash y panel.


## Diagnóstico de captura local del OOBE

La segunda captura local todavía está dominada por el PNG del cubo y no muestra la composición completa. El asset del logo es correcto y coincide con la identidad visual del sistema, pero `gtk_image_set_pixel_size()` no limita por sí solo la requisición natural del widget cuando se usa dentro de una jerarquía expandible. La siguiente corrección debe fijar explícitamente el tamaño solicitado del widget de imagen y evitar que el contenedor izquierdo o la imagen se expandan fuera de su columna; después se repetirá la captura.


## Validación dentro de la ISO

La captura `oobe-iso-reference.png` confirma que el diseño llega al runtime real de Archiso. El arroyo aparece alrededor de la ventana central, el panel usa cristal oscuro translúcido, el logo PNG real está contenido en la columna izquierda, el cierre superior se muestra como SVG y la flecha del botón principal se renderiza correctamente después de añadir `librsvg` al perfil. La estética está ahora mucho más cerca de la introducción del video que la versión clara anterior.


## Fullscreen y modo demostración

La nueva captura fullscreen confirma el ajuste solicitado: la ventana ya ocupa la pantalla completa, el borde externo es recto y el redondeo se trasladó al panel de interacción derecho, con márgenes internos amplios. El panel izquierdo conserva el cubo y la guía visual sobre el arroyo.

El modo `--demo` también fue probado. Después de tres segundos acepta la bienvenida, pasa a Conectividad y se detiene si NetworkManager no informa una conexión real. La captura muestra el mensaje de detención y confirma que no crea usuarios, no cambia contraseñas y no aplica ninguna configuración automáticamente. Esto mantiene la demostración segura y separada del flujo normal del OOBE.


## Revisión específica de proporciones del cuadro

Las capturas del video muestran que el cuadro de interacción no ocupa casi toda la altura disponible. En una composición de 1280×720, el contenido útil del panel derecho se concentra aproximadamente en la franja central, con un ancho moderado y un margen visible de fondo a izquierda, derecha y abajo. El instalador inicial usa un tratamiento casi fullscreen, pero la tarjeta interna permanece contenida; la pantalla de personalización mantiene la ilustración a la izquierda y un bloque derecho más estrecho, con controles agrupados y un botón Next pequeño en la esquina inferior derecha.

La corrección para Danenone será reducir el panel derecho interno respecto al fullscreen: conservar el margen de 40 px alrededor, bajar su expansión vertical mediante una caja de contenido con `valign=GTK_ALIGN_CENTER`, usar un ancho aproximado del 58–62 % de la pantalla y evitar que el `GtkStack` estire la tarjeta hasta el borde inferior. No se cambiarán el fondo, la tipografía ni el resto del tema.


## Iteración de simplificación visual

Se eliminó la tarjeta grande que envolvía todo el contenido derecho. La composición actual mantiene la pantalla completa, pero deja el contenido derecho sobre el fondo oscuro y reserva el redondeo únicamente para elementos que realmente lo necesitan. El resultado muestra más fondo visible, una jerarquía más cercana a la captura del video y un cuadro de interacción que ya no domina la pantalla. También se redujeron el ancho del contenido, la altura del stack y los márgenes internos.


## Validación final fullscreen y verde

La ventana GTK fue medida con `xwininfo` en Xvfb y ocupa exactamente `1280x800+0+0`, por lo que el OOBE sí es fullscreen real; lo que debía reducirse era el cuadro interno, no la ventana raíz. La versión final elimina la tarjeta pesada, deja el fondo del arroyo visible y cambia el botón principal, foco, estados activos, panel final y barra de progreso a una paleta verde/verde azulado. La ISO final contiene un squashfs de aproximadamente 518 MiB y el hash SHA-256 del binario `firstboot` extraído de la ISO coincide con el binario compilado del proyecto.
