# OpenRDX documentation restructure design

## Objective

Turn the repository root README into a product-facing landing page and keep the
published `docs/` tree limited to maintained OpenRDX guidance. Move vendor firmware
documentation and superseded planning material into a clearly non-authoritative
`temp/` holding area without deleting it.

## Audience and message

The primary reader is a technically confident owner evaluating OpenRDX for a
TUSB9261-based Tandberg RDX USB 3.0 receiver. The first screen must communicate
the unusual benefit immediately: OpenRDX retains the RDX cartridge workflow and
also provides guarded support for qualifying standard SATA drives.

The title is simply `OpenRDX`. A centered hero, short tagline, native GitHub
callout, three benefit summaries, and direct navigation provide visual hierarchy.
Low-level LED, GPIO, SCSI, timing, compiler, and state-machine details do not
belong on the landing page.

Customer-facing language must distinguish implemented support from hardware
validation. It must not promise support for every SATA disk, full RDX Manager
compatibility, signed firmware, or a fully hardware-qualified integrated image.

## Published documentation structure

```text
docs/
  README.md
  getting-started/
    installation.md
    using-openrdx.md
  development/
    building.md
    building-on-macos.md
    release-process.md
    rom-loader-recovery.md
  reference/
    firmware-behavior.md
    led-output-map.md
    rdx-manager-protocol.md
    ti-sdk-integration.md
  assets/
    openrdx-rdx-quikstor-hero.png
```

`docs/README.md` defines the navigation and authority order. Operator safety
procedures outrank summaries; maintained OpenRDX behavior outranks vendor firmware
documentation; vendor documents and historical plans are never instructions.

## Temporary holding structure

```text
temp/
  README.md
  vendor/
    *.pdf
  history/
    design-notes/
    implementation-plans/
```

The holding tree remains tracked so moves are reversible. All vendor PDFs move
there, including product manuals, release notes, quick-start guides, and component
datasheets. Existing ignored `docs/superpowers/` plans and specifications move to
`temp/history/`. The temporary index records that these files may be obsolete,
may describe vendor firmware rather than OpenRDX behavior, and may have unresolved
redistribution rights.

## Content ownership

- `README.md`: product value, standard-SATA differentiator, supported receiver,
  validation boundary, safety summary, and navigation.
- `docs/getting-started/installation.md`: canonical guarded update workflow.
- `docs/getting-started/using-openrdx.md`: media choice, everyday behavior,
  write protection, ejection, and rejected-media outcomes.
- `docs/development/building.md`: supported toolchain, build/test commands, and
  artifact roles.
- `docs/development/building-on-macos.md`: the verified Wine-based macOS setup.
- `docs/development/release-process.md`: release bundle generation and integrity
  checks, without claiming the current checked-in bundle is valid.
- `docs/development/rom-loader-recovery.md`: J7 and FlashBurner repair workflow.
- `docs/reference/firmware-behavior.md`: detailed implementation and validation
  boundaries, including startup and hot-plug ordering.
- `docs/reference/rdx-manager-protocol.md`: protocol layouts and supported subset.
- `docs/reference/led-output-map.md`: exact electrical mapping and timing.
- Historical source-analysis notes: legacy source-layout and unresolved-symbol
  evidence that remains relevant to maintainers.

## Reorganization rules

Binary files move byte-for-byte and retain their hashes. Every Markdown link,
build-script source path, test constant, AGENTS reference, and REUSE pattern is
updated for the new locations. The release-local `dist/OPENRDX_USB_UPDATE.md`
name remains unchanged even though its canonical source moves.

Documentation contradictions encountered in moved material are corrected rather
than copied: macOS uses Windows TI CGT 5.2.9 through Wine; the J7 procedure lives
in the ROM-loader guide; SHA-256 provides integrity rather than publisher
authentication; target selection is unambiguous rather than necessarily the only
connected device.

## Verification

Verify local link resolution, build-script source paths, the complete Python
suite, the repository license checker, REUSE lint through `uvx`, checksum
comparisons for every moved binary, and `git diff --check`. Review customer prose
directly instead of adding brittle tests that freeze human-facing wording. Do not
run firmware flashing or hardware operations.
