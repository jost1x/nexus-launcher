# Despliegue

Área para desplegar NEXUS en la compilación Armbian propia para R36S. El prototipo se despliega por SSH con Ansible en espacio de usuario; el empaquetado e integración en la imagen se realizan en una fase posterior.

La configuración está en [`ansible/`](ansible/README.md). `deploy.yml` instala `nexusd` y `nexusctl`, configura la unidad systemd del daemon y consulta su salud. `deploy-ui.yml` copia el bundle ARM64 de la interfaz a la R36S y la abre en la TTY 2 para pruebas. No modifica el build de la imagen Armbian. `stop.yml` detiene y deshabilita el daemon.

## Fases

- Durante el prototipo: permitir copiar y ejecutar binarios temporalmente por SSH, revisar logs y probar el socket UDS local. No guardar credenciales en el proyecto ni instalar servicios persistentes.
- Fase 8: integrar en Armbian con requisitos verificados, empaquetado, permisos, ciclo de vida, diagnóstico y recuperación.
- Fase 9: estabilizar instalación y diagnóstico con pruebas en hardware real.
