# Create an OpenRDX release bundle

This maintainer guide describes the Windows release gate driven by
`build-dist.ps1` and its GitHub Actions automation. Normal PlatformIO builds
also generate the complete `dist/` bundle; the release command adds the full
automated test gate before that build.

## GitHub build and release automation

The [GitHub Actions workflows](https://github.com/SunboX/OpenRDX/actions) build,
test, and package OpenRDX in these cases:

| Trigger | Source selected | Result |
| --- | --- | --- |
| Push to `main`, including a merged pull request | The pushed commit | Downloadable workflow build artifacts |
| Manual `workflow_dispatch` | The selected ref | Downloadable workflow build artifacts |
| GitHub release `published` | That release's exact tag | Build artifacts attached to that release |

Every release tag must have the form `v<MAJOR.MINOR>`, with two minor-version
digits: for example, `v1.06`. Its version must match both `VERSION` and the
compiled firmware version in `include/rdx_mount/tusb9260.h`. A release build
uses the tag's source commit, even if `main` has advanced since that tag was
created. A version mismatch stops publication of the build assets.
The tagged commit must also belong to `main`.

The workflow runs the supported Windows TI ARM CGT 5.2.5 build, the automated
suite, and the complete packaging checks. Its downloaded build inputs are
checked against their pinned digests. Build records identify the source commit
and build environment. Consult the actual workflow result before calling an
artifact verified; configuring the workflow does not establish a successful
build or any device-validation result.

Pushing to `main` does **not** create a GitHub release. Releases are deliberate
publications, and each published release triggers artifact attachment. Wait for
that release run to succeed and verify its Assets list before directing users
to the download.

### Publish a release

1. Set `VERSION` and the compiled firmware version together, update the release
   notes under `docs/releases/`, and merge or push the reviewed sources to
   `main`.
2. Wait for the matching `main` workflow to pass. Inspect its build records,
   checksums, installation bundle, and compiler outputs.
3. Create the matching version tag on that exact commit. Prepare a draft GitHub
   release with detailed release notes, supported hardware, validation limits,
   and links to the installation and recovery guides at that tag.
4. Prefer uploading the verified artifacts from that exact commit to the draft
   before publishing it, so downloads are available immediately. For command-line
   publication, `gh release upload` can attach files to the draft before
   `gh release edit --draft=false` publishes it.
5. Publish the release. The `published` workflow builds the tagged sources and
   attaches the complete artifact set. Confirm the run succeeded, the tag and
   build records agree, and every expected asset is present.

Never attach artifacts from a different commit merely because their filenames
have the same version. An experimental release should retain that description
in its notes and can use GitHub's prerelease designation.

## Before packaging

- Use the supported Windows TI ARM CGT 5.2.5 build environment from
  [Build and test OpenRDX](building.md).
- Provide the pinned compatibility-image template, either below the sibling
  `OpenRDXManager` tree, through `-TemplatePath` or `OPENRDX_TEMPLATE_PATH`, or in
  the verified `.pio/rdx-template/` cache created by an earlier build.
- Ensure `VERSION` matches the compiled major and minor version in
  `include/rdx_mount/tusb9260.h`.
- Run from the intended, reviewed source tree. `-SkipTests` is appropriate only
  when the complete suite already passed for those exact sources.

## Generate the bundle

From Windows PowerShell in the repository root:

```powershell
.\build-dist.ps1
```

For a local macOS bundle with the full test gate, run the same script with
PowerShell 7. It defaults to the PlatformIO executables under
`~/.platformio/penv/bin/`; `-PlatformIoPath`, `-PythonPath`, and `-TemplatePath`
can override the defaults. Normal PlatformIO builds package through Python and
do not require PowerShell.
This uses the configured TI CGT 5.2.9/Wine toolchain; it does not establish
byte identity with the Windows TI CGT 5.2.5 release build.

The script performs one release transaction:

1. validates the requested release version and required tool paths;
2. runs the complete automated suite unless `-SkipTests` was explicitly used;
3. builds OpenRDX with the configured TI toolchain;
4. invokes the shared Python packager, also used by every normal build;
5. stages the container, manifest, continuous FlashBurner image, guarded updater,
   installation guide, and checksum file;
6. validates the version, container length, manifest digests, and checksums; and
7. replaces only the repository's `dist/` directory with the complete staged
   bundle, removing retired artifacts.

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

## GitHub release assets

For `v1.06`, the GitHub packaging step provides the following additional files,
alongside all six individual installation files listed above:

| File | Role |
| --- | --- |
| `OpenRDX-v1-06.zip` | Complete installation bundle, plus documentation, license notices, and build information |
| `OpenRDX-v1-06-build.zip` | Compiler and linker outputs for maintainers |
| `SHA256SUMS` | SHA-256 integrity list for the downloadable release assets |

Use the installation ZIP for installation. The compiler-output ZIP contains
development artifacts and does not replace the complete installation bundle.
The bundled `.sha256` list validates the five accompanying installation files;
the outer `SHA256SUMS` list validates the downloadable assets, including the
archives. Both lists must be generated from the exact files being uploaded.

The installation ZIP includes the matching installation and recovery
documentation, source/build information, and applicable license and attribution
files. The release notes link to the tagged source so users can find the code
and instructions corresponding to their download. GitHub's automatic source
archives contain the source tree; the separately attached installation ZIP is
the prepared firmware bundle.

## Integrity, identity, and version checks

SHA-256 detects accidental or malicious byte changes relative to the expected
digest. It does not authenticate the publisher, prove that a file was signed,
or establish who created the bundle. OpenRDX firmware is not advertised as
signed firmware.

The `.sha256` file lists all five other bundle files. The operator's checksum
loop in the packaged guide validates every listed file. The updater separately
validates target identity and state, reads the manifest, hashes the selected
firmware container for updates of running OpenRDX receivers. Compatibility
installation is disabled before staging because the generic FlashBurner route
does not preserve receiver-specific records. The updater does not independently hash
itself or the procedure, and it does not authenticate the manifest or checksum
file. The manual whole-bundle checksum step remains mandatory.

The manifest and checksum list must agree with the exact packaged files. The
release version must also match `VERSION` and the compiled firmware version;
changing only a filename cannot create a valid new version.

## The checked-in `dist/` snapshot

Every normal build refreshes `dist/` for the local source tree. The files alone
do not establish that the full automated suite or hardware validation passed.
Before publication, run the release gate from the exact source revision being
published and verify the generated checksums. Rebuild after changing any bundled
updater or installation guide so their bytes and digests remain synchronized.

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
