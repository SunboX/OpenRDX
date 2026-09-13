# OpenRDX documentation

Use this index to find the maintained guide for your task. The root
[`README.md`](../README.md) introduces the product; this page defines where the
authoritative details live.

## Start here

### Owners and operators

- [Install OpenRDX](getting-started/installation.md) — safety checks, target
  selection, release verification, and the guarded update workflow.
- [Update existing OpenRDX](getting-started/installation.md#update-existing-openrdx)
  — the in-place route for existing OpenRDX receivers and its validation limits.
- [Use OpenRDX](getting-started/using-openrdx.md) — choose media, understand
  write protection and ejection, and respond to rejected media.

### Maintainers

- [Build and test](development/building.md) — supported toolchains, commands,
  and build artifacts.
- [Build on macOS](development/building-on-macos.md) — verified Windows TI
  toolchain setup through Wine.
- [Create a release](development/release-process.md) — generate and validate
  the six-file distribution bundle.
- [Firmware recovery](development/rom-loader-recovery.md) — recovery and
  restoration instructions.
- [In-place update evidence](development/openrdx-update-validation.md) — source
  trace, simulated host tests, and outstanding device validation.

### Technical reference

- [Firmware behavior](reference/firmware-behavior.md) — device behavior,
  implementation evidence, and validation limits.
- [RDX Manager protocol](reference/rdx-manager-protocol.md) — implemented
  discovery, status, control, and update protocol details.
- [Manufacturing restore](reference/manufacturing-restore.md) — full-record
  recovery, required host checks, and validation limits.
- [LED output map](reference/led-output-map.md) — electrical ownership,
  polarity, and indicator sequencing.
- [TI SDK integration](reference/ti-sdk-integration.md) — maintained evidence
  for mapped controller structures and fixed-address bindings.

## Authority order

When two sources appear to disagree, use this order:

1. Operator safety and installation procedures, especially
   [installation.md](getting-started/installation.md) and
   [rom-loader-recovery.md](development/rom-loader-recovery.md).
2. Maintained OpenRDX behavior documentation, led by
   [using-openrdx.md](getting-started/using-openrdx.md) and
   [firmware-behavior.md](reference/firmware-behavior.md).
3. Technical reference documents for implementation-level detail.
4. Files under [`temp/`](../temp/README.md), including vendor documents and
   historical plans. They are non-authoritative and may be obsolete, describe
   behavior outside the implemented OpenRDX feature set, or carry unresolved
   redistribution terms.

Source code and automated tests remain the deciding evidence for the current
implementation. A build or test result is not a substitute for physical-device
validation when a guide identifies that boundary.
