# Create an OpenRDX release bundle

This maintainer guide describes the Windows release gate driven by
`build-dist.ps1`. Building the firmware alone does not produce an installable
release.

## Before packaging

- Use the supported Windows TI ARM CGT 5.2.5 build environment from
  [Build and test OpenRDX](building.md).
- Provide the pinned compatibility-image template, either below the sibling
  `OpenRDXManager` tree where the script can find it or through `-TemplatePath`.
- Ensure `VERSION` matches the compiled major and minor version in
  `include/rdx_mount/tusb9260.h`.
- Run from the intended, reviewed source tree. `-SkipTests` is appropriate only
  when the complete suite already passed for those exact sources.

## Generate the bundle

From Windows PowerShell in the repository root:

```powershell
.\build-dist.ps1
```

The script performs one release transaction:

1. validates the requested release version and required tool/input paths;
2. builds OpenRDX with the configured TI toolchain;
3. runs the complete automated suite unless `-SkipTests` was explicitly used;
4. recreates only the repository's `dist/` directory;
5. generates and validates the RDX Manager container and manifest;
6. copies the continuous FlashBurner image, guarded updater, and canonical
   installation guide; and
7. writes the release checksum file and validates the container length and
   manifest digest.

The canonical source at
[`docs/getting-started/installation.md`](../getting-started/installation.md) is
packaged under the stable release name `OPENRDX_USB_UPDATE.md`.

## Six-file release set

For release version `1.06`, the current naming pattern produces:

| File | Role |
| --- | --- |
| `OpenRDX-v1-06.bin` | RDX Manager update container |
| `OpenRDX-v1-06-FlashBurner.hex` | Continuous image for the advanced ROM-loader programming route |
| `OpenRDX-v1-06.json` | Relocatable manifest binding the version, artifact names, sizes, and digests |
| `OpenRDX-v1-06.sha256` | SHA-256 integrity list for the other five files |
| `rdx_manager_firmware_update.ps1` | Guarded Windows target-selection and update script |
| `OPENRDX_USB_UPDATE.md` | Packaged operator procedure |

Keep all six files together in an otherwise empty release directory. The
updater rejects ambiguous directories containing more than one versioned
manifest.

## Integrity, identity, and version checks

SHA-256 detects accidental or malicious byte changes relative to the expected
digest. It does not authenticate the publisher, prove that a file was signed,
or establish who created the bundle. OpenRDX firmware is not advertised as
signed firmware.

The `.sha256` file lists all five other bundle files. The operator's checksum
loop in the packaged guide validates every listed file. The updater separately
validates target identity and state, reads the manifest, hashes the selected
firmware container, and, for compatibility installation, hashes the continuous
FlashBurner HEX against its manifest value. It does not independently hash
itself or the procedure, and it does not authenticate the manifest or checksum
file. The manual whole-bundle checksum step remains mandatory.

The manifest and checksum list must agree with the exact packaged files. The
release version must also match `VERSION` and the compiled firmware version;
changing only a filename cannot create a valid new version.

## The checked-in `dist/` snapshot

The current checked-in `dist/` tree is historical. It is not a verified release
bundle for current `main`, and its present checksum set does not validate.
Customers must use a newly generated bundle that completes the full release
gate and passes its checksum verification.

Do not repair or regenerate `dist/` as part of a documentation change. A release
must be generated deliberately from the exact source revision being published.

## Redistribution obligations

The six-file script output is a technical installation bundle; it is not, by
itself, a complete statement of redistribution obligations. Before publishing,
include every license, source, attribution, and notice item required by the
licenses that apply to the OpenRDX changes, TI-derived material, bundled
documentation, and the intended distribution model. Start with
[`LICENSE`](../../LICENSE), [`NOTICE`](../../NOTICE),
[`COMMERCIAL-LICENSE.md`](../../COMMERCIAL-LICENSE.md), and
[`.reuse/dep5`](../../.reuse/dep5).

After packaging, follow the checksum verification and target-selection steps in
the bundled `OPENRDX_USB_UPDATE.md` before treating the release as installable.

[Back to the documentation index](../README.md)
