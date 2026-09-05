import unittest
from pathlib import Path

from scripts import ti_cgt_tools
from scripts.ti_cgt_tools import resolve_ti_cgt_tools


class ResolveTiCgtToolsTests(unittest.TestCase):
    def test_response_file_uses_ascii_project_relative_object_paths(self):
        project_dir = Path("/tmp/Andre\u0301s/project")
        object_path = project_dir / ".pio/build/firmware/ti-objects/ahci.obj"
        formatter = getattr(
            ti_cgt_tools,
            "format_ti_response_file",
            lambda paths, _project_dir: "".join(
                '"{}"\n'.format(path) for path in paths
            ),
        )

        response = formatter([object_path], project_dir)

        self.assertEqual(
            response,
            '".pio/build/firmware/ti-objects/ahci.obj"\n',
        )
        response.encode("ascii")

    def test_windows_preserves_existing_root_and_exe_names(self):
        tools = resolve_ti_cgt_tools(
            r"C:\ti\ti-cgt-arm_5.2.5",
            macos_root="~/ti/ti-cgt-arm_5.2.9",
            environ={},
            host_system="Windows",
        )

        self.assertEqual(tools.root, Path(r"C:\ti\ti-cgt-arm_5.2.5"))
        self.assertEqual(tools.compiler.name, "armcl.exe")
        self.assertEqual(tools.hex_converter.name, "armhex.exe")

    def test_macos_uses_macos_root_and_extensionless_names(self):
        tools = resolve_ti_cgt_tools(
            r"C:\ti\ti-cgt-arm_5.2.5",
            macos_root="/opt/ti/ti-cgt-arm_5.2.9",
            environ={},
            host_system="Darwin",
        )

        self.assertEqual(tools.root, Path("/opt/ti/ti-cgt-arm_5.2.9"))
        self.assertEqual(tools.compiler.name, "armcl")
        self.assertEqual(tools.hex_converter.name, "armhex")

    def test_environment_override_has_highest_priority(self):
        tools = resolve_ti_cgt_tools(
            r"C:\ti\ti-cgt-arm_5.2.5",
            macos_root="/opt/ti/ti-cgt-arm_5.2.9",
            environ={"TI_CGT_ROOT": "/custom/ti-cgt"},
            host_system="Darwin",
        )

        self.assertEqual(tools.root, Path("/custom/ti-cgt"))


if __name__ == "__main__":
    unittest.main()
