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

`nexusctl` es el cliente de `nexusd`; se ejecuta en la R36S y usa el socket UDS local. No se expone RPC por TCP. Los comandos disponibles son:

```sh
nexusctl health [SERVICE]
nexusctl system info
nexusctl system capabilities
nexusctl wifi status
nexusctl wifi scan [INTERFACE]
nexusctl wifi scan --refresh [INTERFACE]
nexusctl --format json wifi scan
nexusctl wifi scan
nexusctl hardware status
nexusctl --confirm hardware brightness set PERCENT
nexusctl --confirm hardware volume set PERCENT
nexusctl --confirm hardware reboot|poweroff
```

`health` acepta `nexus.v1.SystemService`, `nexus.v1.WifiService` y `nexus.v1.HardwareService`, además de la salud global sin argumento. Los errores RPC incluyen el código y el contexto de la operación. Las escrituras de brillo, volumen y energía requieren `--confirm`; la CLI y el daemon validan rangos y capacidades antes de actuar. No se guarda información de red ni se vuelcan secretos en logs.

En la R36S, `nexusd` escucha los botones del dispositivo Linux `gpio-keys`. `Vol+` y `Vol−` cambian el volumen ALSA en 5 puntos porcentuales; `F1 + Vol+` y `F1 + Vol−` cambian el brillo en el mismo paso. Mantener una tecla usa la repetición de eventos del kernel. El DTS candidato asigna F1 a `BTN_TRIGGER_HAPPY1`, pero la correspondencia física de esa tecla todavía requiere validación. La unidad systemd pertenece al grupo `input`; Ansible ajusta el permiso del atributo de backlight para que ese grupo pueda escribirlo. La CLI también permite cambiar ambos valores con `--confirm`.

La salida predeterminada es una tabla legible; usa `--format json` cuando necesites JSON, por ejemplo para scripts. `nexusctl wifi scan` ordena las redes por señal descendente y luego por SSID; las redes sin señal reportada aparecen al final. La columna `CURRENT` marca el punto de acceso al que está asociada la consola, comparando BSSID. `hardware status` indica porcentaje y estado de carga de la batería, además de si hay un cargador detectado y conectado. El formato se indica antes del subcomando, por ejemplo `nexusctl --format json wifi scan`.

`nexusd` conserva los resultados del último escaneo Wi-Fi en memoria durante 15 segundos por interfaz. Una llamada normal reutiliza esos resultados dentro de ese plazo; `nexusctl wifi scan --refresh` fuerza un escaneo nuevo. La respuesta incluye `scanned_at_unix` y `from_cache`; la tabla también muestra cuándo se escaneó y su antigüedad. Los resultados no se escriben en disco.

## Desarrollo local

Desde este directorio, `make build` compila ambos binarios para la plataforma local y `make build-arm64` los compila para Linux ARM64. `make fmt` aplica `gofmt`; `make vet` ejecuta `go vet ./...`.

El módulo requiere Go 1.26 o posterior, según la versión de las dependencias fijadas en `go.mod`.

Por defecto, cliente y daemon usan `$XDG_CACHE_HOME/nexusd/nexusd.sock` (o la ruta de caché del usuario si no se define `XDG_CACHE_HOME`). Se puede cambiar con `--socket PATH` o `NEXUSD_SOCKET`. El daemon crea el directorio con permisos privados y el socket con modo `0600`, de modo que el prototipo puede ejecutarse como el mismo usuario por SSH sin root.

```sh
make build
./bin/nexusd
./bin/nexusctl health
```

Para contratos, instala Buf v1.75.0, `protoc-gen-go` v1.36.11 y `protoc-gen-connect-go` v2.0.0; luego ejecuta `make lint-proto` y `make generate`. El código generado se conserva en el repositorio para builds sin generadores.

## Flujo de prototipado en hardware

Durante el prototipo se compila y despliega `nexusd` y `nexusctl` en la R36S mediante SSH. Ansible instala una unidad systemd de desarrollo para conceder al daemon `CAP_NET_ADMIN`; es una modificación persistente de la consola, separada del build de la imagen Armbian, y se puede detener/deshabilitar con `deploy/ansible/playbooks/stop.yml`. Las credenciales y secretos de acceso no se guardan en el repositorio.

## Fases

1. **Base:** proceso, UDS, gRPC Health, logging, cierre ordenado y CLI. Validado localmente y desplegado por Ansible en la R36S.
2. **Sistema y capacidades:** información Linux desde `os-release`/`procfs`; capacidades detectadas desde sysfs y herramientas disponibles, con indicador de detección observada en R36S.
3. **Wi-Fi:** estado de interfaz, SSID asociado y escaneo cancelable con `iw`, con errores claros cuando falten herramienta, interfaz o permisos. La consola ya tiene `iw` v5.19 en `/usr/sbin`; `nexusd` busca también en las rutas estándar de `sbin`, aunque no estén en el `PATH` del proceso. El escaneo necesita permiso de administración de red. La conexión/desconexión de redes queda supeditada a un mecanismo de Armbian que permita tratar las credenciales de forma segura.
4. **Hardware:** lecturas de batería, estado de carga/cargador, temperatura, backlight y ALSA; controles de brillo y volumen con confirmación; reinicio/apagado explícitos y confirmados.
5. Completar pruebas del daemon y la CLI por SSH en la R36S. La integración persistente, empaquetado e inicio en la imagen Armbian pertenecen a la Fase 8 del proyecto.

No asumir NetworkManager, rutas sysfs, controles ALSA ni permisos root. Usar `r36s-os` como referencia de evidencia del sistema y hardware, indicar explícitamente las capacidades no soportadas y separar pruebas de escritorio, SSH y hardware.

### Observaciones en la consola

En una consulta SSH del 2026-10-24, `/proc/device-tree/model` identificó `R36S (Panel 4 software profile)` y `/etc/os-release` informó Debian 12 con kernel `6.18.58-current-rockchip64`. `nexusctl health` y la salud de los tres servicios, además de `system info`, `system capabilities`, `wifi status` y `hardware status`, respondieron por UDS en la consola. `wlan0` está routable según `systemd-networkd`. `iw` v5.19 está instalado en `/usr/sbin`; `nexusd` busca esta ruta aunque no esté en el `PATH`. Ansible instaló y activó `nexusd.service` con `CAP_NET_ADMIN`; el proceso corre como `r36s`, con esa capacidad efectiva. Tras aumentar el timeout predeterminado de la CLI a 15 segundos, `nexusctl wifi scan` completó por UDS. La tabla marca la red actual usando el BSSID. El parser se corrigió para quitar el sufijo `(on wlan0)`; el binario actualizado está copiado en la consola, pendiente reiniciar `nexusd` para validar la marca. Las lecturas observadas incluyeron batería al 100 %, temperatura SoC alrededor de 53 °C, brillo 833/1666 y volumen ALSA 79 %. `--confirm hardware volume set 79` respondió correctamente y mantuvo la lectura en 79 %. La consola expone `gpio-keys` como `/dev/input/event0`; el mapeo real de botones a eventos físicos todavía requiere validación manual. El cambio de brillo al 50 % fue rechazado por permisos de sysfs. No se ejecutaron reboot/poweroff.
