# Aplicaciones

Contiene los procesos ejecutables del producto. Cada aplicación tiene un ciclo de vida y límites de responsabilidad propios; la comunicación entre procesos usa exclusivamente los contratos compartidos.

## Fases relacionadas

- Fases 2–5: completar `nexusd`, `nexusctl` y los servicios del sistema antes de empezar la UI.
- Fase 6: núcleo de `nexus-ui` para escritorio.
- Fase 7: navegación, preferencias locales e integración RPC de la UI.
- Fase 8: ejecución e integración de ambos procesos en la compilación Armbian propia para R36S.
