# nexusd

Daemon único en Go para exponer capacidades de Linux/Armbian a la interfaz. Se organiza en servicios y adaptadores con interfaces claras; no administra temas ni otras preferencias visuales.

## Responsabilidades

- Servir contratos Protobuf mediante Connect-Go compatible con gRPC.
- Emitir una instantánea inicial y cambios de Wi-Fi y hardware por `WifiService.WatchStatus` y `HardwareService.WatchStatus`; notificar operaciones propias y botones físicos, con comparación cada dos segundos para detectar cambios externos.
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
nexusctl wifi radio on|off
nexusctl wifi saved
nexusctl wifi connect-saved ID
nexusctl wifi prefer ID
nexusctl wifi forget ID
nexusctl --format json wifi scan
nexusctl wifi scan
nexusctl hardware status
nexusctl --confirm hardware brightness set PERCENT
nexusctl --confirm hardware volume set PERCENT
nexusctl --confirm hardware reboot|poweroff
```

`health` acepta `nexus.v1.SystemService`, `nexus.v1.WifiService` y `nexus.v1.HardwareService`, además de la salud global sin argumento. Los errores RPC incluyen el código y el contexto de la operación. Las escrituras de brillo, volumen y energía requieren `--confirm`; la CLI y el daemon validan rangos y capacidades antes de actuar. La contraseña de una red nueva se solicita en la UI; no se imprime en respuestas RPC ni en logs.

### Wi-Fi

El estado del radio se consulta con `rfkill`; la UI desactiva el control si el sistema no informa ese estado. El daemon conecta redes mediante el socket de control de `wpa_supplicant`, compatible con el backend Netplan/systemd-networkd de la imagen R36S. Para acceder al socket de control y a `/dev/rfkill`, la unidad desplegada incluye el grupo `netdev`. Si falta `rfkill`, la interfaz no tiene socket WPA accesible o los permisos no bastan, la operación devuelve un error RPC y el estado no se presenta como cambiado.

Las redes agregadas desde la UI se guardan por orden de preferencia en `$XDG_CACHE_HOME/nexusd/wifi-profiles.json` (o `~/.cache/nexusd/wifi-profiles.json`), dentro de un directorio `0700` y un archivo `0600`. Las credenciales quedan en texto claro dentro de ese archivo, accesible solo a la cuenta que ejecuta `nexusd`; no se copian a respuestas, argumentos de procesos ni logs. La red que se conecta o marca como preferida pasa al primer lugar y se vuelve a instalar en `wpa_supplicant` al iniciar el daemon. Los perfiles siguientes quedan disponibles como respaldo. Se admiten redes abiertas y WPA-PSK; Enterprise y autenticación WPA3-SAE aún no están implementados.

La configuración inicial creada por Netplan continúa intacta. No se importa su contraseña a la lista de NEXUS; para administrar esa red desde la UI, el usuario debe seleccionarla y registrarla ahí.

En la R36S, `nexusd` escucha los botones del dispositivo Linux `gpio-keys`. `Vol+` y `Vol−` cambian el volumen ALSA en 5 puntos porcentuales; `F + Vol+` y `F + Vol−` cambian el brillo en el mismo paso. Mantener una tecla usa la repetición de eventos del kernel. La captura en hardware identificó el botón F como código 708 (`BTN_TRIGGER_HAPPY5`); el DTS candidato nombra esa entrada F5. La unidad systemd pertenece al grupo `input`; Ansible ajusta el permiso del atributo de backlight para que ese grupo pueda escribirlo. La CLI también permite cambiar ambos valores con `--confirm`.

Para batería, `nexusd` usa `voltage_now` y recurre a `voltage_avg` si el driver no expone el primer atributo. En la R36S se observó que `charge_now` y `current_avg` subían mientras el driver informaba `Not charging`; en ese caso NEXUS infiere `Charging` si el cargador está conectado y la corriente media de la batería es positiva.

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

Durante el prototipo se compila y despliega `nexusd` y `nexusctl` en la R36S mediante SSH. Ansible instala una unidad systemd para conceder al daemon `CAP_NET_ADMIN`, acceso de lectura a botones y ALSA, y escritura al atributo de brillo mediante un grupo dedicado. Una regla polkit limitada autoriza a `nexusd` a solicitar reboot y poweroff mediante los métodos D-Bus de `systemd-logind`, sin contraseña ni `sudo`; la CLI mantiene `--confirm`. Estas modificaciones persistentes de la consola son independientes del build de la imagen Armbian y se pueden detener/deshabilitar con `deploy/ansible/playbooks/stop.yml`. Las credenciales y secretos de acceso no se guardan en el repositorio.

## Fases

1. **Base:** proceso, UDS, gRPC Health, logging, cierre ordenado y CLI. Validado localmente y desplegado por Ansible en la R36S.
2. **Sistema y capacidades:** información Linux desde `os-release`/`procfs`; capacidades detectadas desde sysfs y herramientas disponibles, con indicador de detección observada en R36S.
3. **Wi-Fi:** estado del radio, redes guardadas, conexión y preferencia por medio de `wpa_supplicant`, interruptor con `rfkill` y escaneo cancelable con `iw`. La consola ya tiene `iw` v5.19 en `/usr/sbin`; `nexusd` busca también en las rutas estándar de `sbin`, aunque no estén en el `PATH` del proceso. El acceso al control WPA y al radio depende de los permisos del sistema y se informa como no disponible cuando no se cumplen.
4. **Hardware:** lecturas de batería, estado de carga/cargador, temperatura, backlight y ALSA; controles de brillo y volumen con confirmación; reinicio/apagado explícitos y confirmados.
5. Completar pruebas del daemon y la CLI por SSH en la R36S. La integración persistente, empaquetado e inicio en la imagen Armbian pertenecen a la Fase 8 del proyecto.

No asumir NetworkManager, rutas sysfs, controles ALSA ni permisos root. Usar `r36s-os` como referencia de evidencia del sistema y hardware, indicar explícitamente las capacidades no soportadas y separar pruebas de escritorio, SSH y hardware.

### Observaciones en la consola

En una consulta SSH del 2026-10-24, `/proc/device-tree/model` identificó `R36S (Panel 4 software profile)` y `/etc/os-release` informó Debian 12 con kernel `6.18.58-current-rockchip64`. `nexusctl health` y la salud de los tres servicios, además de `system info`, `system capabilities`, `wifi status` y `hardware status`, respondieron por UDS en la consola. `wlan0` está routable según `systemd-networkd`. `iw` v5.19 está instalado en `/usr/sbin`; `nexusd` busca esta ruta aunque no esté en el `PATH`. Ansible instaló y activó `nexusd.service` con `CAP_NET_ADMIN`; el proceso corre como `r36s`, con esa capacidad efectiva. Tras aumentar el timeout predeterminado de la CLI a 15 segundos, `nexusctl wifi scan` completó por UDS. El parser se corrigió para quitar el sufijo `(on wlan0)`; el daemon actualizado y reiniciado confirmó después la red actual por BSSID.

Despliegue Wi-Fi del 2026-10-26: Ansible instaló `rfkill`, asignó el acceso a `/dev/rfkill` al grupo `netdev` y configuró ese grupo en `nexusd.service`. La radio se detecta encendida, `wifi radio on` responde correctamente y un escaneo fresco marca la red actual. El socket `/run/wpa_supplicant/wlan0` respondió `PONG` al usuario del daemon con el grupo `netdev`. La UI y `nexusd` quedaron activos en la consola. Aún no se probó apagar la radio ni crear/conectar otro perfil desde la UI.

Las lecturas observadas incluyeron batería al 100 %, temperatura SoC alrededor de 53 °C, brillo 833/1666 y volumen ALSA 79 %. `--confirm hardware volume set 79` respondió correctamente y mantuvo la lectura en 79 %. La consola expone `gpio-keys` como `/dev/input/event0`; el mapeo real de botones a eventos físicos todavía requiere validación manual. El cambio de brillo al 50 % fue rechazado por permisos de sysfs. No se ejecutaron reboot/poweroff.
