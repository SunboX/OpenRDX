# SPDX-FileCopyrightText: 2026 Andre Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Keep repository text focused on the standalone OpenRDX product."""

from __future__ import annotations

import hashlib
import os
from pathlib import Path
import re
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]
IGNORED_DIRECTORIES = {
    ".git",
    ".pio",
    ".pytest_cache",
    ".superpowers",
    "LICENSES",
    "__pycache__",
    "superpowers",
}
TEXT_SUFFIXES = {
    ".asm",
    ".c",
    ".cmd",
    ".h",
    ".inc",
    ".ini",
    ".json",
    ".md",
    ".ps1",
    ".py",
    ".sha256",
    ".txt",
    ".yaml",
    ".yml",
}
TEXT_FILE_NAMES = {".gitignore", "VERSION"}

# The digests intentionally keep disallowed vocabulary out of the repository
# while still making the check case-insensitive and deterministic. Each value
# is SHA-256 over one lowercase token or adjacent-token phrase.
DISALLOWED_TOKEN_DIGESTS = {
    "0682c5f2076f099c34cfdd15a9e063849ed437a49677e6fcc5b4198c76575be5",
    "06c8aaa93d80a768829b6005973fa92e34612849b79910c8be8e3b006cf91c61",
    "08c4242b5f585628bb8cef8ede40e9895b8719eb8070aee2d96613583ee3904a",
    "10cc74f79a1b5f011a106c79bf0a840af182b3f5a4a29f7ddce8d2344b4ef2dd",
    "1dc7be12a594ad02dcf69b900b16046c92cbc261681585efb77abe2b42ae3845",
    "2d0d38e68843216f183110ca3f4212682e1ccf86371e15fbdc0a3f24e697cfe0",
    "3bc801a33ea83df414e1aeb962a52412835be99db28668ec25de73fdd4733804",
    "59a2ea36c20b30a30f3aa251b346188d5afbeddfca2d496e67b0c2d9eb74a345",
    "5e4edc3e9d98c34d7600433c568d2af26f079768667d7239c6d0f9ea997d9b99",
    "6a9bb179400afb21957db1e9f878f0c8a7dd1bb443303855c43e0597647a3457",
    "6c695db7135689cbb77eb4bc2d0adff21bf841b0015f51e30030c89e3562dba6",
    "6e3fd0b70aa5c52721d996ac6485608b4b1d6a9066541778c2b8bb920742a54f",
    "74da08619813b9c5410783c2306662e73e98cb0fba51da747e77608ed0f411e1",
    "8a6cead4385ed4394247b71692fb729b0563f8e1bd4818a8c6c82940e9e099ba",
    "8dd57713195214f20fc904ef3de163b4cd26462bf9e65c0c311438008b227ba5",
    "94805aea13420685a2929f6859071e2e12d650c6609c0d91ed602a9bf227af39",
    "94adcb78fb6df2e132e4bbf8f9fa89a2e8657721252b5e58953ed595ceb807c1",
    "d3986ad4c179b5809a041dc934093a926a10f6c47858c1ceabf660726ccea10f",
    "d58efc4ec1ab298538e0f804b5064e9c84d411029ecb611875f48f9710558ae3",
    "de207cf377129818b2b92610afd3b4bfa10fe95c65d0d913d70d8ffd0468bc78",
    "f6e09cc89f85dcd21d987a4c4af142fe5bbb741de93d375af548f1d4f1d2063b",
    "f9b45c37c456d88776584f64ab537aeb1f9136663540cca969783153a6fcbf44",
    "fc2eed9acce3fcde24f3cd90826b1e53211f30112330678a8597bf355cbe0bdf",
}
DISALLOWED_PHRASE_DIGESTS = {
    "a34966253ddd9a4b15111a32867c4112ba37b383547801b7bb2cf1772b0aceb0",
    "cc8bac8c92ab27cddfc0cbf92f17d23b43f2cae747744de80f25fe546a24ff5e",
}


def digest(value: str) -> str:
    """Return the lowercase SHA-256 key used by the language policy."""

    return hashlib.sha256(value.lower().encode("utf-8")).hexdigest()


def language_tokens(value: str) -> list[str]:
    """Split prose, snake-case, kebab-case, and camel-case names into tokens."""

    camel_separated = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", value)
    return re.findall(r"[A-Za-z]+", camel_separated.lower())


def repository_paths() -> list[Path]:
    """Return every repository file, including binary artifact filenames."""

    paths = []
    for directory, child_directories, file_names in os.walk(PROJECT_ROOT):
        child_directories[:] = sorted(
            name for name in child_directories if name not in IGNORED_DIRECTORIES
        )
        for file_name in sorted(file_names):
            paths.append(Path(directory) / file_name)
    return sorted(paths)


def text_paths() -> list[Path]:
    """Return repository files whose contents are intentionally text."""

    return [
        path
        for path in repository_paths()
        if path.suffix.lower() in TEXT_SUFFIXES or path.name in TEXT_FILE_NAMES
    ]


def violations_in_text(value: str) -> list[str]:
    """Return opaque keys for every disallowed token or two-token phrase."""

    tokens = language_tokens(value)
    violations = [token for token in tokens if digest(token) in DISALLOWED_TOKEN_DIGESTS]
    violations.extend(
        f"{first} {second}"
        for first, second in zip(tokens, tokens[1:])
        if digest(f"{first} {second}") in DISALLOWED_PHRASE_DIGESTS
    )
    return violations


class ProductLanguageTests(unittest.TestCase):
    """Prevent disallowed vocabulary from entering product text."""

    def test_paths_use_standalone_product_language(self):
        """Check every repository path component and file name."""

        failures = []
        for path in repository_paths():
            relative_path = path.relative_to(PROJECT_ROOT)
            matches = violations_in_text(str(relative_path))
            if matches:
                failures.append(f"{relative_path}: {', '.join(matches)}")
        self.assertEqual([], failures, "\n".join(failures))

    def test_contents_use_standalone_product_language(self):
        """Check identifiers, comments, prose, scripts, tests, and metadata."""

        failures = []
        for path in text_paths():
            relative_path = path.relative_to(PROJECT_ROOT)
            for line_number, line in enumerate(
                path.read_text(encoding="utf-8", errors="replace").splitlines(),
                start=1,
            ):
                matches = violations_in_text(line)
                if matches:
                    failures.append(
                        f"{relative_path}:{line_number}: {', '.join(matches)}"
                    )
        self.assertEqual([], failures, "\n".join(failures))


if __name__ == "__main__":
    unittest.main()
