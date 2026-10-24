# Despliegue

Área para desplegar NEXUS en la compilación Armbian propia para R36S. El prototipo se despliega por SSH con Ansible en espacio de usuario; el empaquetado e integración en la imagen se realizan en una fase posterior.

La configuración está en [`ansible/`](ansible/README.md). El playbook compila-dependiente copia `nexusd` y `nexusctl` ARM64, inicia el daemon temporalmente y consulta su salud. No usa `become`, instala servicios ni escribe en el sistema de archivos de la imagen. `stop.yml` detiene el proceso del prototipo.

## Fases

- Durante el prototipo: permitir copiar y ejecutar binarios temporalmente por SSH, revisar logs y probar el socket UDS local. No guardar credenciales en el proyecto ni instalar servicios persistentes.
- Fase 8: integrar en Armbian con requisitos verificados, empaquetado, permisos, ciclo de vida, diagnóstico y recuperación.
- Fase 9: estabilizar instalación y diagnóstico con pruebas en hardware real.
