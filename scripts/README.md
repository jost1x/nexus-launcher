# Scripts de desarrollo

Área para automatizar tareas repetibles del repositorio, como bootstrap, generación, compilación, pruebas, lint, limpieza y empaquetado.

## Hooks pre-commit

La configuración del repositorio está en [`.pre-commit-config.yaml`](../.pre-commit-config.yaml). Instala `pre-commit` en el entorno de desarrollo y ejecuta `pre-commit install` desde la raíz de `nexus-launcher` para activar los hooks en cada commit. Usa `pre-commit run --all-files` para ejecutarlos manualmente sobre todos los archivos.

Los hooks generales validan YAML y conflictos y normalizan espacios y finales de archivo. Al cambiar archivos Go de `nexusd`, `gofmt` aplica formato y `golangci-lint` revisa el módulo; ambos ejecutables deben estar instalados en el entorno.

## Fases

- Fase 0: definir comandos reproducibles y documentar dependencias necesarias.
- Fases 1–7: extenderlos para desarrollo del daemon/CLI, despliegue de prototipo por SSH y luego integración de la UI.
- Fase 8: añadir empaquetado e instalación/desinstalación con recuperación para Armbian.

Los scripts deben fallar con mensajes claros. El flujo de prototipo por SSH debe desplegar binarios de forma temporal y no persistir secretos ni configuración de servicios.
