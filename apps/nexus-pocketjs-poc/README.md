# PocketJS en R36S: prueba de host y gamepad

Esta app mínima comprueba que PocketJS puede ejecutar un paquete `.pocket` en la R36S con resolución 640×480, framebuffer y botones. No reemplaza NEXUS UI ni implementa un adaptador gRPC.

## Resultado de la prueba

- Manifiesto validado y paquete compilado con PocketJS 0.15.0.
- Runtime Linux ARM64 compilado en el PC con el sysroot Debian Bookworm.
- Ejecutado temporalmente en la R36S desde `/tmp`, con acceso a `/dev/fb0` y los dispositivos `/dev/input/event*`.
- La app apareció en pantalla y el runtime completó 600 ciclos lógicos a 60 Hz.
- Un evento sintético `BTN_SOUTH`/`CROSS` enviado por uinput elevó a 1 el contador del hook de botones de PocketJS. También se inyectó `BTN_CIRCLE`, pero no se pudo confirmar la acción del control enfocable: la captura posterior quedó negra.
- No se instalaron servicios ni se modificó la configuración del sistema.

## Medición de CPU en reposo

Muestras de 5 segundos tomadas mientras NEXUS UI seguía ejecutándose en segundo plano:

| Perfil PocketJS | CPU de PocketJS | RAM residente |
| --- | ---: | ---: |
| 60 Hz | 92,4% de un núcleo | 8,9 MiB |
| 30 Hz | 92,6% de un núcleo | 8,4 MiB |

El runtime a 30 Hz completó 1109 ciclos en 39 segundos (≈28,4 Hz). Reducir la frecuencia no redujo el uso de CPU; el siguiente análisis debe centrarse en el trabajo de cada ciclo/render. En la muestra simultánea, `nexus-ui` usó 2,8–3,0% de un núcleo y `nexusd` 1,4–2,2%.

La compilación usó el checkout de PocketJS clonado en `build/pocketjs-upstream` (directorio ignorado) y el perfil de este proyecto en `profile.ts`.

## App

`main.tsx` muestra un tester de gamepad con D-pad, botones de acción, hombros y controles de sistema. Cada control cambia de estado visual mientras se mantiene presionado; también se muestra el último botón detectado y el total de pulsaciones. `profile.ts` fija el viewport físico y lógico a 640×480 y requiere entrada de botones y glifos empaquetados.

La app muestra los controles recibidos, el último código Linux `EV_KEY` (decimal y hexadecimal) y una señal para códigos aún no asignados. No muestra todavía ejes analógicos ni mide duración, repetición o latencia.

## Tester de gamepad (2026-10-26)

- `pocket check` validó el manifiesto, el perfil R36S y los tipos de TypeScript.
- `pocket compile` generó `build/nexus-pocketjs-poc-gamepad.pocket` (413 KiB; 244 KiB de recursos empaquetados).
- El runtime aceptó el paquete en la R36S y lo mostró a 640×480 usando fbdev. Se detuvo `nexus-ui-r36s.service` durante la prueba y quedó activo otra vez al terminar.
- Con eventos evdev sintéticos de uinput se iluminaron correctamente las 11 entradas que el host PocketJS mapea: D-pad, cuatro botones frontales, L/R y Start. Las capturas están en `build/nexus-gamepad-captures/all-controls.png`.
- Esta fue la prueba inicial con el mapa genérico del host. La prueba física posterior y el mapa específico de la R36S se documentan abajo.
- `pocket.runtime-config.ts` conecta el perfil propio `r36s` con el flujo actual de compilación PocketJS.

## Mapeo físico R36S (2026-10-27)

Durante la prueba física, se observó que el host etiquetaba varios controles como `SELECT`. El tester ahora muestra el código Linux `EV_KEY` de cada tecla. “Confirmado” significa que el usuario pulsó el control en esta unidad y comunicó el ID que mostró la pantalla. Los demás códigos provienen del DTB/hardware inventory y están marcados como referencia.

| Control físico | `EV_KEY` decimal | Hexadecimal | Entrada PocketJS / pantalla | Fuente |
| --- | ---: | ---: | --- | --- |
| D-pad arriba | 544 | `0x220` | `UP` | DTB |
| D-pad abajo | 545 | `0x221` | `DOWN` | DTB |
| D-pad izquierda | 546 | `0x222` | `LEFT` | DTB |
| D-pad derecha | 547 | `0x223` | `RIGHT` | DTB |
| A | 305 | `0x131` | `CIRCLE` / `A` | DTB |
| B | 304 | `0x130` | `CROSS` / `B` | DTB |
| X | 307 | `0x133` | `SQUARE` / `X` | DTB; etiqueta corregida tras la prueba |
| Y | 308 | `0x134` | `TRIANGLE` / `Y` | DTB; etiqueta corregida tras la prueba |
| L1 | 310 | `0x136` | `LTRIGGER` / `L1` | DTB |
| R1 | 311 | `0x137` | `RTRIGGER` / `R1` | DTB |
| L2 / ZL | 706 | `0x2c2` | `ZL` / `L2` | Confirmado |
| R2 / ZR | 707 | `0x2c3` | `ZR` / `R2` | Confirmado |
| SELECT | 704 | `0x2c0` | `SELECT` | Confirmado |
| START | 705 | `0x2c1` | `START` | Confirmado |
| FN | 708 | `0x2c4` | `FN` | Confirmado |
| + / volumen arriba | 115 | `0x073` | `PLUS` | Confirmado |
| − / volumen abajo | 114 | `0x072` | `MINUS` | Confirmado |
| Cualquier otro código | Variable | Variable | `DESCONOCIDO` + ID | Diagnóstico |

La lectura del árbol activo (`/sys/firmware/fdt`) confirma que sus nodos llaman F1–F5 a los códigos 704–708 (`BTN_TRIGGER_HAPPY1`–`BTN_TRIGGER_HAPPY5`). La prueba física identifica esos mismos códigos como SELECT, START, ZL, ZR y FN. Por tanto, el kernel sí entrega códigos distintos; los nombres F1–F5 del DTB son genéricos y no describen las funciones físicas de esta unidad.

El tester usa bits de extensión propios para mostrar FN, PLUS y MINUS por separado, y conserva el ID de la última tecla en pantalla. Los controles frontales y el D-pad se basan en el DTB/hardware inventory; esos IDs son referencia y no pulsaciones confirmadas en esta sesión. La asignación de X/Y refleja el cruce observado en la primera prueba.

Los cambios del puente evdev del prototipo están en el checkout generado bajo `build/pocketjs-upstream` y no son parte del runtime del sistema. El servicio `nexus-ui-r36s.service` se detiene solo mientras el tester temporal corre; el supervisor de prueba lo inicia al salir. No cambiar `linux,code` en el DTB hasta verificar si los códigos 312/313 corresponden a otros botones físicos; cambiarlo sin esa comprobación podría hacer que dos controles compartan el mismo evento.

## Limpieza local

Las dependencias instaladas para esta app y los datos de build de PocketJS son locales. `.gitignore` excluye `node_modules/` y `.pocket/`; la raíz del proyecto excluye `build/`, donde se guardan el checkout upstream, sysroot, ejecutables y capturas.
