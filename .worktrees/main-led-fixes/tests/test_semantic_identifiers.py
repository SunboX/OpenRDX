# SPDX-FileCopyrightText: 2026 Andre Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Guard firmware sources against generated placeholder names."""

import json
import pathlib
import re
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
FIRMWARE_SYMBOL_MANIFEST_PATH = (
    PROJECT_ROOT / "tests" / "fixtures" / "firmware_symbol_manifest.json"
)
UNRESOLVED_SYMBOL_REPORT_PATH = PROJECT_ROOT / "docs" / "UNRESOLVED_FIRMWARE_SYMBOLS.md"
PLACEHOLDER_PATTERN = re.compile(
    r"\b(?:unsigned|signed|boolean|character|short|long|stack|input|preserved)_"
    r"(?:value|pointer)_\d+\b|\b(?:callee_result|temporary|stack_buffer)_\d+\b"
)
NUMBERED_LOCAL_PATTERN = re.compile(
    r"\b(?:[A-Za-z_][A-Za-z0-9_]*_[0-9]{1,2}|offset_field_100)\b"
)
ALLOWED_NUMBERED_IDENTIFIERS = set()
NUMBERED_FUNCTION_FAMILY_PATTERN = re.compile(
    r"\b(?:ahci|core|gio|interrupt|mww|rti|sci|spi|usb)_"
    r"(?:configure|coordinate|dispatch|forward|get_value|process|run|transfer|"
    r"update|update_state)_\d+(?:_[A-Za-z0-9_]+)?\b|"
    r"\bti_memset_entry_wrapper_2\b"
)
GENERATED_RESOURCE_FUNCTION_PATTERN = re.compile(
    r"\b(?:ahci|core|gio|interrupt|mww|rti|sci|spi|usb)_"
    r"(?:configure|coordinate|dispatch|forward|get|handle|increment|is|process|"
    r"read|run|set|store|transfer|transition|update)_?[A-Za-z0-9_]*"
    r"(?:(?:resource_)?(?:(?:core|usb|mww|ahci|interrupt)_)?"
    r"(?:state|storage|mmio_register|register)_\d{3}|"
    r"(?:b|i)stack_[0-9a-f]+|r0x[0-9a-f]+)[A-Za-z0-9_]*\b"
)
FUNCTION_NUMERIC_SUFFIX_PATTERN = re.compile(
    r"\b(?:ahci|core|gio|interrupt|mww|rti|sci|spi|usb)_"
    r"(?:configure|coordinate|dispatch|forward|get|handle|increment|is|process|"
    r"read|run|set|store|transfer|transition|update)_[A-Za-z0-9_]*_\d+\b"
)
ABSOLUTE_SYMBOL_PATTERN = re.compile(
    r"^_([A-Za-z_]\w*)\s*=\s*(0x[0-9A-Fa-f]+);", re.MULTILINE
)
NUMBERED_ABSOLUTE_SYMBOL_PATTERN = re.compile(r"_[0-9]{3}$")
GENERIC_FIRMWARE_COMMENT_PATTERN = re.compile(
    r"Execute the procedure using the named state and hardware resources below|"
    r"Process each entry in the bounded firmware table or buffer|"
    r"Dispatch according to the decoded firmware state or request value|"
    r"Read the current memory-mapped peripheral register value|"
    r"Update the memory-mapped peripheral register|"
    r"Continue until the hardware or state-machine completion condition is met|"
    r"Execute this firmware block using its fixed-address resources|"
    r"Repeat this hardware/state sequence until|"
    r"Invoke the callback selected by the current firmware state|"
    r"Continue the state-machine dispatch|"
    r"@brief (?:Coordinate|Process|Update|Get value|Transfer|Configure|Handle|Run|Forward).*state or data"
)


