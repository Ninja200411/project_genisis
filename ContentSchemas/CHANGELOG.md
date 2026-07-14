# Project Genesis Content Schema Changelog

## Versioning contract

Content definitions use `schemaMajor` and `schemaMinor`.

- A **minor** version may add optional fields with deterministic defaults, widen documentation, or add enum values that older consumers may safely ignore.
- A **major** version is required when a field is removed or renamed, its meaning or unit changes, a required field is added, an ID prefix changes, or a value range is narrowed.
- Runtime loaders must reject unsupported major versions before reference resolution.
- Migrations are explicit, ordered, idempotent transformations from one major version to the next. A migration must never silently infer a physical unit.
- Every migration must preserve the stable content ID or provide an explicit redirect recorded in the migration report.

## 1.0.0 — Initial contract

Introduced versioned schemas for:

- `BuildingDefinition`
- `ResourceDefinition`
- `PersonRoleDefinition`
- `VehicleDefinition`
- `RecipeDefinition`
- `TechnologyDefinition`

Introduced typed references and explicit quantities for mass (`kg`), volume (`m3`), energy (`J`), power (`W`), time (`s`) and cost (`credit`). Unknown fields are rejected in strict mode through `additionalProperties: false`.

## Migration template

Each future major migration must include:

1. source and target schema versions;
2. affected definition types and fields;
3. deterministic transformation rules;
4. default values and their rationale;
5. ID redirects, if any;
6. positive, negative and before/after fixtures;
7. rollback limitations.
