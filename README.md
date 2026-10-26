# R36S NEXUS

Launcher modular para R36S, con foco inicial en navegación, personalización y configuración del dispositivo. La arquitectura separa la interfaz, los servicios del sistema y sus contratos compartidos.

## Estado actual

`nexusd` y `nexusctl` tienen RPC de salud, información del sistema, capacidades, estado/escaneo Wi-Fi y estado/control de hardware. Compilan para Linux AMD64 y ARM64. Salud, sistema, Wi-Fi y lecturas de hardware se validaron por SSH en la R36S. Ansible despliega el daemon como servicio systemd con los permisos necesarios para escanear Wi-Fi y ajustar el brillo. Los botones físicos de volumen y la combinación F + Volumen para brillo funcionan en la consola. No se probaron apagado ni reinicio. `nexus-ui` presenta Inicio, Juegos, Aplicaciones y Favoritos como áreas principales; Ajustes queda separado al final y agrupa Wi-Fi, Pantalla, Audio, Sistema y Acerca de. Consulta el daemon con gRPC C++ asíncrono sobre UDS. La interacción SDL del mando y los ajustes desde la UI aún requieren validación física. Consulta el estado detallado en [`CHECKLIST.md`](CHECKLIST.md).

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

## Builds ARM64 con Docker

Los builds para la R36S se ejecutan en el PC. Docker Compose prepara un entorno Debian reproducible para compilar la UI C++/SDL/gRPC cruzada a ARM64 y otro entorno Go para `nexusd` y `nexusctl`:

```sh
docker compose build --builder default build-ui build-daemon
docker compose run --rm build-ui
docker compose run --rm build-daemon
```

La UI queda en `build/docker/arm64/nexus-r36s-bundle/nexus-ui`; `nexusd` y `nexusctl` quedan en `build/docker/arm64/bin/` y se copian también a `apps/nexusd/bin/` para el despliegue Ansible existente. La primera compilación de la UI descarga LVGL y necesita acceso a GitHub.

## Comandos Make

Desde la raíz del repositorio, `make help` lista los comandos habituales. Si es la primera compilación o cambió un Dockerfile, ejecuta `make build-images`; luego `make build-arm64` compila la UI y el daemon para R36S usando las imágenes locales. `make deploy-ui` compila y despliega la UI preparando dependencias; en iteraciones posteriores puedes usar `make deploy-ui-fast` si las dependencias ya están instaladas. Estos objetivos usan directamente el bundle generado en `build/docker/arm64/nexus-r36s-bundle`. `make deploy-daemon` despliega solo el daemon y `make deploy-all` compila ambos y despliega primero el daemon y luego la UI. Los despliegues usan el inventario local de `deploy/ansible/inventory/hosts.yml` y piden la contraseña de sudo; se puede ajustar la invocación con `ANSIBLE_BECOME_FLAGS` y `ANSIBLE_ARGS`.

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
