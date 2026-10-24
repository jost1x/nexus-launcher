# Automatización de GitHub

Contenedor reservado para configuración futura de repositorio remoto y CI. La integración con GitHub Actions se pospone hasta la fase final, según la decisión del proyecto. Las comprobaciones de hardware quedan fuera de la CI general.

## Fases

- Fase 0: definir verificaciones automatizables disponibles: formato/lint, compilación, validación y generación de contratos, y pruebas.
- Fases posteriores: ampliar la matriz solo cuando haya implementaciones y dependencias reproducibles.

No declarar que una verificación corre en CI hasta que exista un workflow ejecutable.
