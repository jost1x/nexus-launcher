# Código generado

Destino reservado para artefactos generados desde `packages/proto`. No editar manualmente estos archivos; la fuente de verdad está en los contratos y la configuración de generación.

## Destinos previstos

- `go/`: stubs y tipos Go.
- `cpp/`: stubs y tipos C++.

## Fases

- Fase 0: acordar rutas y hacer reproducible la generación con Buf.
- Fase 2 en adelante: generar los contratos que consumen `nexusd`, `nexusctl` y, después, `nexus-ui`.

No añadir artefactos hasta que exista una configuración de generación funcional.
