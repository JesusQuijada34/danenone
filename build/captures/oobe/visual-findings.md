# Verificación visual inicial del OOBE

La captura `welcome.png` muestra el fondo del arroyo desenfocado, el notch vacío en la parte superior, el cubo Danenone junto al texto de marca y una ventana central translúcida con el botón verde `Continuar`. El indicador inferior muestra `Paso 1 de 12 · Bienvenida`.

La captura `edition.png` muestra la pantalla `Elige la edición de Danenone`, un selector desplegable con `Home — uso personal` como valor inicial y el texto descriptivo de las cinco ediciones: Home, Enterprise, Developer, Minimal y Frozen Lab. El indicador inferior muestra `Paso 3 de 12 · Edición`; los botones `Atrás` y `Continuar` aparecen correctamente.

No se observa texto ni iconos dentro del notch. Las imágenes se capturaron a 1280×800 desde el binario GTK4 activo, sin construir la ISO.

La captura `language-connectivity.png` confirma que el idioma aparece en el paso 2, inmediatamente después de bienvenida, con English integrado, estado de NetworkManager y advertencia de conectividad para paquetes descargables.

La captura `summary.png` confirma que el resumen final muestra sistema, edición `home`, idioma `en`, modo de instalación y la indicación de que la identidad OEM se genera localmente sin mostrar el número de serie. El notch permanece vacío.

La captura `notch.png` muestra las opciones `Recorte completo` e `Isla dinámica`, con el recorte superior negro vacío y sin contenido superpuesto.

La captura `installation.png` muestra el paso `12 de 12 · Instalación`, la barra de progreso horizontal dentro de la ventana y el botón final `Finalizar`. No se inició ninguna instalación real ni se construyó la ISO; la imagen representa el estado visual del binario OOBE en Xvfb.

La captura del shell Qt6 (`build/captures/shell/desktop.png`) muestra el escritorio con iconos alineados a la izquierda, fondo del arroyo, notch vacío y barra de tareas completa anclada abajo con perfil de usuario, Inicio, búsqueda, aplicaciones y controles de hardware.

La captura `appearance.png` muestra las opciones de modo claro, modo oscuro y acentos Verde Danenone, Azul y Ámbar. Se mantiene el fondo translúcido y el indicador `Paso 8 de 12 · Apariencia`.
