# R36S NEXUS — Lineamientos del proyecto

Este documento define las reglas de trabajo para R36S NEXUS. El proyecto es un launcher para la consola R36S, orientado inicialmente a navegación, personalización y configuración del dispositivo, con destino a la compilación Armbian mantenida en el proyecto hermano `r36s-os`.

## Objetivos de ingeniería

- Priorizar simplicidad, rendimiento, modularidad y facilidad de mantenimiento.
- Avanzar por fases pequeñas y verificables; no implementar el proyecto completo en una sola iteración.
- No declarar una fase terminada sin verificaciones que respalden ese estado.
- Mantener documentadas las dependencias, limitaciones reales y decisiones técnicas.
- Evitar sobreingeniería, abstracciones prematuras, Bazel, Kubernetes, microservicios y orquestación innecesaria.

## Stack previsto

- **Interfaz (`nexus-ui`):** C++17, LVGL 9, SDL2 para desarrollo de escritorio, CMake, gRPC C++, Protocol Buffers y JSON para preferencias locales.
- **Daemon (`nexusd`):** Go, Connect RPC compatible con gRPC, HTTP/2 sobre Unix Domain Sockets y logging con `log/slog`.
- **Contratos:** Protocol Buffers v3, gestionados con Buf y con generación reproducible para Go y C++.
- **Herramientas:** Git, pre-commit, Go Modules, Make, clang-format, golangci-lint y GitHub Actions. Docker es opcional.

Verificar las versiones y la disponibilidad de las dependencias en el entorno antes de fijar la configuración de compilación. No afirmar compatibilidad con una versión o hardware sin comprobarla.

## Iconografía

- Usar siempre iconos oficiales de Lucide en la interfaz. Preferir sus SVG originales y conservar la licencia incluida en `apps/nexus-ui/src/assets/icons/LICENSE.Lucide`.
- No dibujar aproximaciones manuales ni sustituir un icono Lucide por otro de una fuente distinta. Si se necesita un icono que no exista en Lucide, consultar al usuario antes de usar otra biblioteca o fuente.
- Para LVGL, generar los recursos rasterizados a partir de los SVG oficiales; no reemplazar los vectores originales por trazos reinterpretados.

## Entorno de compilación y despliegue

- Compilar siempre en este PC (el entorno de desarrollo local), incluida la UI destinada a ARM64. Para la R36S usar compilación cruzada y un sysroot/toolchain local cuando haga falta.
- No compilar en la R36S ni instalar allí compiladores, CMake, cabeceras o dependencias `-dev` para construir el proyecto. La consola se usa para ejecutar y validar los artefactos desplegados.
- Si faltan dependencias para compilar localmente, resolverlas en el PC o preparar un sysroot local; no trasladar la compilación a la consola como alternativa.

## Límites arquitectónicos

- `nexus-ui` controla la presentación, navegación, entrada, temas, idiomas, sonidos y preferencias visuales.
- `nexusd` controla la integración con Linux y el hardware. No administra preferencias visuales.
- Los contratos Protobuf son la interfaz pública entre los procesos. La UI no depende de implementaciones internas de Go.
- Usar UDS local con cliente gRPC C++ y servidor Connect-Go compatible con gRPC; no agregar proxies. Resolver explícitamente HTTP/2 sobre UDS sin TLS y verificar la interoperabilidad real.
- Las llamadas RPC deben ser asíncronas desde la UI, con timeout, cancelación, errores tipados y recuperación ante desconexiones. Nunca bloquear el hilo principal de LVGL.
- La interfaz debe seguir funcionando si `nexusd` no está disponible.
- La UI guarda sus preferencias en JSON, con validación y escritura atómica. No trasladar esas preferencias al daemon.
- Detectar capacidades y usar adaptadores para hardware. No asumir rutas sysfs, controles ALSA, NetworkManager ni permisos root.
- Se permite desplegar prototipos en la R36S por SSH para pruebas. Mantener ese flujo separado de la integración de la imagen, no incluir credenciales y evitar cambios persistentes del sistema hasta la fase de integración.
- No registrar secretos ni ejecutar comandos arbitrarios.
- Mantener las pruebas de hardware separadas de las verificaciones generales de CI.

## Fases de trabajo

1. **Monorepo y herramientas:** estructura, configuración, contratos, generación, scripts y documentación inicial. La integración con GitHub Actions se pospone hasta la fase final.
2. **`nexusd` base y CLI:** proceso Go, RPC sobre UDS, salud, logging, cierre ordenado y `nexusctl`; validar interoperabilidad RPC antes de iniciar la UI.
3. **Información del sistema:** métricas disponibles y capacidades, con adaptadores Linux y mocks.
4. **Wi-Fi:** detección y operaciones cancelables, verificadas contra los mecanismos disponibles en Armbian.
5. **Hardware:** audio, pantalla y energía, con validación y confirmación para operaciones destructivas.
6. **Núcleo de UI:** aplicación LVGL/SDL2 de escritorio y pantallas iniciales, después de completar las fases de `nexusd`.
7. **Navegación e integración UI:** controles físicos/SDL2, foco, navegación completa, persistencia local e integración RPC asíncrona.
8. **Integración Armbian:** compilación cruzada, empaquetado, instalación, diagnóstico y mecanismo de recuperación para la imagen propia de R36S.
9. **Estabilización:** mediciones en hardware real, optimización y documentación de la versión 0.1.

No avanzar automáticamente a una fase posterior. La biblioteca de ROMs, el lanzamiento de juegos, carátulas, metadatos, configuración de emuladores y gestión avanzada de partidas guardadas quedan fuera de la versión 0.1.

## Verificación y entrega

- Preferir verificaciones reproducibles: compilación, pruebas unitarias, integración RPC y validación de generación de código, según la fase.
- No simular resultados ni inventar métricas. Distinguir pruebas locales, CI y pruebas que requieren la consola física.
- Para cada fase, resumir el objetivo, las decisiones tomadas, los cambios, las verificaciones ejecutadas y lo pendiente.
- Mantener los contratos y el código generado en ubicaciones compartidas; no duplicar definiciones Protobuf entre aplicaciones.

## Commits

- Crear commits locales únicamente cuando el usuario solicite subir cambios o crear un commit. Una petición de implementar o desplegar no autoriza por sí sola a crear commits.
- Si el usuario pide subir cambios sin especificar destino, crear solo un commit local. Hacer push a GitHub u otro remoto únicamente cuando el usuario lo solicite explícitamente; crear un commit local no autoriza a publicarlo.
- Escribir todos los mensajes de commit en inglés y seguir el formato Conventional Commits: `<type>(<scope>): <imperative summary>`; el scope es opcional cuando no aporta claridad.
- Elegir un tipo acorde al cambio, por ejemplo `feat`, `fix`, `docs`, `build`, `ci`, `refactor`, `test` o `chore`.
- Separar los commits por responsabilidad: cada commit debe representar un cambio coherente y revisable. No mezclar responsabilidades independientes en un mismo commit.
- Mantener el resumen breve, específico y en modo imperativo; usar el cuerpo para explicar contexto cuando sea necesario.

## Operaciones externas y validación

- Aplicar cambios persistentes en la consola o desplegar servicios únicamente cuando el usuario lo solicite explícitamente. No ejecutar reboot ni poweroff como parte de las pruebas.
- Informar con precisión qué verificaciones se ejecutaron, cuáles pasaron y cuáles no pudieron completarse. Distinguir resultados locales de los observados en la R36S y no declarar como verificada una capacidad que no se haya probado.
- Revisar `git status` antes de editar. No incluir, sobrescribir ni revertir cambios ajenos a la tarea.
