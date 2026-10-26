# Despliegue remoto con Ansible

El playbook despliega los binarios ARM64 de `nexusd` y `nexusctl` a la cuenta SSH de la R36S, instala y habilita una unidad systemd para el daemon, y valida el RPC de salud por el socket UDS local. El daemon corre con la cuenta SSH, recibe `CAP_NET_ADMIN` y acceso suplementario a `input`, `audio`, `netdev` y `nexusd-backlight`. El playbook instala `rfkill` y configura `/dev/rfkill` con acceso para `netdev`; ese grupo también permite acceder al socket de control WPA. El estado, el escaneo y el comando para mantener Wi-Fi encendido ya respondieron en la R36S; falta probar apagar/encender y conectar perfiles desde la UI. Una regla udev limita la escritura al atributo de brillo; la unidad permite escritura solo en ese atributo y en el directorio de estado UDS. Una regla polkit autoriza al usuario SSH a solicitar únicamente reinicio y apagado mediante `systemd-logind`; `nexusd` usa `busctl` para llamar sus métodos D-Bus sin `sudo` y la CLI requiere `--confirm`. La captura en hardware identifica F como `BTN_TRIGGER_HAPPY5` (código 708), por lo que `F + Vol+/−` ajusta el brillo; `Vol+/−` ajusta el audio.

## Requisitos

- Ansible Core instalado en el equipo de desarrollo.
- Acceso SSH funcional a la R36S y su clave de host aceptada en `known_hosts`.
- La cuenta SSH puede ejecutar `sudo` para instalar y administrar `/etc/systemd/system/nexusd.service`.
- Los binarios ARM64 construidos desde `apps/nexusd`.

## Preparar el inventario

Desde la raíz de `nexus-launcher`:

```sh
cp deploy/ansible/inventory/hosts.example.yml deploy/ansible/inventory/hosts.yml
```

Edita `deploy/ansible/inventory/hosts.yml` y reemplaza `192.0.2.10` con la IP actual de la consola. El inventario local está excluido de Git. La clave SSH se obtiene del agente/configuración SSH local; no se almacenan contraseñas ni claves en este proyecto. Si se usa autenticación por contraseña, Ansible puede solicitarla con `--ask-pass`.

## Desplegar, iniciar y consultar salud

```sh
make -C apps/nexusd build-arm64
cd deploy/ansible
ansible-playbook playbooks/deploy.yml
```

Si Ansible solicita contraseña para `sudo`, agrega `-K` al comando. El playbook copia los ejecutables a `~/.local/opt/nexusd`, crea enlaces `nexusd` y `nexusctl` en `~/.local/bin`, agrega ese directorio al `PATH` de login en `~/.profile`, instala `polkitd`, la regla polkit para energía, la regla udev de backlight y la unidad de sistema `nexusd.service`. systemd ejecuta el daemon como el usuario SSH, con `CAP_NET_ADMIN` y los grupos suplementarios `input`, `audio` y `nexusd-backlight`; `ProtectSystem=strict` y `ReadWritePaths` restringen las escrituras al estado del socket y al brillo. `NoNewPrivileges=yes` y `RestrictSUIDSGID` permanecen activos; las solicitudes de energía pasan por los métodos D-Bus de `systemd-logind` y polkit, por lo que no dependen de `sudo` ni de elevar privilegios mediante setuid. La regla solo autoriza al usuario SSH las acciones `reboot`, `reboot-multiple-sessions`, `power-off` y `power-off-multiple-sessions`; no permite ignorar inhibidores ni ejecutar otros comandos privilegiados. La CLI normal no requiere `sudo`. Abre una nueva sesión SSH para usar `nexusctl` como comando. Para detener y deshabilitar el servicio:

```sh
ansible-playbook playbooks/stop.yml
```

Los logs del daemon se consultan con `sudo journalctl -u nexusd`; su socket UDS y estado quedan en `~/.cache/nexusd`. `playbooks/stop.yml` detiene y deshabilita el servicio, sin borrar binarios ni la unidad.

## Desplegar la interfaz en la R36S

La UI se despliega aparte del daemon, directamente en el framebuffer. El playbook instala un servicio systemd persistente y lo habilita al arranque. Prepara un bundle ARM64 en el equipo de desarrollo con esta estructura:

```text
build/docker/arm64/nexus-r36s-bundle/
├── nexus-ui
└── lib/
```

El binario debe estar compilado con `NEXUS_UI_FBDEV=ON`; `lib/` contiene bibliotecas ARM64 de ejecución que no estén disponibles en la consola. El directorio por defecto coincide con el bundle usado en las pruebas; se puede indicar otra ruta con `-e nexus_ui_bundle_dir=/ruta/al/bundle`.

