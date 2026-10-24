# R36S NEXUS

Launcher modular para R36S, con foco inicial en navegación, personalización y configuración del dispositivo. La arquitectura separa la interfaz, los servicios del sistema y sus contratos compartidos.

## Estado actual

`nexusd` y `nexusctl` tienen RPC de salud, información del sistema, capacidades, estado/escaneo Wi-Fi y estado/control de hardware. Compilan para Linux AMD64 y ARM64. Salud, sistema, Wi-Fi y lecturas de hardware se validaron por SSH en la R36S. `iw` está instalado en `/usr/sbin`; Ansible ejecuta `nexusd` como servicio systemd con `CAP_NET_ADMIN`, y el escaneo Wi-Fi respondió en el hardware. El control de volumen se probó; brillo fue denegado por permisos de sysfs. No se probaron apagado ni reinicio. La interoperabilidad con un futuro cliente C++ y la implementación de `nexus-ui` siguen pendientes.

## Mapa del repositorio

| Área | Propósito |
|---|---|
| [`apps/nexus-ui`](apps/nexus-ui/README.md) | Interfaz C++/LVGL y preferencias visuales locales. |
| [`apps/nexusd`](apps/nexusd/README.md) | Daemon Go para capacidades del sistema y hardware. |
| [`packages/proto`](packages/proto/README.md) | Contratos Protobuf versionados entre procesos. |
| [`generated`](generated/README.md) | Salidas reproducibles de generación de contratos. |
| [`cmake`](cmake/README.md), [`toolchains`](toolchains/README.md) | Soporte de configuración y compilación. |
| [`scripts`](scripts/README.md) | Automatización de desarrollo y empaquetado. |
| [`deploy`](deploy/README.md) | Prototipado remoto por SSH e integración futura con Armbian. |
| [`docs`](docs/README.md) | Documentación transversal y decisiones del proyecto. |
| [`.github/workflows`](.github/workflows/README.md) | Automatización futura de CI. |

## Fases del proyecto

1. Monorepo y herramientas.
2. `nexusd` base, RPC sobre UDS, salud y CLI `nexusctl`.
3. Información del sistema y capacidades.
4. Wi-Fi.
5. Configuración de hardware: audio, pantalla y energía.
6. Núcleo de interfaz de escritorio.
7. Navegación, preferencias locales e integración de la UI con `nexusd`.
8. Integración y recuperación en la compilación Armbian para R36S.
9. Optimización y estabilización de la versión 0.1.

Las fases 2–5 completan primero el daemon y su CLI antes de iniciar el launcher. Las fases se ejecutan en orden y se revisan antes de avanzar. El detalle y las responsabilidades están en el `README.md` de cada área y en [`AGENTS.md`](AGENTS.md).
