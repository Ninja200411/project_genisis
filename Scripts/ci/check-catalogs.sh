#!/usr/bin/env bash
set -euo pipefail

REPOSITORY_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$REPOSITORY_ROOT"

python3 -m unittest discover -s Tools/Catalogs -p '*_tests.py' -v
python3 Tools/Catalogs/catalog_tool.py check
python3 Tools/Catalogs/catalog_tool.py generate
git diff --exit-code -- ContentData Docs/Generated
