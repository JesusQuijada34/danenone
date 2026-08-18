## Restauración final del gestor de almacenamiento

La captura `storage-english.png` muestra el diseño solicitado: una lista vertical de almacenamiento con el icono del dispositivo a la izquierda y su nombre, capacidad y sistema de archivos a la derecha. Las particiones y otros dispositivos se agregan como filas sucesivas con sangría e iconografía diferenciada cuando `lsblk` los informa.

Debajo de la lista aparece una barra horizontal de herramientas SVG para `Partition`, `Extend / resize` y `Format`. Cada herramienta enfoca el control correspondiente del plan de instalación. La primera fila real queda seleccionada con el indicador verde lateral.

El entorno de prueba solo expuso `/dev/vda` y no informó particiones adicionales; por eso la captura contiene una sola fila real. El código no simula entradas: muestra directamente los discos y particiones que devuelve `lsblk`. La compilación y la ejecución completa finalizaron sin errores GTK, segmentaciones ni errores de compilación del proyecto.
