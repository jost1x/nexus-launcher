# nexus-ui

Interfaz del launcher, escrita en C++17 con LVGL 9. SDL2 es el backend inicial de escritorio; la aplicación funciona sin `nexusd`.

## Estado actual

La aplicación presenta una interfaz de 640×480 con las vistas Inicio, Wi-Fi, Pantalla, Audio, Sistema y Acerca de. Por defecto usa una ventana SDL2 para escritorio. Para la R36S se puede compilar con `-DNEXUS_UI_FBDEV=ON`, lo que dibuja directamente en `/dev/fb0` y evita depender de EGL/Mesa/DRI; SDL2 queda a cargo de leer los controles. Esa variante necesita permisos de lectura/escritura sobre el framebuffer, normalmente ejecutándose desde una TTY con permisos adecuados. Consulta el daemon de forma asíncrona para presentar estado y redes, información del dispositivo y valores actuales de brillo y volumen. Si `nexusd` no responde, conserva la navegación y muestra el estado de servicio.

LVGL reserva 256 KiB para su heap interno. La UI registra avisos cuando quedan 48 KiB o menos y un nivel crítico cuando quedan 16 KiB o menos; también informa al recuperarse. LVGL imprime además fallos de asignación con el tamaño solicitado y memoria libre. En la unidad transitoria de Ansible, esos mensajes se consultan con `journalctl -u nexus-ui-r36s.service`.

LVGL v9.2.2 está fijado por commit en `CMakeLists.txt` y se descarga mediante CMake FetchContent. Se requiere SDL2 con sus archivos de desarrollo, Protobuf, gRPC C++ (incluido `grpc_cpp_plugin`), CMake 3.20 o posterior y un compilador C/C++17. En Manjaro/Arch se instalan con `sudo pacman -S cmake sdl2 protobuf grpc`.

```sh
cmake -S apps/nexus-ui -B build/nexus-ui -DCMAKE_BUILD_TYPE=Release
cmake --build build/nexus-ui --parallel
./build/nexus-ui/nexus-ui
```

La compilación inicial necesita acceso a GitHub para descargar LVGL. SDL2 se resuelve desde la instalación del sistema.

En escritorio, `Tab`/`Page Down` avanza el foco, `Page Up` retrocede, `Enter` acepta y `Esc` vuelve a Inicio; el ratón también activa los controles. En un mando SDL, la cruceta vertical recorre el foco, `A` acepta y `B` vuelve a Inicio. Las flechas izquierda/derecha ajustan los deslizadores. En la R36S, SDL 2.26.5 enumera `gpio-keys` con 19 botones. Una captura física confirmó A/B `0/1`, arriba/abajo `8/9` e izquierda/derecha `10/11`; este último orden difiere del mapa `/dev/input/js0`. El puente de entrada de la UI coincide con la detección SDL. La validación visual de la app en el dispositivo queda para la integración Armbian.

El cliente gRPC usa `NEXUSD_SOCKET` si está definido; de lo contrario, busca el socket en `$XDG_CACHE_HOME/nexusd/nexusd.sock` o `~/.cache/nexusd/nexusd.sock`, igual que `nexusd`.

Las fuentes Montserrat con glifos para español y su licencia están en [`src/assets/fonts`](src/assets/fonts/README.md).

## Responsabilidades

- Renderizar pantallas, componentes, temas, transiciones y estados de foco.
- Gestionar navegación desde controles físicos y SDL2.
- Mantener internacionalización, sonidos y preferencias visuales.
- Consumir RPC de forma asíncrona sin bloquear el hilo principal de LVGL.
- Persistir preferencias propias en JSON validado y con escritura atómica.

## Fases

La UI presenta las capacidades implementadas por `nexusd`; las operaciones que todavía no existen en el daemon, como conectar o desconectar redes, no se ofrecen como controles activos.

1. (Fase 6) Núcleo de escritorio LVGL/SDL2 y páginas Inicio, Wi-Fi, Pantalla, Audio, Sistema y Acerca de. Implementado.
2. (Fase 7) Navegación con foco por teclado y mando SDL, cliente gRPC C++ asíncrono sobre UDS y ajustes de brillo/volumen. Implementado; falta validación física del mapeo de botones en la R36S.
3. (Fase 8) Integrar con la compilación Armbian propia y validar la experiencia conjunta en la consola real.

La UI no contiene lógica de acceso directo a hardware ni implementaciones internas de Go. Los eventos de navegación permanecen locales; solo las operaciones del sistema se envían por RPC. La vista Wi-Fi consulta el estado y escanea redes; conectar y desconectar requieren ampliar el daemon.
