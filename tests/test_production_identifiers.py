# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Check names in production source documentation against their declarations."""

from pathlib import Path
import re
import unittest


PROJECT_DIR = Path(__file__).resolve().parents[1]
PRODUCTION_ROOTS = (
    PROJECT_DIR / "src" / "rdx_mount",
    PROJECT_DIR / "include" / "rdx_mount",
)
COMMENT_PATTERN = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
FUNCTION_PATTERN = re.compile(
    r"^[ \t]*(?:[A-Za-z_]\w*[\t *\r\n]+)+"
    r"([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{",
    re.MULTILINE,
)


def production_sources():
    """Yield maintained C, header, and assembly files beneath both roots."""
    for root in PRODUCTION_ROOTS:
        for path in sorted(root.rglob("*")):
            if path.suffix in {".c", ".h", ".asm", ".inc"}:
                yield path


def mask_comment(match):
    """Preserve positions and line boundaries while hiding one C comment."""
    return "".join("\n" if char == "\n" else " " for char in match.group())


class ProductionIdentifierTests(unittest.TestCase):
    """Prevent copied names from contradicting production source declarations."""

    def test_filename_banners_identify_their_actual_file(self):
        """File banners identify the containing source or header, including its suffix."""
        for path in production_sources():
            source = path.read_text(encoding="utf-8")
            for banner in re.finditer(r"Filename\s*:\s*(\S+)", source):
                with self.subTest(path=path.relative_to(PROJECT_DIR)):
                    self.assertEqual(path.name, banner.group(1))

    def test_function_banners_identify_their_actual_definition(self):
        """Doxygen function names agree with the following C definition."""
        for path in production_sources():
            if path.suffix not in {".c", ".h", ".inc"}:
                continue
            source = path.read_text(encoding="utf-8")
            masked_source = COMMENT_PATTERN.sub(mask_comment, source)
            definitions = tuple(FUNCTION_PATTERN.finditer(masked_source))
            for banner in re.finditer(r"Function:\s*(\w+)\b", source):
                definition = next(
                    (item for item in definitions if item.start() >= banner.end()),
                    None,
                )
                with self.subTest(
                    path=path.relative_to(PROJECT_DIR), name=banner.group(1)
                ):
                    self.assertIsNotNone(definition)
                    self.assertEqual(banner.group(1), definition.group(1))


if __name__ == "__main__":
    unittest.main()
