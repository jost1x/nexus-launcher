# NEXUS checklist

Estado de las funcionalidades de NEXUS. La marca `[x]` indica implementación y verificación según la nota; `[ ]` indica trabajo pendiente. Las pruebas en escritorio, despliegue por SSH y validación física se registran por separado.

## Herramientas y flujo de desarrollo

- [x] Monorepo organizado por aplicación, contratos, despliegue, herramientas y documentación.
- [x] Reglas de trabajo en `AGENTS.md`, incluidos Conventional Commits en inglés separados por responsabilidad.
- [x] Hooks locales configurados con el framework `pre-commit`.
- [x] Compilación de `nexusd` y `nexusctl` para Linux AMD64 y ARM64.
- [x] Despliegue de prototipos a la R36S por SSH con Ansible, independiente del build de la imagen.
- [x] Unidad systemd de desarrollo para `nexusd`, socket UDS, permisos de hardware y consulta de logs.
- [ ] GitHub Actions; se pospone hasta la fase final.

## `nexusd` y `nexusctl`

- [x] Daemon Go y RPC compatible con gRPC sobre socket UDS local.
- [x] Salud global y por servicio; comprobada en la R36S.
- [x] CLI `nexusctl` con salida tabla predeterminada y JSON opcional.
- [x] Errores RPC con código y contexto de operación.
- [x] Información del sistema y detección de capacidades; consultadas por SSH en la R36S.
- [x] Logs del daemon disponibles con `journalctl -u nexusd`.
- [ ] Verificar interoperabilidad de un cliente C++ con Connect-Go/gRPC sobre UDS.

## Wi-Fi

- [x] Consultar el estado de la interfaz y la red asociada.
- [x] Escanear redes con `iw`; el escaneo respondió en la R36S con permisos limitados a `nexusd`.
- [x] Ordenar resultados por señal y SSID, e identificar la red actual por BSSID.
- [x] Cache en memoria de resultados durante 15 segundos; `--refresh` solicita un escaneo nuevo.
- [x] Activar/desactivar el radio mediante `rfkill` y reportar cuándo el estado no se puede detectar.
- [x] Conectar y guardar perfiles abiertos y WPA-PSK mediante el socket de control de `wpa_supplicant`.
- [x] Conservar perfiles en archivo privado `0600`, no devolver contraseñas por RPC y restaurarlos al iniciar `nexusd`.
- [x] Ordenar perfiles guardados, conectar a uno guardado, seleccionar preferida y olvidar redes.
- [x] Desplegar en la R36S `nexusd` y la UI con Ansible; ambos servicios quedaron activos.
- [x] Confirmar en la R36S estado Wi-Fi conectado, radio disponible/encendida y respuesta correcta a `wifi radio on`.
- [x] Confirmar respuesta `PONG` del socket WPA al ejecutarlo como `r36s` en el grupo `netdev`.
- [x] Ejecutar un escaneo fresco en la consola y confirmar que `CURRENT` identifica `Dpto1401`.
- [ ] Probar apagar y volver a encender el radio desde la pantalla, con observación física y recuperación local disponible.
- [ ] Añadir y conectar un perfil WPA desde la UI; comprobar reconexión, preferencia y olvido de redes.

## Hardware

- [x] Consultar batería, estado de carga, conexión del cargador, temperatura SoC, brillo y volumen.
- [x] Ajustar volumen con confirmación desde `nexusctl`.
- [x] Ajustar brillo con confirmación desde `nexusctl`; Ansible configura acceso al atributo de backlight.
- [x] Control físico de volumen: `Vol+` y `Vol−` cambian el nivel en pasos de 5 puntos porcentuales.
- [x] Control físico de brillo: `F + Vol+` y `F + Vol−` cambian el nivel en pasos de 5 puntos porcentuales.
- [x] Confirmar en la R36S que F genera el evento Linux `BTN_TRIGGER_HAPPY5` (código 708); volumen genera los códigos 115 y 114.
- [x] Detectar carga con `current_avg` positivo cuando el cargador está conectado, como respaldo para el estado `Not charging` incorrecto del driver RK817.
- [x] Leer `voltage_avg` cuando el driver no expone `voltage_now`.
- [x] Validar después del despliegue que `nexusctl hardware status` reporte `Charging` y el voltaje real en la consola.
- [x] Registrar estados reales con el cargador conectado y desconectado; verificar transiciones entre `Charging` y `Discharging`.
- [ ] Registrar el estado de batería cuando la carga haya terminado.
- [ ] Probar reinicio y apagado confirmados en hardware, con un procedimiento de recuperación.
- [ ] Determinar por qué `voltage_now` de batería reporta `0` en la consola, o documentar que no está disponible.

## Interfaz y preferencias

