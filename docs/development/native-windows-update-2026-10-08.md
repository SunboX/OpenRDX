# Native Windows firmware update verification — 8 October 2026

The corrected normal OpenRDX 1.10 image completed an actual update through
OpenRDXManager on native Windows x64 build 26200. The selected TANDBERG RDX
receiver was unit `7820999743`, USB serial `007820999743`, with an empty bay.
This qualifies this image and this receiver's update flow, not every management
operation or another host platform.

## Image and recovery

The native TI ARM CGT 5.2.5 build contains the explicit manufacturing-latch
initializer and guarded BOT cleanup from commit `c12f1d2`. It was packaged as
the complete six-file 1.10 installation bundle with matching checksums.

- Container length: 62,110 bytes.
- Container SHA-256: `4607fc4bced3868e65af13201f7c7aa86f1dd626099780fd95af3473057cdc97`.
- Boot region length: 61,710 bytes.
- Boot region SHA-256: `86dd1fade35225dd69ef09df4b9f13488052613c22dacd198e8aeec423fa865e`.

The previously installed image rejected the Serial capability query with
`05/24/00` on both native Windows SPTI paths, reproducing the macOS and VM result.
The user entered J7 ROM mode, removed the link, and explicitly requested flashing.
A separately compiled personalized bootstrap restored the same receiver's exact
pre-update 256-byte Manufacturing and 64-byte State records before USB startup.
Physical complete SPI readback verified that bootstrap and all preserved records.
Both readiness queries then returned GOOD through direct and buffered SPTI.

The installed TI GUI 2.10.0.0 rejects full 256 KiB images, including in full-image
mode. Recovery therefore used the qualified small already-formatted bootstrap
through the installed burner driver. This did not validate restoring an arbitrary
complete backup through the vendor tool; see [ROM-loader recovery](rom-loader-recovery.md).

## Normal Manager update and independent verification

At 12:12:50–12:12:59 UTC the source-built Windows Manager used its real production
Electron main, sandboxed renderer, preload IPC and native service to send the
normal corrected 1.10 container. Only the file chooser was preselected by the
verification harness. Production file/manifest validation, readiness, backup,
transfer, reconnect and readback checks remained active. The operation finished
with state `succeeded`, stage `complete`.

At 12:13 UTC independent allowlisted read-only probes captured all 262,144 SPI
bytes twice with identical results. The complete comparison matched the normal
boot, erased padding and both pre-update records exactly. Installed SPI SHA-256:

`c44846f67181949a7a7437791aba8f9c872428e1197c92a86672c172a8f23d7f`.

Manufacturing and Serial readiness returned host success and SCSI GOOD through
direct and buffered SPTI after the update. The pre-update complete backup remained
unchanged. Manager reopened with privileged hardware access, the correct unit,
revision 1.10 and firmware-update capability available. Renderer sandbox and
context isolation remained enabled; Node integration remained disabled.

The full investigation is recorded in OpenRDXManager's
`docs/WINDOWS_FLASH_RECOVERY_2026-10-08.md`, with earlier audit reports linked there.
Private snapshots, personalized bootstrap bytes, raw records and detailed
transcripts remain outside both Git repositories in the transfer workspace's
ACL-protected `native-windows-20261008-1/` directory. They are not release assets.

macOS normal updates, other receivers, broader cartridge operations, complete
backup rollback and a signed Windows installer remain separately unqualified.

## Cartridge capacity refresh candidate

A subsequent TI ARM CGT 5.2.5 candidate limits runtime caching-page refreshes
to cache-enable bits. It preserves the admitted RDX data-area capacity and
translation offset instead of rerunning the initialization IDENTIFY parser.
The Python suite completed 307 tests with 39 skips, including an executable
host-C regression probe for the production cache-refresh helper.

This candidate retains version 1.10 and is distinguished by its container
SHA-256 `007d233114a9ba3b36f7cd1e98329bcf03524281aea5dc23cf05dbe9c1e56f10`.
Its boot region SHA-256 is
`fd823afeedf8d5db93fdcbcd59645829a2a1aac20bd3a4f9e86f029ed6bbb046`.
At 16:33:41–16:33:56 UTC it completed the real production Manager update on
the same receiver with the bay empty. Independent repeated complete SPI reads
matched the selected boot, erased padding through 0x10000, and every retained
pre-update byte from 0x10000 through 0x40000. Both readiness routes returned GOOD.
Installed complete SPI SHA-256:
`239b7a0fc44e801c47a59a12fa26c8dd57465ce349bbcbd4ed40ed211b5311f8`.

After reinserting the same 3 TB cartridge, a read-only retained-handle probe at
17:00:36 UTC confirmed a last LBA of 5,860,524,975 with 512-byte sectors:
3,000,588,787,712 bytes. READ16 returned GOOD at LBA 0, 4,294,967,296,
5,860,524,943 and 5,860,524,975. Caching-page MODE SENSE6 and MODE SENSE10
returned GOOD; a subsequent READ CAPACITY16 reported the same last LBA, and
both end-sector reads again returned GOOD. Dock identity was checked before
and after the probe. No cartridge data was written or retained by this check.

These results qualify the corrected capacity and readable end sectors on this
receiver and cartridge. Successful formatting remains unverified until a
separate user-initiated format completes. Private raw evidence remains in
`native-windows-20261008-1/format-capacity-update-2/` outside the repositories.
