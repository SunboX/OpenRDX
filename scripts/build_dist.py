#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Refresh the complete versioned dist bundle after a TI firmware build."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from build_rdx_update_container import COMPATIBILITY_TEMPLATE_SHA256, sha256
from package_github_release import read_required, validate_bundle, validate_version


ROOT = Path(__file__).resolve().parents[1]
TEMPLATE_NAME = "RDX2E__STD__F-0283.bin"


def resolve_template(root: Path, template: Path | None = None) -> Path:
    """Resolve and verify an explicit, cached, or sibling Manager template."""
    configured = template or os.environ.get("OPENRDX_TEMPLATE_PATH")
    cache = root / ".pio/rdx-template" / TEMPLATE_NAME
    if configured:
        selected = Path(configured).expanduser().resolve()
        if not selected.is_file():
            raise ValueError(f"Compatibility template not found: {selected}")
        data = selected.read_bytes()
        if sha256(data) != COMPATIBILITY_TEMPLATE_SHA256:
            raise ValueError(f"Compatibility template SHA-256 mismatch: {selected}")
    else:
        candidates = [cache] + sorted(
            (root.parent / "OpenRDXManager").rglob(TEMPLATE_NAME)
        )
        selected = next((
            path for path in candidates
            if path.is_file() and sha256(path.read_bytes()) == COMPATIBILITY_TEMPLATE_SHA256
        ), None)
        if selected is None:
            raise ValueError(
                "Pinned compatibility template is unavailable. Set "
                f"OPENRDX_TEMPLATE_PATH to {TEMPLATE_NAME}, or provide --template."
            )
        data = selected.read_bytes()
    # Retain only verified bytes so subsequent plain `pio run` commands need no
    # shell-specific configuration. PlatformIO already ignores this local cache.
    cache.parent.mkdir(parents=True, exist_ok=True)
    if selected.resolve() != cache.resolve():
        cache.write_bytes(data)
    return cache


def build_distribution(root: Path, build_dir: Path, template: Path | None = None,
                       version: str | None = None) -> dict:
    """Validate and stage all six files before replacing the generated dist tree."""
    root, build_dir = root.resolve(), build_dir.resolve()
    version = version or read_required(root / "VERSION").decode("ascii").strip()
    validate_version(root, version)
    template = resolve_template(root, template)
    dist = root / "dist"
    if dist.is_symlink() or (dist.exists() and not dist.is_dir()):
        raise ValueError(f"Distribution path must be a regular directory: {dist}")
    base = "OpenRDX-v" + version.replace(".", "-")
    firmware_hex = build_dir / "TUSB9261_RDX.hex"
    read_required(firmware_hex)
    copies = {
        "flashburner_hex": (base + "-FlashBurner.hex", build_dir / "TUSB9261_RDX_flash.hex"),
        "installation_updater": ("rdx_manager_firmware_update.ps1", root / "scripts/rdx_manager_firmware_update.ps1"),
        "installation_procedure": ("OPENRDX_USB_UPDATE.md", root / "docs/getting-started/installation.md"),
    }
    with tempfile.TemporaryDirectory(prefix=".openrdx-dist-", dir=root) as temporary:
        transaction = Path(temporary)
        stage = transaction / "new"
        stage.mkdir()
        manifest_path = stage / (base + ".json")
        result = subprocess.run(
            [sys.executable, str(ROOT / "scripts/build_rdx_update_container.py"),
             str(firmware_hex), str(template), str(stage / (base + ".bin")),
             "--manifest", str(manifest_path)],
            capture_output=True, text=True,
        )
        if result.returncode:
            raise ValueError(f"Update container generation failed:\n{result.stderr}")
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        manifest.pop("firmware_hex", None)
        manifest.update(
            container=base + ".bin", release_version=version,
            release_name=base, required_receiver_kind="CompatibilityReceiver",
        )
        for field, (name, source) in copies.items():
            data = read_required(source)
            (stage / name).write_bytes(data)
            manifest[field] = name
            manifest[field + "_sha256"] = sha256(data)
        manifest_path.write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8",
        )
        checksums = "".join(
            f"{sha256(path.read_bytes())} *{path.name}\n"
            for path in sorted(stage.iterdir())
        )
        (stage / (base + ".sha256")).write_text(checksums, encoding="ascii")
        validate_bundle(stage, version)
        # Keep the old bundle intact until the replacement has passed all checks.
        # A failed rename restores it; retired versions disappear only on success.
        previous = transaction / "previous"
        if dist.exists():
            dist.rename(previous)
        try:
            stage.rename(dist)
        except OSError:
            if previous.exists():
                previous.rename(dist)
            raise
    return {"release_version": version, "directory": str(dist)}


def main() -> int:
    """Package existing TI outputs for the normal build or the release gate."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--template", type=Path)
    parser.add_argument("--version")
    args = parser.parse_args()
    try:
        result = build_distribution(
            args.root, args.build_dir or args.root / ".pio/build/tusb9261_ti_cgt",
            args.template, args.version,
        )
    except (OSError, ValueError) as error:
        parser.exit(1, f"Distribution build failed: {error}\n")
    print(f"Distribution {result['release_version']} completed: {result['directory']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
