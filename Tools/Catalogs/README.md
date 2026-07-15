# Content catalog toolchain

`ContentData/catalogs.json` is the canonical source for buildings, resources, people, vehicles, recipes and technologies. `Docs/Generated/content-catalogs.md` is generated and must not be edited manually.

## Commands

```powershell
python Tools/Catalogs/catalog_tool.py generate
python Tools/Catalogs/catalog_tool.py check
python Tools/Catalogs/catalog_tool.py import
python -m unittest discover -s Tools/Catalogs -p '*_tests.py' -v
```

- `generate` writes deterministic Markdown from canonical JSON.
- `check` fails with exit code `2` when committed Markdown differs from generated output.
- `import` parses the generated Markdown format back into canonical JSON.
- `Scripts/ci/check-catalogs.ps1` and `Scripts/ci/check-catalogs.sh` run tests, drift validation and a clean-tree check.

## Canonical fields

Every row contains `id`, `name`, `unit`, `tags`, `references` and `description`. IDs use lowercase `<type>:<stable_id>` syntax. Lists are normalized, deduplicated and sorted. References must resolve to another catalog entry.

Diagnostics include severity, code, file, line, column, object ID and a correction hint. Missing units are reported as warnings because the importer applies the explicit fallback `unit`; malformed IDs, rows and references are errors.

## CI integration

Run one of the scripts under `Scripts/ci` in the repository validation job. Any manual documentation drift produces a non-zero exit code and a Git diff.
