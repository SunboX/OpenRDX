# OpenRDX Documentation Restructure Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the engineering-heavy landing page with a polished product README and reorganize maintained, vendor, and historical documentation by audience and authority.

**Architecture:** The root README becomes a concise customer router. Maintained documents live under audience-specific `docs/` paths, while byte-preserved vendor PDFs and superseded plans live under tracked `temp/`; all path consumers and licensing metadata move atomically with their targets.

**Tech Stack:** GitHub-flavored Markdown, Python `unittest`, PlatformIO repository scripts, Debian DEP5/REUSE metadata.

**Spec:** `temp/history/design-notes/2026-09-04-documentation-restructure-design.md`

## Global Constraints

- Preserve the existing `.vscode/` working-tree changes.
- Preserve the supplied PNG and all moved PDFs byte-for-byte.
- Keep `main` checked out and do not flash hardware.
- Use `standard SATA drives` prominently, qualified by admission checks and the documented validation boundary.
- Keep TI ARM CGT 5.2.5 on Windows and Windows TI ARM CGT 5.2.9 through Wine on macOS.
- Keep `dist/OPENRDX_USB_UPDATE.md` as the packaged filename.
- Never present external research results as OpenRDX behavior.

---

### Task 1: Move assets, vendor sources, and history

**Files:**
- Move: `docs/images/tandberg-data-rdx-quikstor-usb-3-0.png` to `docs/assets/openrdx-rdx-quikstor-hero.png`
- Move: all eight `docs/*.pdf` files to `temp/vendor/`
- Move: `docs/superpowers/plans/*.md` to `temp/history/implementation-plans/`
- Move: `docs/superpowers/specs/*.md` to `temp/history/design-notes/`
- Create: `temp/README.md`
- Modify: `.gitignore`, `.reuse/dep5`
- Modify: `README.md` only to keep the hero-image path valid during the move.
- Modify: `tests/test_product_language.py` to exclude linked worktrees and the
  explicitly non-authoritative temporary holding tree.

**Interfaces:**
- Consumes: the approved holding and asset paths.
- Produces: byte-preserved assets and a clear non-authoritative holding area.

- [ ] **Step 1: Record SHA-256 hashes for the PNG and PDFs.**
- [ ] **Step 2: Create the approved directory tree and move binaries with filesystem rename operations.**
- [ ] **Step 3: Move historical Markdown sources and preserve all filenames.**
- [ ] **Step 4: Add `temp/README.md`, remove the obsolete `/docs/superpowers/` ignore, and classify `docs/assets/*.png`, `temp/history/*.md`, and `temp/vendor/*.pdf` in DEP5.**
- [ ] **Step 5: Update the existing README image target to the new asset path.**
- [ ] **Step 6: Recompute hashes and confirm every moved binary matches its recorded hash.**
- [ ] **Step 7: Exclude `.worktrees/` and `temp/` from the product-language scanner, then run its focused tests.**
- [ ] **Step 8: Run the license checker and REUSE lint.**
- [ ] **Step 9: Commit the structure with `git add .gitignore .reuse/dep5 README.md docs temp tests/test_product_language.py && git commit -m "docs: separate published and source material"`.**

### Task 2: Move canonical documents and update path consumers

**Files:**
- Move: `docs/OPENRDX_USB_UPDATE.md` to `docs/getting-started/installation.md`.
- Move: `docs/RDX_SPECIFIC_FUNCTIONALITY.md` to `docs/reference/firmware-behavior.md`.
- Move: `docs/MACOS_BUILD_SETUP.md` to `docs/development/building-on-macos.md`.
- Move: `docs/TUSB9261_FLASHING_PROCEDURE.md` to `docs/development/rom-loader-recovery.md`.
- Move: `docs/LED_OUTPUT_MAPPING.md` to `docs/reference/led-output-map.md`.
- Move: `docs/RDX_MANAGER_PROTOCOL_IMPLEMENTATION.md` to `docs/reference/rdx-manager-protocol.md`.
- Move: `docs/TI_SDK_INTEGRATION.md` to `docs/reference/ti-sdk-integration.md`.
- Retain `docs/UNRESOLVED_FIRMWARE_SYMBOLS.md` as a historical unresolved-symbol ledger.
- Modify: `AGENTS.md`
- Modify: `build-dist.ps1`
- Modify: `tests/test_build_dist_script.py`
- Modify: `tests/test_main_led_mapping.py`
- Modify: `tests/test_semantic_identifiers.py`
- Modify: every moved Markdown document to keep links valid.

**Interfaces:**
- Consumes: the final documentation paths from the design.
- Produces: a mechanically complete path reorganization with no broken consumer.

- [ ] **Step 1: Move all maintained Markdown documents to the approved audience paths.**
- [ ] **Step 2: Update relative links inside moved documents.**
- [ ] **Step 3: Update every hard-coded moved path while preserving the packaged update-guide filename.**
- [ ] **Step 4: Run the affected build-script, LED-mapping, and semantic-identifier tests.**
- [ ] **Step 5: Run a local Markdown-link resolution check.**
- [ ] **Step 6: Commit the path reorganization with `git add AGENTS.md build-dist.ps1 docs tests && git commit -m "docs: organize maintained documentation"`.**

### Task 3: Rewrite customer and maintainer documentation

**Files:**
- Modify: `README.md`
- Create: `docs/README.md`
- Create: `docs/getting-started/using-openrdx.md`
- Create: `docs/development/building.md`
- Create: `docs/development/release-process.md`
- Modify: every moved canonical Markdown document.

**Interfaces:**
- Consumes: the paths produced by Task 2 and verified firmware behavior from source/tests.
- Produces: one customer landing page and non-duplicating canonical guides.

- [ ] **Step 1: Replace README with the approved centered hero, prominent standard-SATA callout, benefit overview, media comparison, validation boundary, install/build routes, documentation links, and license summary.**
- [ ] **Step 2: Write `docs/README.md` with audience navigation and authority rules.**
- [ ] **Step 3: Write the operating, building, and release guides using only verified repository behavior.**
- [ ] **Step 4: Update moved documents for new relative links and remove duplicated operator/build instructions.**
- [ ] **Step 5: Correct the Wine/Rosetta, J7-location, integrity-language, target-selection, artifact-scope, and startup-ordering drift identified in the design.**
- [ ] **Step 6: Run a local Markdown-link resolution check and review the README directly for the approved hierarchy and absence of implementation-heavy detail.**
- [ ] **Step 7: Commit the content with `git add README.md docs temp && git commit -m "docs: present OpenRDX for RDX and SATA users"`.**

### Task 4: Verify the full reorganization

**Interfaces:**
- Consumes: final documentation and paths from Tasks 1 through 3.
- Produces: full repository verification evidence.

- [ ] **Step 1: Run `rg` over old filenames and accept matches only inside clearly labeled historical material.**
- [ ] **Step 2: Run `~/.platformio/penv/bin/python -m unittest discover -s tests -v` and require zero failures.**
- [ ] **Step 3: Run `~/.platformio/penv/bin/python scripts/check_license_metadata.py .` and require success.**
- [ ] **Step 4: Run `uvx --from 'reuse[charset-normalizer]' reuse lint --root .` and require a compliant result.**
- [ ] **Step 5: Recheck moved binary hashes and all local Markdown links.**
- [ ] **Step 6: Run `git diff --check`, inspect the complete diff, and verify `main...origin/main` plus unrelated `.vscode/` changes remain intact.**
