# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Verify build-dist output and prepare deterministic GitHub release assets."""

import argparse
import hashlib
import io
import json
import re
import shutil
import tempfile
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILD_FILES = (
    "TUSB9261_RDX.out",
    "TUSB9261_RDX.map",
    "TUSB9261_RDX.hex",
    "TUSB9261_RDX_flash.hex",
)
REQUIRED_DOCUMENTS = (
    "docs/README.md",
    "docs/getting-started/installation.md",
    "docs/development/rom-loader-recovery.md",
    "docs/assets/rdx-board-j7-location.jpg",
    "docs/assets/rdx-board-j7-close-up.jpg",
)


def sha256(data: bytes) -> str:
    """Return the lowercase SHA-256 digest of immutable input bytes."""

    return hashlib.sha256(data).hexdigest()


def read_required(path: Path) -> bytes:
    """Read a nonempty regular input file without following a file symlink."""

    if path.is_symlink() or not path.is_file():
        raise ValueError(f"Required regular file is missing or a symlink: {path}")
    try:
        data = path.read_bytes()
    except OSError as error:
        raise ValueError(f"Cannot read required file {path}: {error}") from error
    if not data:
        raise ValueError(f"Required file is empty: {path}")
    return data


def collect_tree(root: Path, directory: str) -> dict[str, bytes]:
    """Collect a documentation or license tree using portable archive paths."""

    base = root / directory
    if base.is_symlink() or not base.is_dir():
        raise ValueError(f"Required directory is missing or a symlink: {base}")
    result = {}
    for path in sorted(base.rglob("*")):
        relative = path.relative_to(root)
        if relative.parts[:2] == ("docs", "superpowers"):
            continue
        if path.is_symlink():
            raise ValueError(f"Symlink is not allowed in release documentation: {path}")
        if path.is_file():
            result[relative.as_posix()] = read_required(path)
    if not result:
        raise ValueError(f"Required directory is empty: {base}")
    return result


def validate_version(root: Path, version: str) -> None:
    """Require the release label, VERSION, and compiled firmware to agree."""

    if not re.fullmatch(r"(?:0|[1-9]\d*)\.\d{2}", version):
        raise ValueError("Version must use MAJOR.MINOR firmware form, for example 1.06")
    recorded = read_required(root / "VERSION").decode("ascii").strip()
    if recorded != version:
        raise ValueError(f"VERSION {recorded!r} does not match requested version {version}")
    header = read_required(root / "include/rdx_mount/tusb9260.h").decode("utf-8")
    values = []
    for component in ("MAJOR", "MINOR"):
        matches = re.findall(
            rf"^#define\s+FIRMWARE_{component}_VERSION\s+(\d+)\s*$", header, re.M
        )
        if len(matches) != 1:
            raise ValueError(f"Expected one compiled FIRMWARE_{component}_VERSION")
        values.append(int(matches[0]))
    compiled = f"{values[0]}.{values[1]:02d}"
    if compiled != version:
        raise ValueError(f"Compiled firmware version {compiled} does not match {version}")


def validate_bundle(dist: Path, version: str) -> dict[str, bytes]:
    """Verify all six portable build-dist files against both digest contracts."""

    base = "OpenRDX-v" + version.replace(".", "-")
    fields = {
        "container": base + ".bin",
        "flashburner_hex": base + "-FlashBurner.hex",
        "installation_updater": "rdx_manager_firmware_update.ps1",
        "installation_procedure": "OPENRDX_USB_UPDATE.md",
    }
    manifest_name = base + ".json"
    checksums_name = base + ".sha256"
    bundle = {
        name: read_required(dist / name)
        for name in (*fields.values(), manifest_name, checksums_name)
    }
    try:
        manifest = json.loads(bundle[manifest_name].decode("utf-8-sig"))
    except (ValueError, UnicodeError) as error:
        raise ValueError(f"Invalid release manifest: {error}") from error
    if not isinstance(manifest, dict):
        raise ValueError("Release manifest must contain an object")
    if manifest.get("release_version") != version or manifest.get("release_name") != base:
        raise ValueError("Manifest release version or release name does not match the requested release")
    if manifest.get("container_length") != 62110 or len(bundle[fields["container"]]) != 62110:
        raise ValueError("RDX Manager container must have the required 62,110-byte length")
    for field, name in fields.items():
        # Exact basenames reject traversal, drive paths, and unexpected aliases
        # on every host; no manifest value is ever used as a filesystem path.
        if manifest.get(field) != name:
            raise ValueError(f"Manifest {field} must name the portable release file {name}")
        expected = manifest.get(field + "_sha256")
        if not isinstance(expected, str) or not re.fullmatch(r"[0-9a-fA-F]{64}", expected):
            raise ValueError(f"Manifest digest is invalid for {name}")
        if expected.lower() != sha256(bundle[name]):
            raise ValueError(f"Manifest SHA-256 mismatch for {name}")

    expected_names = set(fields.values()) | {manifest_name}
    found = set()
    for line in bundle[checksums_name].decode("ascii").splitlines():
        match = re.fullmatch(r"([0-9a-fA-F]{64}) \*([^/\\]+)", line)
        if match is None:
            raise ValueError(f"Malformed release checksum line: {line!r}")
        expected, name = match.groups()
        if name not in expected_names or name in found:
            raise ValueError(f"Unexpected or duplicate checksum member: {name}")
        if expected.lower() != sha256(bundle[name]):
            raise ValueError(f"Checksum listing SHA-256 mismatch for {name}")
        found.add(name)
    if found != expected_names:
        raise ValueError("Checksum listing does not cover all five release payload files")
    return bundle


