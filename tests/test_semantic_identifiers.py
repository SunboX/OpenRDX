# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Guard firmware sources against generated placeholder names."""

import hashlib
import json
import pathlib
import re
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
FIRMWARE_SYMBOL_MANIFEST_PATH = (
    PROJECT_ROOT / "tests" / "fixtures" / "firmware_symbol_manifest.json"
)
FIRMWARE_SYMBOL_HEADER_PATH = PROJECT_ROOT / "include" / "firmware_symbols.h"
IMMUTABLE_MANIFEST_PROJECTION_SHA256 = (
    "eb7705a0a60416bf402c664ff678a2c2d8adbe104aab5bb68f0f99785e4c5052"
)
PLACEHOLDER_PATTERN = re.compile(
    r"\b(?:unsigned|signed|boolean|character|short|long|stack|input|preserved)_"
    r"(?:value|pointer)_\d+\b|\b(?:callee_result|temporary|stack_buffer)_\d+\b"
)
NUMBERED_LOCAL_PATTERN = re.compile(
    r"\b(?:[A-Za-z_][A-Za-z0-9_]*_[0-9]{1,2}|offset_field_100)\b"
)
# Widths, hardware ring indices, and checksum seed indices are meaningful.
ALLOWED_NUMBERED_IDENTIFIERS = {
    "BSWAP_16", "BSWAP_32", "EVNT_BUFF_0", "event_buffer_0",
    "RDX_STATE_CHECKSUM_SEED_0", "RDX_STATE_CHECKSUM_SEED_1",
    "RDX_STATE_CHECKSUM_SEED_2", "RDX_STATE_CHECKSUM_SEED_3",
    "TRBCTRL_CONTROL_STATUS_2", "TRBCTRL_CONTROL_STATUS_3",
}
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
HEADER_DECLARATION_PATTERN = re.compile(
    r"^extern\s+(.+?)([A-Za-z_]\w*)\s*;$", re.MULTILINE
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
        """Return C files from both firmware trees and included fragments."""
        paths = list((PROJECT_ROOT / "src").rglob("*.c"))
        paths.extend((PROJECT_ROOT / "src").rglob("*.inc"))
        return sorted(paths)

    def test_implementation_scan_includes_the_production_tree(self):
        """Keep the production mechanism and transport inside naming checks."""
        paths = set(self.firmware_implementation_paths())
        for name in ("rdx_mechanism.c", "usb_stack.c", "ahci.c", "scsi.c"):
            self.assertIn(PROJECT_ROOT / "src" / "rdx_mount" / name, paths)

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

    @staticmethod
    def load_header_declarations():
        """Return fixed-symbol header names and normalized declaration types."""
        return {
            name: re.sub(r"\s+", " ", declaration_type.strip())
            for declaration_type, name in HEADER_DECLARATION_PATTERN.findall(
                FIRMWARE_SYMBOL_HEADER_PATH.read_text(encoding="utf-8")
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

    def test_manifest_immutable_projection_matches_reviewed_layout(self):
        """Freeze binding identities, addresses and types across naming audits."""
        entries = self.load_firmware_symbol_manifest()["symbols"]
        projection = [
            [
                entry["placeholder_name"],
                entry["address"],
                entry["declaration_type"],
            ]
            for entry in entries
        ]
        payload = json.dumps(
            projection,
            ensure_ascii=True,
            separators=(",", ":"),
        ).encode("utf-8")
        self.assertEqual(
            IMMUTABLE_MANIFEST_PROJECTION_SHA256,
            hashlib.sha256(payload).hexdigest(),
        )

    def test_catalog_usage_references_match_current_source_locations(self):
        """Reject stale evidence locations after source splits and renames."""
        entries = self.load_firmware_symbol_manifest()["symbols"]
        actual_uses = {entry["semantic_name"]: [] for entry in entries}
        for path in self.firmware_implementation_paths():
            relative_name = path.relative_to(PROJECT_ROOT).as_posix()
            for line_number, line in enumerate(path.read_text().splitlines(), 1):
                for name in set(re.findall(r"\b[A-Za-z_]\w*\b", line)):
                    if name in actual_uses:
                        actual_uses[name].append(f"{relative_name}:{line_number}")
        for entry in entries:
            name = entry["semantic_name"]
            self.assertEqual(sorted(actual_uses[name]), entry["uses"], name)

    def test_data_bindings_are_not_classified_as_control_flow(self):
        """Keep image signatures, key data and descriptor tables non-callable."""
        entries = self.load_firmware_symbol_manifest()["symbols"]
        data_addresses = {
            entry["address"]
            for entry in entries
            if entry["placeholder_name"] in {
                "core_branch_target_086", "core_branch_target_087",
                "core_branch_target_117", "core_branch_target_118",
                "core_branch_target_119", "core_branch_target_120",
            }
        }
        self.assertEqual(6, len(data_addresses))
        for entry in entries:
            if entry["address"] in data_addresses:
                self.assertIn(
                    entry["category"], {"storage", "label_data"},
                    entry["semantic_name"],
                )

    def test_mmio_classification_matches_controller_address_ranges(self):
        """Separate datapath RAM fields from peripheral and core registers."""
        register_ranges = (
            (0xE000E000, 0xE000F000),
            (0xFB000000, 0xFB001000),
            (0xFC000000, 0xFC010000),
            (0xFD000000, 0xFD001000),
            (0xFFFFFE00, 0xFFFFFE48),
        )
        for entry in self.load_firmware_symbol_manifest()["symbols"]:
            address = int(entry["address"], 16)
            is_register = any(
                start <= address < end for start, end in register_ranges
            )
            self.assertEqual(
                is_register, entry["category"] == "mmio_register",
                entry["semantic_name"],
            )

    def test_catalog_assigns_every_fixed_binding_a_semantic_name(self):
        """Require supported, unique semantic names for every fixed binding."""
        entries = self.load_firmware_symbol_manifest()["symbols"]
        errors = []
        semantic_names = []
        for entry in entries:
            placeholder_name = entry["placeholder_name"]
            semantic_name = entry["semantic_name"]
            unresolved_reason = entry["unresolved_reason"]
            verification = entry["verification"]
            if not semantic_name:
                errors.append(f"{placeholder_name}: semantic name is required")
            if unresolved_reason:
                errors.append(f"{placeholder_name}: unresolved reason must be empty")
            if not verification:
                errors.append(f"{placeholder_name}: verification is required")
            elif not isinstance(verification, list) or any(
                not isinstance(note, str) or len(note.split()) < 2
                for note in verification
            ):
                errors.append(
                    f"{placeholder_name}: verification must contain complete notes"
                )
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
        searchable_paths = list((PROJECT_ROOT / "src").rglob("*"))
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
            if placeholder_name in searchable_text:
                errors.append(
                    f"{placeholder_name}: placeholder remains after semantic binding"
                )
        self.assertEqual([], errors, "\n".join(errors))

    def test_header_declarations_match_catalog_types(self):
        """Keep every catalog binding declared with its reviewed type."""
        entries = self.load_firmware_symbol_manifest()["symbols"]
        declarations = self.load_header_declarations()
        errors = []
        for entry in entries:
            name = entry["semantic_name"]
            declaration_type = declarations.get(name)
            if declaration_type is None:
                errors.append(f"{name}: missing header declaration")
            elif declaration_type != entry["declaration_type"]:
                errors.append(
                    f"{name}: expected {entry['declaration_type']}, "
                    f"found {declaration_type}"
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


if __name__ == "__main__":
    unittest.main()
