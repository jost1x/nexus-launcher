# Contratos Protobuf

Los archivos `.proto` de esta carpeta son la fuente canónica de los servicios entre `nexus-ui` y `nexusd`. Los contratos actuales están en `nexus.v1` e incluyen salud gRPC estándar más sistema, capacidades, estado/escaneo Wi-Fi y hardware. El estado Wi-Fi informa el SSID y BSSID asociados; los resultados de escaneo incluyen BSSID y marcan el punto de acceso actual. Las solicitudes de escaneo pueden forzar una actualización; las respuestas indican la hora y si se usó caché. El estado de hardware informa porcentaje/estado de batería y detección/conexión del cargador.

## Validación y generación

Instala Buf v1.75.0, `protoc-gen-go` v1.36.11 y `protoc-gen-connect-go` v2.0.0 en el `PATH`. Desde la raíz del repositorio:

```sh
make -C apps/nexusd lint-proto
make -C apps/nexusd generate
```

El código generado Go queda en `apps/nexusd/gen/nexus/v1`; no se deben editar esos archivos manualmente. La generación C++ se añadirá cuando comience la implementación de `nexus-ui`.

Evitar eventos de navegación e interacción visual en estos contratos y no duplicar definiciones entre aplicaciones.
