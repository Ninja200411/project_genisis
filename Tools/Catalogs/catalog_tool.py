#!/usr/bin/env python3
"""Deterministic Project Genesis catalog importer and Markdown generator."""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

CATALOG_TYPES = ("buildings", "resources", "people", "vehicles", "recipes", "technologies")
ID_PATTERN = re.compile(r"^[a-z][a-z0-9_-]*:[a-z0-9][a-z0-9_.-]*$")
TABLE_COLUMNS = ("id", "name", "unit", "tags", "references", "description")


@dataclass(frozen=True)
class Diagnostic:
    severity: str
    code: str
    file: str
    line: int
    column: str
    object_id: str
    hint: str

    def format(self) -> str:
        return (
            f"{self.severity} {self.code}: {self.file}:{self.line} "
            f"column={self.column} id={self.object_id or '-'}: {self.hint}"
        )


def normalize_id(value: str) -> str:
    return value.strip().lower().replace(" ", "_")


def normalize_list(value: Any) -> list[str]:
    if value is None or value == "":
        return []
    if isinstance(value, str):
        values = value.split(",")
    elif isinstance(value, list):
        values = value
    else:
        raise TypeError("expected string or list")
    return sorted({str(item).strip() for item in values if str(item).strip()})


def escape_cell(value: Any) -> str:
    if isinstance(value, list):
        value = ", ".join(value)
    return str(value or "").replace("\\", "\\\\").replace("|", "\\|").replace("\n", "<br>")


def unescape_cell(value: str) -> str:
    return value.replace("<br>", "\n").replace("\\|", "|").replace("\\\\", "\\").strip()


def split_row(line: str) -> list[str]:
    cells: list[str] = []
    current: list[str] = []
    escaped = False
    for char in line.strip().strip("|"):
        if escaped:
            current.append("\\" + char if char != "|" else "|")
            escaped = False
        elif char == "\\":
            escaped = True
        elif char == "|":
            cells.append(unescape_cell("".join(current)))
            current = []
        else:
            current.append(char)
    cells.append(unescape_cell("".join(current)))
    return cells


def canonicalize_entry(raw: dict[str, Any], catalog_type: str, source: str, line: int) -> tuple[dict[str, Any], list[Diagnostic]]:
    diagnostics: list[Diagnostic] = []
    object_id = normalize_id(str(raw.get("id", "")))
    if not ID_PATTERN.match(object_id):
        diagnostics.append(Diagnostic("ERROR", "GEN-CAT-ID", source, line, "id", object_id, "Use '<type>:<stable_id>' with lowercase ASCII characters."))
    if object_id and not object_id.startswith(catalog_type.rstrip("s") + ":"):
        diagnostics.append(Diagnostic("WARNING", "GEN-CAT-ID-PREFIX", source, line, "id", object_id, f"Expected prefix '{catalog_type.rstrip('s')}:'."))

    name = str(raw.get("name", "")).strip()
    if not name:
        diagnostics.append(Diagnostic("ERROR", "GEN-CAT-NAME", source, line, "name", object_id, "Provide a non-empty display name."))

    unit = str(raw.get("unit", "")).strip().lower()
    if not unit:
        unit = "unit"
        diagnostics.append(Diagnostic("WARNING", "GEN-CAT-DEFAULT-UNIT", source, line, "unit", object_id, "Implicit default 'unit' was applied."))

    try:
        tags = normalize_list(raw.get("tags"))
        references = [normalize_id(item) for item in normalize_list(raw.get("references"))]
    except TypeError as exc:
        diagnostics.append(Diagnostic("ERROR", "GEN-CAT-LIST", source, line, "tags/references", object_id, str(exc)))
        tags, references = [], []

    return {
        "id": object_id,
        "name": name,
        "unit": unit,
        "tags": tags,
        "references": references,
        "description": str(raw.get("description", "")).strip(),
    }, diagnostics


def load_json(path: Path) -> tuple[dict[str, list[dict[str, Any]]], list[Diagnostic]]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    catalogs = payload.get("catalogs", payload)
    result: dict[str, list[dict[str, Any]]] = {}
    diagnostics: list[Diagnostic] = []
    for catalog_type in CATALOG_TYPES:
        rows = catalogs.get(catalog_type, [])
        if not isinstance(rows, list):
            diagnostics.append(Diagnostic("ERROR", "GEN-CAT-TYPE", str(path), 1, catalog_type, "", "Catalog must be a JSON array."))
            rows = []
        normalized: list[dict[str, Any]] = []
        for index, raw in enumerate(rows, start=1):
            if not isinstance(raw, dict):
                diagnostics.append(Diagnostic("ERROR", "GEN-CAT-ROW", str(path), index, catalog_type, "", "Catalog row must be an object."))
                continue
            entry, row_diagnostics = canonicalize_entry(raw, catalog_type, str(path), index)
            normalized.append(entry)
            diagnostics.extend(row_diagnostics)
        result[catalog_type] = sorted(normalized, key=lambda item: item["id"])
    diagnostics.extend(validate_references(result, str(path)))
    return result, diagnostics


