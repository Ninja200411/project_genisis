#!/usr/bin/env python3

import json
import tempfile
import unittest
from pathlib import Path

import catalog_tool


class CatalogToolTests(unittest.TestCase):
    @staticmethod
    def sample_catalogs():
        catalogs = {catalog_type: [] for catalog_type in catalog_tool.CATALOG_TYPES}
        catalogs["resources"] = [{
            "id": "resource:iron_ore",
            "name": "Eisenerz",
            "unit": "kg",
            "tags": ["ore", "raw"],
            "references": [],
            "description": "Erz mit Umlaut und | Trennzeichen.",
        }]
        catalogs["recipes"] = [{
            "id": "recipe:iron_plate",
            "name": "Eisenplatte",
            "unit": "batch",
            "tags": ["industry"],
            "references": ["resource:iron_ore"],
            "description": "",
        }]
        return catalogs

    def test_repeated_generation_is_byte_identical(self):
        catalogs = self.sample_catalogs()
        self.assertEqual(catalog_tool.render_markdown(catalogs), catalog_tool.render_markdown(catalogs))

    def test_roundtrip_preserves_canonical_fields_and_special_characters(self):
        catalogs = self.sample_catalogs()
        with tempfile.TemporaryDirectory() as directory:
            markdown = Path(directory) / "catalog.md"
            markdown.write_text(catalog_tool.render_markdown(catalogs), encoding="utf-8")
            imported, diagnostics = catalog_tool.parse_markdown(markdown)
        self.assertFalse([item for item in diagnostics if item.severity == "ERROR"])
        self.assertEqual(imported, catalogs)

    def test_json_rows_are_sorted_by_id(self):
        first = {"id": "resource:zinc", "name": "Zink", "unit": "kg"}
        second = {"id": "resource:aluminium", "name": "Aluminium", "unit": "kg"}
        payload = {"catalogs": {catalog_type: [] for catalog_type in catalog_tool.CATALOG_TYPES}}
        payload["catalogs"]["resources"] = [first, second]
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "catalogs.json"
            source.write_text(json.dumps(payload), encoding="utf-8")
            catalogs, diagnostics = catalog_tool.load_json(source)
        self.assertFalse([item for item in diagnostics if item.severity == "ERROR"])
        self.assertEqual([row["id"] for row in catalogs["resources"]], ["resource:aluminium", "resource:zinc"])

    def test_unknown_reference_has_location_object_and_hint(self):
        catalogs = self.sample_catalogs()
        catalogs["recipes"][0]["references"] = ["resource:missing"]
        diagnostic = catalog_tool.validate_references(catalogs, "catalogs.json")[0]
        self.assertEqual(diagnostic.code, "GEN-CAT-REFERENCE")
        self.assertEqual(diagnostic.column, "references")
        self.assertEqual(diagnostic.object_id, "recipe:iron_plate")
        self.assertIn("correct the ID", diagnostic.hint)

    def test_implicit_default_is_reported(self):
        entry, diagnostics = catalog_tool.canonicalize_entry(
            {"id": "resource:water", "name": "Wasser"}, "resources", "catalog.md", 12
        )
        self.assertEqual(entry["unit"], "unit")
        self.assertEqual(diagnostics[0].line, 12)
        self.assertEqual(diagnostics[0].code, "GEN-CAT-DEFAULT-UNIT")


if __name__ == "__main__":
    unittest.main()
