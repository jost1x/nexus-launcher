# Despliegue

Área para desplegar NEXUS en la compilación Armbian propia para R36S. El prototipo se despliega por SSH con Ansible en espacio de usuario; el empaquetado e integración en la imagen se realizan en una fase posterior.

La configuración está en [`ansible/`](ansible/README.md). El playbook copia `nexusd` y `nexusctl` ARM64 por SSH, instala reglas de permisos acotados y una unidad systemd para el prototipo, y consulta su salud. No modifica el build de la imagen Armbian. `stop.yml` detiene y deshabilita el servicio.

## Fases

- Durante el prototipo: permitir copiar y ejecutar binarios temporalmente por SSH, revisar logs y probar el socket UDS local. No guardar credenciales en el proyecto ni instalar servicios persistentes.
- Fase 8: integrar en Armbian con requisitos verificados, empaquetado, permisos, ciclo de vida, diagnóstico y recuperación.
- Fase 9: estabilizar instalación y diagnóstico con pruebas en hardware real.
