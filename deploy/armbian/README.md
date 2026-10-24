# Integración con Armbian

Destino de la integración futura de NEXUS en la compilación Armbian propia para R36S. El prototipo se despliega y prueba primero por SSH, sin instalar servicios persistentes ni modificar la imagen durante el ciclo de desarrollo.

## Fases

- Prototipo: copiar y ejecutar temporalmente `nexusd` y `nexusctl` en la consola; validar RPC por UDS y revisar logs mediante SSH.
- Fase 8: comprobar arquitectura y requisitos de la imagen, empaquetar, definir permisos y ciclo de vida, e integrar el inicio y diagnóstico.
- Fase 9: estabilizar en hardware real y documentar instalación, desinstalación y recuperación.

Consultar `r36s-os` para evidencia del sistema y hardware. Registrar explícitamente lo que aún no se haya validado en la consola.
