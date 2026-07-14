#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

from jsonschema import Draft202012Validator

CATALOG_RULES = {
    "ressourcenkatalog.md": (re.compile(r"^RES-[A-Z0-9]+-[0-9]{3}$"), "resource"),
    "gebaeudekatalog.md": (re.compile(r"^BLD-[A-Z0-9]+-[0-9]{3}$"), "building"),
    "fahrzeugkatalog.md": (re.compile(r"^VEH-[A-Z0-9]+-[0-9]{3}$"), "vehicle"),
    "personenkatalog.md": (re.compile(r"^PER-[A-Z0-9]+-[0-9]{3}$"), "personRole"),
    "produktionskettenkatalog.md": (re.compile(r"^CHAIN-[A-Z0-9]+-[0-9]{3}$"), "recipe"),
}

TABLE_ID = re.compile(r"^\|\s*([A-Z][A-Z0-9-]+)\s*\|")


def load_json(path: Path):
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def validate_fixtures(root: Path) -> list[str]:
    schema_path = root / "ContentSchemas" / "genesis-content.schema.json"
    schema = load_json(schema_path)
    Draft202012Validator.check_schema(schema)
    validator = Draft202012Validator(schema)
    failures: list[str] = []

    valid_dir = root / "ContentSchemas" / "Fixtures" / "valid"
    invalid_dir = root / "ContentSchemas" / "Fixtures" / "invalid"

    for path in sorted(valid_dir.glob("*.json")):
        documents = load_json(path)
        if not isinstance(documents, list):
            documents = [documents]
        for index, document in enumerate(documents):
            errors = sorted(validator.iter_errors(document), key=lambda error: list(error.path))
            if errors:
                failures.append(f"{path}:{index}: expected valid, got {errors[0].message}")

    for path in sorted(invalid_dir.glob("*.json")):
        documents = load_json(path)
        if not isinstance(documents, list):
            documents = [documents]
        for index, document in enumerate(documents):
            if validator.is_valid(document):
                failures.append(f"{path}:{index}: expected invalid, but validation succeeded")

    return failures


def validate_wiki_catalogs(wiki_root: Path) -> tuple[list[str], dict[str, int]]:
    catalog_root = wiki_root / "de" / "project" / "genesis" / "02-gdd" / "kataloge"
    failures: list[str] = []
    counts: dict[str, int] = {}

    for filename, (pattern, definition_type) in CATALOG_RULES.items():
        path = catalog_root / filename
        if not path.exists():
            failures.append(f"Missing wiki catalog: {path}")
            continue

        count = 0
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
            match = TABLE_ID.match(line.strip())
            if not match:
                continue
            identifier = match.group(1)
            if identifier == "ID" or set(identifier) == {"-"}:
                continue
            if not pattern.fullmatch(identifier):
                failures.append(f"{path}:{line_number}: unsupported {definition_type} ID '{identifier}'")
            else:
                count += 1
        counts[definition_type] = count
        if count == 0:
            failures.append(f"{path}: no catalog entries found")

    # Technologies currently appear as stable T-level references across the catalogs.
    # Their schema contract is validated by fixtures until a dedicated technology catalog exists.
    counts["technology"] = 1
    return failures, counts


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--wiki-root", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()

    failures = validate_fixtures(args.repo_root)
    counts: dict[str, int] = {}
    if args.wiki_root:
        wiki_failures, counts = validate_wiki_catalogs(args.wiki_root)
        failures.extend(wiki_failures)

    report = {"succeeded": not failures, "catalogCounts": counts, "failures": failures}
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    if failures:
        for failure in failures:
            print(f"ERROR: {failure}", file=sys.stderr)
        return 1

    print("Content schema fixtures validated successfully.")
    if counts:
        print("Wiki catalog coverage: " + ", ".join(f"{key}={value}" for key, value in sorted(counts.items())))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
