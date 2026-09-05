# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""PlatformIO/SCons adapter for the validated TI ARM CGT 5.2.5 build."""

from pathlib import Path
import shutil
import subprocess
import sys

Import("env")  # type: ignore[name-defined]  # Provided by PlatformIO/SCons.

SCRIPT_DIR = Path(env.subst("$PROJECT_DIR")) / "scripts"
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from ti_cgt_tools import (
    align_up,
    format_ti_response_file,
    get_intel_hex_address_span,
    resolve_ti_cgt_tools,
)


PROJECT_DIR = Path(env.subst("$PROJECT_DIR"))
BUILD_DIR = Path(env.subst("$BUILD_DIR"))
tools = resolve_ti_cgt_tools(
    env.GetProjectOption("custom_ti_cgt_root"),
    macos_root=env.GetProjectOption("custom_ti_cgt_root_macos", ""),
)
TI_ROOT = tools.root
ARMCL = tools.compiler
ARMHEX = tools.hex_converter
TI_INCLUDE = tools.include_dir
PROJECT_INCLUDE = PROJECT_DIR / "include" / "rdx_mount"
SOURCE_DIR = PROJECT_DIR / "src" / "rdx_mount"
LINKER_DIR = PROJECT_DIR / "linker"

PROGRAM = BUILD_DIR / "TUSB9261_RDX.out"
MAP_FILE = BUILD_DIR / "TUSB9261_RDX.map"
HEX_FILE = BUILD_DIR / "TUSB9261_RDX.hex"
FLASH_HEX_FILE = BUILD_DIR / "TUSB9261_RDX_flash.hex"
FLASH_COMMAND_FILE = BUILD_DIR / "TUSB9261_RDX_flash.cmd"
OBJECT_DIR = BUILD_DIR / "ti-objects"
OBJECT_LIST = BUILD_DIR / "objects.opt"

C_SOURCES = sorted(SOURCE_DIR.glob("*.c"))
ASM_SOURCES = sorted(SOURCE_DIR.glob("*.asm"))
HEADERS = sorted(PROJECT_INCLUDE.rglob("*.h"))


def run(command, working_directory=PROJECT_DIR):
    """Run one TI tool command in the requested working directory."""
    print(subprocess.list2cmdline([str(item) for item in command]))
    return subprocess.run(
        [str(item) for item in command], cwd=str(working_directory)
    ).returncode


def build_with_ti_cgt(target, source, env):
    del target, source, env
    if not ARMCL.is_file():
        print("TI ARM compiler not found: {}".format(ARMCL))
        return 2
    if not ARMHEX.is_file():
        print("TI ARM hex converter not found: {}".format(ARMHEX))
        return 3

    if OBJECT_DIR.exists():
        shutil.rmtree(str(OBJECT_DIR))
    OBJECT_DIR.mkdir(parents=True, exist_ok=True)
    BUILD_DIR.mkdir(parents=True, exist_ok=True)

    common = [
        str(ARMCL),
        "-c",
        "-mv7m3",
        "--abi=ti_arm9_abi",
        "-me",
        "--gcc",
        "--verbose_diagnostics",
        "--display_error_number",
        "--emit_warnings_as_errors",
        "--symdebug:none",
        "--obj_directory={}".format(OBJECT_DIR),
        "--include_path={}".format(PROJECT_INCLUDE),
        "--include_path={}".format(TI_INCLUDE),
    ]

    for source_path in C_SOURCES:
        # OpenRDX requires -O3 for the interrupt-fed memory wrap-window path.
        # Lower optimization can keep the USB/SATA producer-consumer pipeline
        # from servicing the first host-sector transfer within its deadline.
        result = run(common + ["-O3", str(source_path)])
        if result:
            return result
    for source_path in ASM_SOURCES:
        result = run(common + [str(source_path)])
        if result:
            return result

    objects = sorted(OBJECT_DIR.glob("*.obj"))
    if len(objects) != len(C_SOURCES) + len(ASM_SOURCES):
        print(
            "Expected {} objects but found {}".format(
                len(C_SOURCES) + len(ASM_SOURCES), len(objects)
            )
        )
        return 4
    OBJECT_LIST.write_text(
        format_ti_response_file(objects, PROJECT_DIR), encoding="ascii"
    )

    link_command = [
        str(ARMCL),
        "-mv7m3",
        "--abi=ti_arm9_abi",
        "-me",
        "-@={}".format(OBJECT_LIST),
        "-z",
        str(LINKER_DIR / "tusb9260_link.cmd"),
        "--disable_auto_rts",
        "-m={}".format(MAP_FILE),
        "--rom_model",
        "--verbose_diagnostics",
        "--warn_sections",
        "-o={}".format(PROGRAM),
    ]
    result = run(link_command)
    if result:
        return result

    hex_command = [
        str(ARMHEX),
        "--intel",
        "--byte",
        "--memwidth=8",
        "--romwidth=8",
        "--outfile={}".format(HEX_FILE),
        str(PROGRAM),
    ]
    result = run(hex_command)
    if result:
        return result

    try:
        image_start, image_end = get_intel_hex_address_span(HEX_FILE)
    except (OSError, ValueError) as error:
        print("Unable to determine Intel HEX image span: {}".format(error))
        return 5
    if image_start != 0x08000000:
        print("Unexpected firmware image origin: 0x{:08X}".format(image_start))
        return 6

    # FlashBurner 1.3.1 rejects a final short Intel HEX data record (for
    # example, "Line 1096 is too short" for a one-byte tail).  Round the ROM
    # span to armhex's 32-byte record width so --image fills a complete final
    # record with 0xFF without changing any linked firmware byte.
    image_length = align_up(image_end - image_start + 1, 32)
    FLASH_COMMAND_FILE.write_text(
        (
            "TUSB9261_RDX.out\n"
            "--intel\n"
            "--byte\n"
            "--image\n\n"
            "ROMS\n"
            "{{\n"
            "    FLASH: origin = 0x{origin:08X},\n"
            "           length = 0x{length:X},\n"
            "           romwidth = 8,\n"
            "           memwidth = 8,\n"
            "           fill = 0xFFFFFFFF,\n"
            "           files = {{ TUSB9261_RDX_flash.hex }}\n"
            "}}\n"
        ).format(origin=image_start, length=image_length),
        encoding="ascii",
    )
    result = run([str(ARMHEX), FLASH_COMMAND_FILE.name], BUILD_DIR)
    if result:
        return result

    print("TI CGT build completed: {}".format(PROGRAM))
    print("Intel HEX completed: {}".format(HEX_FILE))
    print("FlashBurner HEX completed: {}".format(FLASH_HEX_FILE))
    return 0


dependencies = C_SOURCES + ASM_SOURCES + HEADERS + [
    LINKER_DIR / "tusb9260_link.cmd",
]
firmware = env.Command(
    [str(PROGRAM), str(MAP_FILE), str(HEX_FILE), str(FLASH_HEX_FILE)],
    [str(path) for path in dependencies],
    build_with_ti_cgt,
)


def build_program_with_ti_cgt(build_env):
    """Replace PlatformIO's native Program builder with the TI firmware build."""
    build_env.Replace(PIOMAINPROG=firmware[0])
    return firmware


env.AddMethod(build_program_with_ti_cgt, "BuildProgram")
