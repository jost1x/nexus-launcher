# nexus-ui

Interfaz del launcher, escrita en C++17 con LVGL 9. SDL2 será el backend inicial de escritorio; la aplicación debe poder ejecutarse y navegarse aunque `nexusd` no esté disponible.

## Responsabilidades

- Renderizar pantallas, componentes, temas, transiciones y estados de foco.
- Gestionar navegación desde controles físicos y SDL2.
- Mantener internacionalización, sonidos y preferencias visuales.
- Consumir RPC de forma asíncrona sin bloquear el hilo principal de LVGL.
- Persistir preferencias propias en JSON validado y con escritura atómica.

## Fases

La UI comienza después de completar las fases 2–5 de `nexusd` y `nexusctl` en el roadmap del proyecto.

1. (Fase 6 del proyecto) Inicializar LVGL/SDL2, ventana 640×480, ciclo de eventos y cierre limpio; pantallas Inicio, Configuración y Acerca de.
2. (Fase 7) Añadir cruceta, A/B, foco visible, navegación completa, teclado virtual, persistencia de preferencias e integración RPC asíncrona.
3. (Fase 8) Integrar con la compilación Armbian propia y validar la experiencia conjunta en la consola real.

La UI no contiene lógica de acceso directo a hardware ni implementaciones internas de Go. Los eventos visuales y las pulsaciones de botones no se envían por RPC.
