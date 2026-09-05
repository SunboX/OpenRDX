# AGENTS.md rules adaptation design

## Goal

Create a root `AGENTS.md` that preserves the intent and coverage of every rule
in `/Users/afiedler/Documents/projects/widi_core/AGENTS.md` while replacing
WIDI/ESP32-specific facts with accurate rules for the TUSB9261 RDX
PlatformIO wrapper.

## Structure

The new file retains the reference document's sections for project overview,
key files, build and flash, tests, notes, coding style, testing, commits and
merge requests, security, skills, skill triggers and usage, coordination,
context hygiene, missing skills, and safety.

## Project-specific adaptation

- Build commands target the single `tusb9261_ti_cgt` PlatformIO environment.
- Unit tests use Python `unittest`; a complete firmware build requires TI ARM
  CGT 5.2.x.
- Windows keeps CGT 5.2.5, `.exe` tool names, and its existing configured root.
- macOS uses CGT 5.2.9 and extensionless tools through Rosetta where needed.
- GCC and newer TI compilers are not valid fallbacks for the legacy
  `ti_arm9_abi` firmware.
- Compiler flags, firmware linker scripts, absolute symbols, artifact names,
  and the validated Windows hash boundary are protected.
- Flashing is prohibited unless the user explicitly authorizes it and provides
  a separately validated safety procedure; the current artifacts have no
  validated upload target.
- ESP32 pins, Bluetooth names, partition tables, web firmware files, and WIDI
  API paths are omitted because they do not exist in this project.

## Reusable-rule adaptation

- The 1000-line preference, function documentation, explanatory comments,
  behavior-change tests, README documentation, commit-prefix convention, and
  concise GitLab merge-request guidance remain.
- Doxygen applies to C/C++ functions; Python functions use docstrings.
- Git conventions are conditional because this directory currently has no Git
  metadata.
- Secrets remain prohibited. TI credentials and authenticated installers must
  not be committed; `TI_CGT_ROOT` is documented as a non-secret path override.
- Skill discovery, trigger, reading, sequencing, context, missing-skill, and
  fallback rules remain, aligned with the current requirement to read a
  selected `SKILL.md` completely.

## Verification

Compare the headings and rule categories in the reference and new files,
confirm every category is represented, scan for stale WIDI/ESP32 terminology,
and verify all local commands, paths, environment names, ABI names, linker
files, and test commands against the current project.