class SemanticIdentifierTests(unittest.TestCase):
    """Verify that firmware C sources retain semantic local names."""

    @staticmethod
    def firmware_implementation_paths():
        """Return compiled C files and included C implementation fragments."""
        paths = list((PROJECT_ROOT / "src").glob("*.c"))
        paths.extend((PROJECT_ROOT / "src").rglob("*.inc"))
        return sorted(paths)

    @staticmethod
    def load_firmware_symbol_manifest():
        """Load the semantic symbol catalog used by fixed-layout checks."""
        return json.loads(FIRMWARE_SYMBOL_MANIFEST_PATH.read_text(encoding="utf-8"))

    @staticmethod
    def load_absolute_symbol_bindings():
        """Return current absolute linker symbol names and normalized addresses."""
        linker_path = PROJECT_ROOT / "linker" / "firmware_absolute_symbols.cmd"
        return {
            name: int(address, 16)
            for name, address in ABSOLUTE_SYMBOL_PATTERN.findall(
                linker_path.read_text(encoding="utf-8")
            )
        }

    def test_sources_do_not_contain_generated_local_names(self):
        """Reject numbered type-based local identifiers in firmware sources."""
        matches = []
        for source_path in self.firmware_implementation_paths():
            for line_number, line in enumerate(
                source_path.read_text(encoding="utf-8").splitlines(), start=1
            ):
                for pattern in (PLACEHOLDER_PATTERN, NUMBERED_LOCAL_PATTERN):
                    for match in pattern.finditer(line):
                        if match.group(0) in ALLOWED_NUMBERED_IDENTIFIERS:
                            continue
                        matches.append(
                            f"{source_path.relative_to(PROJECT_ROOT)}:{line_number}:"
                            f"{match.group(0)}"
                        )
        self.assertEqual([], matches, "\n".join(matches))

    def test_sources_do_not_contain_numbered_function_families(self):
        """Reject numbered functions and identifiers derived from their names."""
        matches = []
        paths = self.firmware_implementation_paths()
        paths.extend((PROJECT_ROOT / "include").rglob("*.h"))
        for path in sorted(paths):
            for line_number, line in enumerate(
                path.read_text(encoding="utf-8").splitlines(), start=1
            ):
                for pattern in (
                    NUMBERED_FUNCTION_FAMILY_PATTERN,
                    GENERATED_RESOURCE_FUNCTION_PATTERN,
                    FUNCTION_NUMERIC_SUFFIX_PATTERN,
                ):
                    for match in pattern.finditer(line):
                        matches.append(
                            f"{path.relative_to(PROJECT_ROOT)}:{line_number}:"
                            f"{match.group(0)}"
                        )
        self.assertEqual([], matches, "\n".join(matches))

    def test_manifest_covers_every_numbered_absolute_symbol(self):
        """Require one catalog entry for every numbered fixed binding."""
        manifest = self.load_firmware_symbol_manifest()
        entries = manifest["symbols"]
        placeholder_names = [entry["placeholder_name"] for entry in entries]
        self.assertEqual(534, len(entries))
        self.assertEqual(len(placeholder_names), len(set(placeholder_names)))
        self.assertTrue(
            all(
                NUMBERED_ABSOLUTE_SYMBOL_PATTERN.search(name)
                for name in placeholder_names
            )
        )

    def test_catalog_records_a_semantic_name_or_unresolved_reason(self):
        """Reject undecided, unsupported, duplicate, or numbered semantic names."""
        entries = self.load_firmware_symbol_manifest()["symbols"]
        errors = []
        semantic_names = []
        for entry in entries:
            placeholder_name = entry["placeholder_name"]
            semantic_name = entry["semantic_name"]
            unresolved_reason = entry["unresolved_reason"]
            verification = entry["verification"]
            if bool(semantic_name) == bool(unresolved_reason):
                errors.append(f"{placeholder_name}: choose exactly one decision")
            if not verification:
                errors.append(f"{placeholder_name}: verification is required")
            if semantic_name:
                semantic_names.append(semantic_name)
                if re.search(r"_[0-9]+$", semantic_name):
                    errors.append(
                        f"{placeholder_name}: numbered semantic name {semantic_name}"
                    )
                if entry["confidence"] not in {"confirmed", "structural"}:
                    errors.append(
                        f"{placeholder_name}: invalid confidence "
                        f"{entry['confidence']!r}"
                    )
        if len(semantic_names) != len(set(semantic_names)):
            errors.append("semantic names must be unique")
        self.assertEqual([], errors, "\n".join(errors))

    def test_semantic_absolute_symbols_are_atomic_and_keep_their_addresses(self):
        """Require semantic bindings to be atomic and keep exact addresses."""
        entries = self.load_firmware_symbol_manifest()["symbols"]
        bindings = self.load_absolute_symbol_bindings()
        searchable_paths = list((PROJECT_ROOT / "src").glob("*"))
        searchable_paths.extend((PROJECT_ROOT / "include").rglob("*.h"))
        searchable_paths.append(PROJECT_ROOT / "linker" / "firmware_absolute_symbols.cmd")
        searchable_text = "\n".join(
            path.read_text(encoding="utf-8", errors="ignore")
            for path in searchable_paths
            if path.is_file()
        )
        errors = []
        for entry in entries:
            placeholder_name = entry["placeholder_name"]
            current_name = entry["semantic_name"] or placeholder_name
            expected_address = int(entry["address"], 16)
            if current_name not in bindings:
                errors.append(f"{current_name}: missing linker binding")
            elif bindings[current_name] != expected_address:
                errors.append(
                    f"{current_name}: expected {entry['address']}, "
                    f"found 0x{bindings[current_name]:08X}"
                )
            if entry["semantic_name"] and re.search(
                rf"\b{re.escape(placeholder_name)}\b", searchable_text
            ):
                errors.append(
                    f"{placeholder_name}: placeholder remains after semantic binding"
                )
        self.assertEqual([], errors, "\n".join(errors))

    def test_sources_do_not_use_generic_implementation_comments(self):
        """Require code comments to explain the specific operation."""
        matches = []
        for source_path in self.firmware_implementation_paths():
            lines = source_path.read_text(encoding="utf-8").splitlines()
            for line_number, line in enumerate(lines, start=1):
                if GENERIC_FIRMWARE_COMMENT_PATTERN.search(line):
                    matches.append(
                        f"{source_path.relative_to(PROJECT_ROOT)}:{line_number}:"
                        f"{line.strip()}"
                    )
                if (
                    line_number > 1
                    and line.strip().startswith("/*")
                    and line.strip() == lines[line_number - 2].strip()
                ):
                    matches.append(
                        f"{source_path.relative_to(PROJECT_ROOT)}:{line_number}:"
                        "duplicate adjacent comment"
                    )
        self.assertEqual([], matches, "\n".join(matches))

    def test_unresolved_report_matches_manifest(self):
        """Keep the human-readable unresolved list synchronized with the manifest."""
        expected = {
            entry["placeholder_name"]
            for entry in self.load_firmware_symbol_manifest()["symbols"]
            if not entry["semantic_name"]
        }
        reported = set(
            re.findall(
                r"^\| `([A-Za-z_]\w*)` \| `0x[0-9A-Fa-f]+` \|",
                UNRESOLVED_SYMBOL_REPORT_PATH.read_text(encoding="utf-8"),
                re.MULTILINE,
            )
        )
        self.assertEqual(expected, reported)


if __name__ == "__main__":
    unittest.main()
