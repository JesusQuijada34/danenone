# Danenone v0.1.0-prototype

Esta versión entrega la primera ISO arrancable de Danenone basada en Linux/Debian bookworm, con kernel Linux amd64, live-boot, GRUB, Python 3, PyQt5, requests, NetworkManager y el shell visual Danenone integrado.

La interfaz incluye escritorio paginado horizontalmente, puntos de navegación, barra de tareas compacta, notch superior, centro de control y paneles translúcidos. El repositorio también contiene el cliente FoundStore con validación de releases `.iflapp`, un planificador de instalación en modo simulación y la receta reproducible `iso/build_iso.sh`.

El asset `danenone-0.1.0-prototype-amd64.iso` fue inspeccionado como ISO 9660 bootable con registro El Torito y arrancó en QEMU hasta cargar GRUB/stage2. SHA-256: `b13ce1e927b38c2f1e7db8024a9e52e5681dbb632fe18aac6b9d18854b01aa4f`.

Esta no es todavía una distribución de producción: el instalador real de disco permanece deshabilitado, la composición acrílica es una aproximación Qt y la sesión gráfica necesita un entorno gráfico disponible. Se recomienda probar primero en una máquina virtual.
