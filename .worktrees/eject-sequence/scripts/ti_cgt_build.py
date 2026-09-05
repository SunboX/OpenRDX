"""PlatformIO/SCons adapter for the validated TI ARM CGT 5.2.5 build."""

from pathlib import Path
import shutil
import subprocess
import sys

Import("env")  # type: ignore[name-defined]  # Provided by PlatformIO/SCons.

SCRIPT_DIR = Path(env.subst("$PROJECT_DIR")) / "scripts"
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from ti_cgt_tools import format_ti_response_file, resolve_ti_cgt_tools


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
PROJECT_INCLUDE = PROJECT_DIR / "include"
SOURCE_DIR = PROJECT_DIR / "src"
LINKER_DIR = PROJECT_DIR / "linker"

PROGRAM = BUILD_DIR / "TUSB9261_RDX.out"
MAP_FILE = BUILD_DIR / "TUSB9261_RDX.map"
HEX_FILE = BUILD_DIR / "TUSB9261_RDX.hex"
OBJECT_DIR = BUILD_DIR / "ti-objects"
OBJECT_LIST = BUILD_DIR / "objects.opt"

C_SOURCE_NAMES = ["main.c", "startup.c"]
ASM_SOURCE_NAMES = ["exceptions_isr.asm", "intvecs.asm"]
C_SOURCES = [SOURCE_DIR / name for name in C_SOURCE_NAMES]
ASM_SOURCES = [SOURCE_DIR / name for name in ASM_SOURCE_NAMES]
HEADERS = sorted(PROJECT_INCLUDE.rglob("*.h"))


def run(command):
    print(subprocess.list2cmdline([str(item) for item in command]))
    return subprocess.run([str(item) for item in command], cwd=str(PROJECT_DIR)).returncode


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
        result = run(common + ["-O0", str(source_path)])
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

    print("TI CGT build completed: {}".format(PROGRAM))
    print("Intel HEX completed: {}".format(HEX_FILE))
    return 0


dependencies = C_SOURCES + ASM_SOURCES + HEADERS + [
    LINKER_DIR / "tusb9260_link.cmd",
]
firmware = env.Command(
    [str(PROGRAM), str(MAP_FILE), str(HEX_FILE)],
    [str(path) for path in dependencies],
    build_with_ti_cgt,
)


def build_program_with_ti_cgt(build_env):
    """Replace PlatformIO's native Program builder with the TI firmware build."""
    build_env.Replace(PIOMAINPROG=firmware[0])
    return firmware


env.AddMethod(build_program_with_ti_cgt, "BuildProgram")