def create_zip(files: dict[str, bytes]) -> bytes:
    """Create a stable archive independent of host timestamps and permissions."""

    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, "w", compression=zipfile.ZIP_STORED) as archive:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            # Stored entries also avoid zlib-version differences across hosts.
            info.compress_type = zipfile.ZIP_STORED
            archive.writestr(info, data)
    return buffer.getvalue()


def package_release(
    *, dist: Path, build_dir: Path, output_dir: Path, version: str,
    commit: str, repository: str, root: Path = ROOT,
) -> dict:
    """Validate every input, then publish the complete asset directory at once."""

    root, dist, build_dir, output_dir = map(Path, (root, dist, build_dir, output_dir))
    if not re.fullmatch(r"[0-9a-fA-F]{40}", commit):
        raise ValueError("Source commit must be an immutable 40-character hexadecimal SHA")
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9-]*/[A-Za-z0-9][A-Za-z0-9._-]*", repository):
        raise ValueError("Repository must use GitHub OWNER/REPOSITORY form")
    if output_dir.is_symlink() or (output_dir.exists() and (
        not output_dir.is_dir() or any(output_dir.iterdir())
    )):
        raise ValueError(f"Output directory must be absent or empty: {output_dir}")
    validate_version(root, version)
    bundle = validate_bundle(dist, version)
    build = {name: read_required(build_dir / name) for name in BUILD_FILES}
    base = "OpenRDX-v" + version.replace(".", "-")
    if build["TUSB9261_RDX_flash.hex"] != bundle[base + "-FlashBurner.hex"]:
        raise ValueError("Build continuous HEX does not match the bundled FlashBurner image")
    legal = {
        name: read_required(root / name)
        for name in ("LICENSE", "NOTICE", "COMMERCIAL-LICENSE.md")
    }
    legal.update(collect_tree(root, "LICENSES"))
    documentation = collect_tree(root, "docs")
    for name in REQUIRED_DOCUMENTS:
        if name not in documentation:
            raise ValueError(f"Required release documentation is missing: {name}")
    for name in ("README.md", "VERSION"):
        documentation[name] = read_required(root / name)
    if documentation["docs/getting-started/installation.md"] != bundle["OPENRDX_USB_UPDATE.md"]:
        raise ValueError("Bundled installation procedure does not match the source documentation")

    commit = commit.lower()
    source_url = f"https://github.com/{repository}"
    provenance = {
        "schema_version": 1,
        "project": "OpenRDX",
        "author": "André Fiedler",
        "release_version": version,
        "source_repository": source_url,
        "source_commit": commit,
        "source_commit_url": f"{source_url}/commit/{commit}",
        "source_archive_url": f"{source_url}/archive/{commit}.zip",
        "compiler": {
            "name": "Texas Instruments ARM Code Generation Tools",
            "version": "5.2.5",
            "host": "Windows",
            "abi": "ti_arm9_abi",
        },
        "platformio_environment": "tusb9261_ti_cgt",
        "bundle_sha256": {name: sha256(data) for name, data in sorted(bundle.items())},
        "build_artifacts_sha256": {name: sha256(data) for name, data in sorted(build.items())},
    }
    common = dict(legal)
    common["BUILD-INFO.json"] = (json.dumps(
        provenance, ensure_ascii=False, sort_keys=True, indent=2
    ) + "\n").encode("utf-8")
    archives = {
        base + ".zip": create_zip({**bundle, **documentation, **common}),
        base + "-build.zip": create_zip({**build, **common}),
    }
    assets = {**bundle, **archives}
    assets["SHA256SUMS"] = "".join(
        f"{sha256(data)} *{name}\n" for name, data in sorted(assets.items())
    ).encode("ascii")

    # Capture and validate all input bytes before creating any publication file.
    # Staging in a sibling directory prevents a failed write from exposing a
    # partial release directory to the subsequent upload step.
    output_dir.parent.mkdir(parents=True, exist_ok=True)
    stage = Path(tempfile.mkdtemp(prefix=".openrdx-release-", dir=output_dir.parent))
    try:
        for name, data in sorted(assets.items()):
            (stage / name).write_bytes(data)
        if output_dir.exists():
            output_dir.rmdir()
        stage.rename(output_dir)
    finally:
        if stage.exists():
            shutil.rmtree(stage)
    return {"release_version": version, "assets": sorted(assets)}


def main() -> int:
    """Package one Windows TI CGT build for GitHub Actions or a manual release."""

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--dist", type=Path, default=ROOT / "dist")
    parser.add_argument("--build-dir", type=Path, default=ROOT / ".pio/build/tusb9261_ti_cgt")
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--repository", default="SunboX/OpenRDX")
    arguments = parser.parse_args()
    try:
        result = package_release(**vars(arguments))
    except (ValueError, OSError) as error:
        parser.exit(1, f"Release packaging failed: {error}\n")
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
