# Contratos Protobuf

Fuente canónica de Protocol Buffers v3 para la comunicación entre `nexus-ui` y `nexusd`. Los paquetes se versionan por dominio, por ejemplo `nexus.health.v1` y `nexus.system.v1`.

## Dominios previstos

Salud, Wi-Fi, sistema, audio, pantalla y energía. Añadir mensajes y RPC solo cuando una fase los necesite.

## Fases

1. Configurar Buf y validación reproducible durante la fase de herramientas del proyecto.
2. Definir HealthService para verificar la interoperabilidad Go sobre UDS con `nexusctl`; añadir el cliente C++ más adelante, al iniciar la UI.
3. Añadir contratos de sistema, Wi-Fi y hardware junto con sus implementaciones y pruebas antes de comenzar `nexus-ui`.

No duplicar definiciones ni código generado entre aplicaciones. Evitar incluir eventos de navegación o interacción visual en estos contratos.