- [x] Inicializar `nexus-ui` con C++17, LVGL 9.2.2 y SDL2; CMake obtiene LVGL en una revisión fijada.
- [x] Crear vistas de Inicio, Wi-Fi, Pantalla, Audio, Sistema y Acerca de con navegación por ratón, teclado y mando SDL.
- [x] Revisar las seis vistas renderizadas por SDL a 640×480; comprobar cambio de página con `Tab`/`Enter`, `Esc`, ratón y cierre de ventana mediante eventos SDL locales.
- [x] Generar stubs C++ desde el Protobuf compartido e integrar gRPC C++ sobre UDS; comprobado con `SystemService.GetInfo`, `WifiService.GetStatus`, `WifiService.Scan` y `HardwareService.GetStatus`, contra `nexusd` local y el daemon de la R36S por túnel SSH UDS.
- [x] Añadir navegación con teclado, foco visible, equivalentes de cruceta/A/B y soporte de ratón.
- [x] Ejecutar RPC desde un worker con deadline, cancelación de escaneo al salir de Wi-Fi y entrega de resultados al hilo LVGL.
- [x] Crear vistas Inicio, Wi-Fi (estado y escaneo), Pantalla (brillo), Audio (volumen), Sistema e Información.
- [x] Añadir estados de daemon desconectado, carga, lista vacía, error y resultados cacheados.
- [x] Ajustar brillo y volumen desde las acciones de la UI contra el daemon de la R36S: probar 99 %/79 % y restaurar los valores originales 100 %/80 %; el daemon confirmó la lectura final.
- [x] Validar la detección SDL física de cruceta/A/B en la R36S y ajustar su puente. Una captura SDL 2.26.5 registró ↑=8, ↓=9, ←=10, →=11, B=1 y A=0 en `gpio-keys`, en coincidencia con el mapeo de la UI.
- [x] Revisar visualmente las seis vistas a 640×480 con lecturas reales de Wi-Fi, batería, carga, temperatura, brillo, volumen y sistema obtenidas de la R36S.
- [ ] Guardar preferencias de UI en JSON con validación y escritura atómica.

## Integración y estabilización de Armbian

- [ ] Preparar compilación cruzada y toolchain de UI para el entorno Armbian real.
- [ ] Empaquetar e integrar `nexusd`, `nexusctl` y `nexus-ui` en la imagen propia `r36s-os`.
- [ ] Definir instalación, actualización, desinstalación, permisos, inicio automático y recuperación.
- [ ] Medir uso de CPU y memoria en reposo y bajo carga, y revisar tiempos de respuesta en hardware.
- [ ] Completar la validación de la versión 0.1 en la consola y documentar limitaciones observadas.
- [ ] Añadir GitHub Actions al final, cuando las verificaciones locales estén estabilizadas.

## Evidencia de validación

- Las consultas de salud, sistema, Wi-Fi y hardware se realizaron por SSH contra la R36S con Armbian.
- El daemon y la CLI se compilaron para Linux ARM64.
- Los controles físicos de volumen y brillo se probaron en la consola; la combinación de brillo usa el evento F medido en hardware.
- Una muestra de 25 s con cargador conectado mostró `charger_online=1`, `current_avg` de aproximadamente 250 mA y `charge_now` en aumento (+1548 µAh); el kernel seguía reportando `Not charging`.
- Prueba física por SSH (2026-10-24): al conectar el cable, `CHARGER CONNECTED` cambió a `true` y después `BATTERY STATUS`/`BATTERY CHARGING` a `Charging`/`true`, con voltaje de batería de aproximadamente 4.196 V. Al desconectarlo, `CHARGER CONNECTED` volvió a `false` y el estado pasó a `Discharging`; el voltaje bajó a aproximadamente 4.032 V. Las lecturas muestran que la detección de `Charging` puede tardar unos segundos después de conectar.
- El escaneo Wi-Fi requiere `CAP_NET_ADMIN`, asignada a la unidad del daemon por Ansible.
- Prueba de reinicio por software (2026-10-24): `nexusctl --confirm hardware reboot` fue aceptado por `systemd-logind` y SSH se desconectó; al minuto la R36S no respondía en la IP ni apareció otra IP con SSH en la LAN. Se recuperó con el botón físico de reset. El reinicio automático queda pendiente de diagnosticar en `r36s-os`; apagado no probado.
- La UI se compiló desde una configuración limpia y se renderizó en SDL a 640×480 sin daemon y conectada al daemon real de la R36S mediante un túnel SSH al socket UDS.
- La respuesta de escaneo vacía se representó en su estado dedicado; la lista Wi-Fi se comprobó con error, resultados reales y caché.
- Prueba en R36S (2026-10-25): la UI consultó y mostró estado Wi-Fi, redes detectadas, red actual, señal y seguridad; una segunda consulta confirmó resultados en caché. También mostró batería/carga, temperatura, sistema, brillo y volumen.
- Despliegue Wi-Fi en R36S (2026-10-26): se compilaron y desplegaron `nexusd`, `nexusctl` y la UI ARM64. Ansible instaló `rfkill`, añadió el grupo `netdev` al daemon y aplicó una regla udev para `/dev/rfkill`. El RPC de salud respondió `serving`; la UI y `nexusd` quedaron activos. `nexusctl wifi status` informó `wlan0`, conexión a `Dpto1401`, radio disponible y encendida; `wifi radio on` respondió correctamente, el socket WPA devolvió `PONG` para el usuario `r36s` en `netdev` y un escaneo fresco volvió a marcar la red actual. No se probó apagar el radio ni conectar una red nueva para conservar la conexión SSH.
- Prueba de ajustes en R36S (2026-10-25): la UI solicitó brillo 99 % y volumen 79 %, recibió confirmación y restauró brillo 100 % y volumen 80 %. La lectura final de `nexusctl hardware status` confirmó ambos valores.
- Prueba SDL temporal en R36S (2026-10-25): sin instalar paquetes, se extrajo SDL 2.26.5 y sus dependencias en `/tmp`; SDL enumeró `adc-joystick` (0 botones) y `gpio-keys` (19 botones). Una captura registró en orden las pulsaciones ↑=8, ↓=9, ←=10, →=11, A=0 y B=1; el orden izquierdo/derecho de SDL difiere del mapa de `/dev/input/js0`. Se retiraron los archivos temporales. La prueba del movimiento visual dentro de `nexus-ui` se reserva para la integración Armbian de la fase 8.