def validate_references(catalogs: dict[str, list[dict[str, Any]]], source: str) -> list[Diagnostic]:
    known = {row["id"] for rows in catalogs.values() for row in rows}
    diagnostics: list[Diagnostic] = []
    for rows in catalogs.values():
        for line, row in enumerate(rows, start=1):
            for reference in row["references"]:
                if reference not in known:
                    diagnostics.append(Diagnostic("ERROR", "GEN-CAT-REFERENCE", source, line, "references", row["id"], f"Unknown reference '{reference}'. Add the object or correct the ID."))
    return diagnostics


def render_markdown(catalogs: dict[str, list[dict[str, Any]]]) -> str:
    output = ["# Project Genesis Content Catalogs", "", "<!-- Generated by Tools/Catalogs/catalog_tool.py. Do not edit manually. -->", ""]
    for catalog_type in CATALOG_TYPES:
        output.extend([f"## {catalog_type.title()}", "", "| ID | Name | Unit | Tags | References | Description |", "|---|---|---|---|---|---|"])
        for row in catalogs[catalog_type]:
            cells = [escape_cell(row[column]) for column in TABLE_COLUMNS]
            output.append("| " + " | ".join(cells) + " |")
        output.append("")
    return "\n".join(output).rstrip() + "\n"


def parse_markdown(path: Path) -> tuple[dict[str, list[dict[str, Any]]], list[Diagnostic]]:
    catalogs = {catalog_type: [] for catalog_type in CATALOG_TYPES}
    diagnostics: list[Diagnostic] = []
    current: str | None = None
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        if line.startswith("## "):
            candidate = line[3:].strip().lower()
            current = candidate if candidate in catalogs else None
            continue
        if current is None or not line.startswith("|") or line.startswith("|---") or line.startswith("| ID "):
            continue
        cells = split_row(line)
        if len(cells) != len(TABLE_COLUMNS):
            diagnostics.append(Diagnostic("ERROR", "GEN-CAT-COLUMNS", str(path), line_number, "table", "", f"Expected {len(TABLE_COLUMNS)} columns, got {len(cells)}."))
            continue
        raw = dict(zip(TABLE_COLUMNS, cells))
        entry, row_diagnostics = canonicalize_entry(raw, current, str(path), line_number)
        catalogs[current].append(entry)
        diagnostics.extend(row_diagnostics)
    for catalog_type in catalogs:
        catalogs[catalog_type].sort(key=lambda item: item["id"])
    diagnostics.extend(validate_references(catalogs, str(path)))
    return catalogs, diagnostics


def canonical_json(catalogs: dict[str, list[dict[str, Any]]]) -> str:
    return json.dumps({"schemaVersion": 1, "catalogs": catalogs}, ensure_ascii=False, indent=2, sort_keys=True) + "\n"


def fail_on_errors(diagnostics: list[Diagnostic]) -> None:
    for diagnostic in diagnostics:
        print(diagnostic.format(), file=sys.stderr)
    if any(item.severity == "ERROR" for item in diagnostics):
        raise SystemExit(1)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=("generate", "import", "check"))
    parser.add_argument("--data", type=Path, default=Path("ContentData/catalogs.json"))
    parser.add_argument("--markdown", type=Path, default=Path("Docs/Generated/content-catalogs.md"))
    args = parser.parse_args()

    if args.command == "import":
        catalogs, diagnostics = parse_markdown(args.markdown)
        fail_on_errors(diagnostics)
        args.data.parent.mkdir(parents=True, exist_ok=True)
        args.data.write_text(canonical_json(catalogs), encoding="utf-8", newline="\n")
        return

    catalogs, diagnostics = load_json(args.data)
    fail_on_errors(diagnostics)
    rendered = render_markdown(catalogs)
    if args.command == "check":
        current = args.markdown.read_text(encoding="utf-8") if args.markdown.exists() else ""
        if current != rendered:
            print(f"Generated catalog drift detected: {args.markdown}", file=sys.stderr)
            raise SystemExit(2)
        return
    args.markdown.parent.mkdir(parents=True, exist_ok=True)
    args.markdown.write_text(rendered, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
