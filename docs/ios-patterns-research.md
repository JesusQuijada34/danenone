# Investigación de patrones para Influent Danenone

## Hallazgos confirmados

Apple describe iOS 26 como un rediseño transversal basado en un material translúcido llamado Liquid Glass. La fuente oficial señala que el material cambia según el contenido y el contexto, adapta su color entre entornos claros y oscuros, responde al movimiento y se usa en botones, interruptores, deslizadores, barras, Lock Screen, Home Screen, notificaciones y Control Center [1].

La misma fuente explica que los controles se comportan como una capa funcional diferenciada sobre el contenido, que pueden cambiar de forma según el contexto y que las barras de navegación se reducen al hacer scroll para devolver protagonismo al contenido, expandiéndose al volver a desplazarse hacia arriba [1]. También describe iconos y widgets por capas, apariencias claras, oscuras, tintadas y transparentes, además de controles que se adaptan a esquinas redondeadas y diferentes superficies [1].

Apple Support resume iOS 26 como una combinación de nuevo diseño, experiencias inteligentes y mejoras en aplicaciones, con Liquid Glass aplicado a Lock Screen, Home Screen, aplicaciones, navegación y controles [2]. La página pública de novedades muestra que Apple está presentando iOS 27 con énfasis en Foundation Models, App Intents, búsqueda contextual, widgets, diseño, accesibilidad, gráficos y nuevas APIs [3]. Esto confirma el ecosistema de desarrollo disponible, pero no constituye una especificación completa de un supuesto comportamiento final de iOS 27.

## Traducción original para Influent Danenone

| Patrón observado | Traducción original en Influent |
|---|---|
| Material translúcido que responde al contenido | Paneles Influent Glass con opacidad y color calculados desde el wallpaper, sin copiar nombres ni recursos de Apple. |
| Controles como capa funcional | Centro de control flotante sobre el escritorio, con separación visual clara, lectura de hardware real y acciones reversibles. |
| Barras que reducen su presencia | Barra inferior centrada que puede compactarse al maximizar contenido, pero vuelve a expandirse al acercar el puntero o tocar el borde inferior. |
| Iconos por capas y tintes | SVG monocromos con color de acento configurable y subrayado de estado, evitando emojis y glifos Unicode. |
| Adaptación a pantallas y esquinas | Safe areas registradas por Influent para reservar el notch y los bordes redondeados cuando una aplicación solicita pantalla completa. |
| Búsqueda y App Intents | Intenciones locales de Influent para lanzar aplicaciones, buscar archivos y consultar paquetes; no se ejecuta código remoto sin consentimiento. |
| Notificaciones y contexto | Tarjetas agrupadas por aplicación con hora, prioridad, acción y estado de lectura, ancladas al status bar y expandibles desde el notch. |

## Criterio sobre iOS 27

No se deben tratar rumores o publicaciones de terceros como requisitos del sistema. Para Influent se adoptarán únicamente patrones públicos y verificables: materiales adaptativos, navegación contextual, widgets, búsqueda local, acciones declarativas, accesibilidad y composición por capas. El sistema mantendrá una identidad propia: barra inferior familiar para usuarios de Windows/deepin, escritorio de iconos adaptativos y un instalador Calamares personalizado.

## Referencias

[1]: https://www.apple.com/newsroom/2025/06/apple-introduces-a-delightful-and-elegant-new-software-design/ "Apple introduces a delightful and elegant new software design"

[2]: https://support.apple.com/en-us/123075 "About iOS 26 Updates — Apple Support"

[3]: https://developer.apple.com/ios/whats-new/ "What’s New — iOS — Apple Developer"
