# Wiki catalog coverage

The authoritative Wiki catalogs are validated in CI against the schema families below.

| Wiki catalog | Stable ID family | Content definition | Canonical mapping |
|---|---|---|---|
| `ressourcenkatalog.md` | `RES-*` | `ResourceDefinition` | ID, name, usage, physical storage unit, mass/volume metadata, tags and description |
| `gebaeudekatalog.md` | `BLD-*` | `BuildingDefinition` | ID, name, construction cost/time, power demand, prerequisites and tags |
| `fahrzeugkatalog.md` | `VEH-*` | `VehicleDefinition` | ID, name, cargo mass/volume, power demand, prerequisites and tags |
| `personenkatalog.md` | `PER-*` | `PersonRoleDefinition` | ID, role name, wage/upkeep, technology prerequisites, tools and tags |
| `produktionskettenkatalog.md` | `CHAIN-*` | `RecipeDefinition` | inputs, outputs, duration, supported buildings, prerequisites and metadata |
| Technology references | `T*` design tiers | `TechnologyDefinition` | stable technology ID, research cost/time and prerequisite graph |

The Markdown catalogs contain balancing and design columns that are not all mandatory in schema major version 1. Such values map to explicit metadata, tags, description, or future optional minor-version fields; they do not require untyped extension properties. Unknown JSON fields remain rejected in strict mode.

CI checks every table ID in the five canonical catalogs and emits `Saved/ContentValidation/report.json` with the number of represented entries per definition family.
