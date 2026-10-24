# Paquetes compartidos

Incluye definiciones compartidas por más de una aplicación. Debe ser la fuente única de los contratos entre procesos; no guardar copias de archivos Protobuf en las aplicaciones.

## Fases

- Fase 0: definir estructura, versionado y validación de contratos con Buf.
- Fase 2: introducir salud y transporte RPC para desbloquear el trabajo de `nexusd` y su CLI antes de la UI.
- Fases 3–5: ampliar contratos de sistema, Wi-Fi y hardware según capacidades verificadas, antes de empezar `nexus-ui`.

Los cambios de API deben ser compatibles cuando sea posible y pasar validación antes de generar código.
