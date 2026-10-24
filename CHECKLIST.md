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
- [ ] Conectar y desconectar redes mediante el gestor disponible en Armbian, con manejo seguro de credenciales.
- [ ] Validar de nuevo el indicador `CURRENT` tras un escaneo fresco en la consola.

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

- [ ] Inicializar `nexus-ui` con C++17, LVGL 9 y SDL2 para desarrollo de escritorio.
- [ ] Crear pantallas iniciales de Inicio, Configuración y Acerca de.
- [ ] Añadir navegación completa con controles físicos, foco visible y recuperación ante desconexión del daemon.
- [ ] Integrar llamadas RPC asíncronas de la UI con timeout, cancelación y errores visibles.
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
- No existe todavía una UI funcional.
