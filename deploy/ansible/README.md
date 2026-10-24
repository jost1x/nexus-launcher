# Despliegue remoto con Ansible

El playbook despliega los binarios ARM64 de `nexusd` y `nexusctl` a la cuenta SSH de la R36S, instala y habilita una unidad systemd para el daemon, y valida el RPC de salud por el socket UDS local. El daemon corre con la cuenta SSH de despliegue, con `CAP_NET_ADMIN` asignada por systemd para permitir el escaneo Wi-Fi y acceso suplementario al grupo `input` para leer botones. Una regla udev permite al grupo `input` escribir el atributo de brillo del backlight; `ReadWritePaths` exceptúa solo ese atributo de la protección de solo lectura sobre `/sys`. Los botones `F1 + Vol+/−` ajustan el brillo, si F1 corresponde al evento esperado en el hardware; `Vol+/−` ajustan el audio. La CLI sigue siendo de usuario y consulta salud, sistema, capacidades, Wi-Fi y estado de hardware por el socket local.

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

Si Ansible solicita contraseña para `sudo`, agrega `-K` al comando. El playbook copia los ejecutables a `~/.local/opt/nexusd`, crea enlaces `nexusd` y `nexusctl` en `~/.local/bin`, agrega ese directorio al `PATH` de login en `~/.profile`, instala la regla udev de backlight e instala la unidad de sistema `nexusd.service`. systemd ejecuta el daemon como el usuario SSH, le asigna `CAP_NET_ADMIN`, añade el grupo `input` y limita su conjunto de capacidades a `CAP_NET_ADMIN`. `ReadWritePaths` permite escribir solo el atributo de brillo dentro de `/sys`. La CLI no requiere `sudo`. Abre una nueva sesión SSH para usar `nexusctl` como comando. Para detener y deshabilitar el servicio:

```sh
ansible-playbook playbooks/stop.yml
```

Los logs del daemon se consultan con `sudo journalctl -u nexusd`; su socket UDS y estado quedan en `~/.cache/nexusd`. `playbooks/stop.yml` detiene y deshabilita el servicio, sin borrar binarios ni la unidad.

## Validación en consola

La consola ya tiene `iw` v5.19 en `/usr/sbin`; `nexusd` busca esta ruta aunque no forme parte del `PATH` del proceso. `CAP_NET_ADMIN` permite el escaneo al proceso `nexusd`. Es una capacidad amplia de administración de red, por lo que se concede únicamente a la unidad del daemon y no a `nexusctl` ni a la sesión SSH. La integración final deberá revisar si conviene mantener esta asignación o aislar el escaneo en un auxiliar más acotado.

2026-10-24: el despliegue en la R36S confirmó servicio `nexusd.service` activo como `r36s`, `CAP_NET_ADMIN` efectiva y RPC de salud respondiendo. La consola reportó Debian 12, kernel `6.18.58-current-rockchip64`, interfaz `wlan0` routable por `systemd-networkd`, batería RK817, backlight, sensor térmico y ALSA `Master`. `iw` v5.19 está en `/usr/sbin`. El primer escaneo excedió el timeout original de 3 segundos; se subió el default de `nexusctl` a 15 segundos. `nexusctl wifi scan` terminó correctamente en la consola. El parser del BSSID se corrigió para eliminar el sufijo `(on wlan0)`; el binario actualizado está copiado, pendiente reiniciar `nexusd` para que el cambio se active y validar la marca `CURRENT`. Los logs del servicio se consultan con `sudo journalctl -u nexusd`.
