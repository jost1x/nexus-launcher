# nexusd

Daemon único en Go para exponer capacidades de Linux/Armbian a la interfaz. Se organiza en servicios y adaptadores con interfaces claras; no administra temas ni otras preferencias visuales.

## Responsabilidades

- Servir contratos Protobuf mediante Connect-Go compatible con gRPC.
- Exponer HTTP/2 sobre Unix Domain Socket con configuración explícita sin TLS.
- Aislar integración de sistema y hardware detrás de adaptadores detectables.
- Aplicar timeouts, cancelación, errores tipados y logging estructurado con `log/slog`.
- Cerrar ordenadamente y no registrar secretos ni ejecutar comandos arbitrarios.

El formato y lint del código Go se ejecutan mediante los hooks pre-commit definidos en la raíz del repositorio.

## CLI de desarrollo

`nexusctl` es el cliente de `nexusd` para consultar salud, capacidades y los servicios disponibles conforme se implementen. En esta primera fase incluye `nexusctl health [SERVICE]`; sin `SERVICE` consulta la salud del daemon. Se ejecuta en la R36S y usa el socket UDS local; los fallos de conexión y errores RPC incluyen contexto para diagnóstico. No se expone RPC por TCP.

## Desarrollo local

Desde este directorio, `make build` compila ambos binarios para la plataforma local y `make build-arm64` los compila para Linux ARM64. `make fmt` aplica `gofmt`; `make vet` ejecuta `go vet ./...`.

El módulo requiere Go 1.26 o posterior, según la versión de las dependencias fijadas en `go.mod`.

Por defecto, cliente y daemon usan `$XDG_CACHE_HOME/nexusd/nexusd.sock` (o la ruta de caché del usuario si no se define `XDG_CACHE_HOME`). Se puede cambiar con `--socket PATH` o `NEXUSD_SOCKET`. El daemon crea el directorio con permisos privados y el socket con modo `0600`, de modo que el prototipo puede ejecutarse como el mismo usuario por SSH sin root.

```sh
make build
./bin/nexusd
./bin/nexusctl health
```

## Flujo de prototipado en hardware

Durante el prototipo se podrá compilar, copiar y ejecutar `nexusd` y `nexusctl` en la R36S mediante SSH, consultar logs y repetir las pruebas. Este despliegue de desarrollo es temporal y separado de la imagen Armbian; no implica instalar servicios ni modificar la configuración persistente del sistema. Las credenciales y secretos de acceso no se guardan en el repositorio.

## Fases

1. Establecer proceso, transporte UDS, servicio estándar gRPC Health, logging y cierre ordenado; validar RPC con `nexusctl`. Esta fase inicia antes del desarrollo de `nexus-ui`.
2. Añadir métricas de sistema y capacidades con adaptadores Linux y mocks; consultables desde `nexusctl`.
3. Añadir Wi-Fi tras verificar los mecanismos disponibles en Armbian; admitir ausencia de adaptador y cancelación, con operaciones accesibles desde `nexusctl`.
4. Añadir audio, pantalla y energía con validación y confirmación para acciones destructivas; exponer las operaciones soportadas en `nexusctl`.
5. Completar pruebas del daemon y la CLI por SSH en la R36S. La integración persistente, empaquetado e inicio en la imagen Armbian pertenecen a la Fase 8 del proyecto.

No asumir NetworkManager, rutas sysfs, controles ALSA ni permisos root. Usar `r36s-os` como referencia de evidencia del sistema y hardware, indicar explícitamente las capacidades no soportadas y separar pruebas de escritorio, SSH y hardware.