Desde `deploy/ansible`:

```sh
ansible-playbook playbooks/deploy-ui.yml -K
```

El playbook comprueba que el destino sea ARM64, instala SDL2, gRPC C++ y Protobuf desde APT en Debian (incluidas sus dependencias) y copia solo las bibliotecas del bundle que `ldconfig` todavía no encuentra en la consola. Así evita enviar de nuevo dependencias que el sistema puede mantener como paquetes. Luego copia el ejecutable y las bibliotecas a `/opt/nexus-ui`, detiene la UI anterior con `SIGTERM`, devuelve la consola a la TTY 1 y abre la versión nueva en la TTY 2. También configura SDL como entrada de controles (`SDL_VIDEODRIVER=dummy`) y conecta la UI al socket de `nexusd`. La unidad `nexus-ui-r36s.service` queda habilitada al arranque y `systemd` la vuelve a iniciar si el proceso termina. Si no responde al detenerse, systemd fuerza el cierre después de 3 segundos. La aplicación cambia la TTY a modo gráfico mientras corre y restaura el modo texto al cerrarse. Se requiere `sudo` para acceder al framebuffer y cambiar la TTY.

`nexusd` escucha la combinación **F + L2 + R2 durante 3 segundos** para solicitar el reinicio de `nexus-ui-r36s.service`. Una regla polkit limita ese permiso al reinicio de esa unidad concreta. Los códigos previstos para L2/R2 son BTN_TL2/BTN_TR2; confirmar la combinación con los botones físicos en la consola sigue pendiente.

Para iterar más rápido después de una instalación completa, cuando SDL2, gRPC y Protobuf ya estén instalados en la consola, omite la comprobación y copia de dependencias:

```sh
ansible-playbook playbooks/deploy-ui.yml -K --skip-tags ui_dependencies
```

La ruta normal sigue preparando dependencias. En ambas rutas el playbook compara el ejecutable remoto y solo detiene/reinicia la UI cuando cambió algún archivo, la unidad no está activa o no está habilitada.

Para cerrar la UI desde la consola:

```sh
sudo systemctl stop nexus-ui-r36s.service
sudo chvt 1
```

Los mensajes del servicio quedan disponibles con `sudo journalctl -u nexus-ui-r36s.service`.

## Validación en consola

La consola ya tiene `iw` v5.19 en `/usr/sbin`; `nexusd` busca esta ruta aunque no forme parte del `PATH` del proceso. `CAP_NET_ADMIN` permite el escaneo al proceso `nexusd`. Es una capacidad amplia de administración de red, por lo que se concede únicamente a la unidad del daemon y no a `nexusctl` ni a la sesión SSH. El acceso a brillo y audio se otorga mediante grupos de dispositivo; el grupo dedicado de backlight solo puede escribir su atributo `brightness`. La política polkit concede únicamente reboot y poweroff al usuario SSH. Ansible comprueba esas cuatro autorizaciones con `pkcheck`, sin ejecutar las acciones, y confirma que no se autorizan comandos ajenos a ellas mediante `sudo`. La prueba funcional de reinicio/apagado debe hacerse aparte desde la CLI o el launcher con confirmación explícita y un procedimiento de recuperación.

2026-10-24: el despliegue en la R36S confirmó servicio `nexusd.service` activo como `r36s`, `CAP_NET_ADMIN` efectiva y RPC de salud respondiendo. La consola reportó Debian 12, kernel `6.18.58-current-rockchip64`, interfaz `wlan0` routable por `systemd-networkd`, batería RK817, backlight, sensor térmico y ALSA `Master`. `iw` v5.19 está en `/usr/sbin`. El primer escaneo excedió el timeout original de 3 segundos; se subió el default de `nexusctl` a 15 segundos. `nexusctl wifi scan` terminó correctamente en la consola. El parser del BSSID se corrigió para eliminar el sufijo `(on wlan0)`; el binario actualizado está copiado, pendiente reiniciar `nexusd` para que el cambio se active y validar la marca `CURRENT`. Los logs del servicio se consultan con `sudo journalctl -u nexusd`.

2026-10-26: el despliegue de Wi-Fi instaló `rfkill`, aplicó la regla udev de `/dev/rfkill` y mantuvo `netdev` en los grupos suplementarios de `nexusd`. La unidad y la UI quedaron activas. `nexusctl wifi status` informó `wlan0` conectado a `Dpto1401`, con radio disponible/encendida; `wifi radio on` respondió correctamente y un escaneo fresco marcó la red actual. Apagar el radio y conectar un perfil nuevo desde la UI quedan pendientes de validación manual para no cortar la sesión SSH.
