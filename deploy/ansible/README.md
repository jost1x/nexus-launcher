# Despliegue remoto con Ansible

El playbook despliega los binarios ARM64 de `nexusd` y `nexusctl` a la cuenta SSH de la R36S, inicia el daemon como proceso temporal y valida el RPC de salud por el socket UDS local. No requiere privilegios root, instala una unidad systemd ni guarda credenciales en el repositorio.

## Requisitos

- Ansible Core instalado en el equipo de desarrollo.
- Acceso SSH funcional a la R36S y su clave de host aceptada en `known_hosts`.
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

El playbook copia los ejecutables a `~/.local/opt/nexusd`, crea enlaces `nexusd` y `nexusctl` en `~/.local/bin`, agrega ese directorio al `PATH` de login en `~/.profile`, guarda PID y log en `~/.cache/nexusd`, inicia el daemon si no está activo y reintenta la consulta de salud hasta obtener respuesta. Abre una nueva sesión SSH para usar `nexusctl` como comando. Para detener el daemon:

```sh
ansible-playbook playbooks/stop.yml
```

Los logs de la consola quedan en `~/.cache/nexusd/nexusd.log`. Los binarios, enlaces y el cambio de `PATH` son de usuario y reversibles; el proceso es temporal. No se instala un servicio ni se integra en la imagen Armbian.

## Validación en consola

2026-10-24: el despliegue Ansible en la R36S detectó arquitectura ARM64, copió ambos binarios e inició `nexusd` como el usuario `r36s`. Una sesión de login resolvió `nexusctl` como `/home/r36s/.local/bin/nexusctl` y `nexusctl health` respondió `serving`. El proceso queda temporalmente activo hasta detenerlo con `playbooks/stop.yml` o reiniciar la consola.
