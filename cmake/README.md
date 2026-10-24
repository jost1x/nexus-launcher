# CMake compartido

Área para módulos y configuración CMake reutilizable por la UI y sus dependencias. La configuración específica de `nexus-ui` permanece en su aplicación.

## Fases

- Fase 0: establecer convenciones compartidas y configuración mínima.
- Fase 6: integrar LVGL/SDL2 para compilación de escritorio, una vez terminado `nexusd` y su CLI.
- Fase 8: habilitar opciones necesarias para la compilación cruzada ARM hacia Armbian.

Mantener la configuración pequeña y no introducir un sistema de build adicional.
