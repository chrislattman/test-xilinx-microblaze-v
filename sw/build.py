"""Build the platform and hello_world, then embed hello_world.elf into the bitstream.

Run from this folder with the Vitis environment sourced:
    vitis -s build.py [--release] [--clean | --distclean]
"""
import argparse
import os
import subprocess

import vitis
from vitis.component import HostComponent
from vitis.platform_component import Platform

APP_NAMES = ["hello_world"]
MB_DOMAIN = "standalone_microblaze_riscv_0"
MB_INST_PATH = "design_1_i/microblaze_riscv_0"  # processor InstPath in design_1_wrapper.mmi

SW = os.path.dirname(os.path.abspath(__file__))
PLATFORM_HW = os.path.join(SW, "platform/hw/sdt")  # regenerated on every XSA update
BOOT_BIT = os.path.join(SW, "build/design_1_wrapper.bit")

release_mode = False
parser = argparse.ArgumentParser()
parser.add_argument("--release", action="store_true")
clean_group = parser.add_mutually_exclusive_group()
clean_group.add_argument("--clean", action="store_true",
                         help="clean the application components only")
clean_group.add_argument("--distclean", action="store_true",
                         help="clean the application components and the platform (BSPs)")
args = parser.parse_args()
if args.release:
    release_mode = True

client = vitis.create_client()
client.update_workspace(SW)

platform = client.get_component("platform")
assert isinstance(platform, Platform)

if args.clean or args.distclean:
    for app_name in APP_NAMES:
        app = client.get_component(app_name)
        assert isinstance(app, HostComponent)
        app.clean()
    if args.distclean:
        platform.clean()
    vitis.dispose()
    raise SystemExit(0)

# platform.update_hw(hw_design="../fpga/design_1_wrapper.xsa")  # re-read the XSA if desired
# Keep -ffunction-sections/-fdata-sections: with --gc-sections they keep the app inside
# the MicroBlaze-V's 32 KB of BRAM.
bsp_flags = "-O2 -ffunction-sections -fdata-sections -Wall -Wextra -fno-tree-loop-distribute-patterns"
if not release_mode:
    bsp_flags = "-O2 -g -ffunction-sections -fdata-sections -Wall -Wextra -fno-tree-loop-distribute-patterns"
mb_domain = platform.get_domain(MB_DOMAIN)
mb_domain.set_config(option="proc", param="proc_extra_compiler_flags", value=bsp_flags)
# The Arty Z7's USB-UART is PS UART0 (MIO 14/15). A freshly cleaned BSP can fall back to
# the default console, so set it explicitly for every domain.
for domain in platform.list_domains():
    domain_object = platform.get_domain(domain["domain_name"])
    for param in ["standalone_stdin", "standalone_stdout"]:
        domain_object.set_config(option="os", param=param, value="ps7_uart_0")
platform.build()

for app_name in APP_NAMES:
    app = client.get_component(app_name)
    assert isinstance(app, HostComponent)
    if release_mode:
        app.set_app_config(key="USER_COMPILE_DEBUG_LEVEL", values="")
        app.set_app_config(key="USER_COMPILE_OPTIMIZATION_LEVEL", values="-O2")
    else:
        app.set_app_config(key="USER_COMPILE_DEBUG_LEVEL", values="-g3")
        app.set_app_config(key="USER_COMPILE_OPTIMIZATION_LEVEL", values="-O0")
    app.build()

vitis.dispose()

# The MicroBlaze-V runs from its LMB BRAM, which the FSBL can't write (it isn't on the
# PS's bus), so a boot image carries the app inside the bitstream's BRAM contents instead.
# system.bif uses this bitstream. The bitstream and .mmi come from platform/hw/sdt, since
# a platform build from this API leaves only the XSA in platform/export/platform/hw.
os.makedirs(os.path.dirname(BOOT_BIT), exist_ok=True)
subprocess.run(["updatemem", "-force",
                "-meminfo", os.path.join(PLATFORM_HW, "design_1_wrapper.mmi"),
                "-bit", os.path.join(PLATFORM_HW, "design_1_wrapper.bit"),
                "-data", os.path.join(SW, "hello_world/build/hello_world.elf"),
                "-proc", MB_INST_PATH,
                "-out", BOOT_BIT],
               check=True, cwd=os.path.dirname(BOOT_BIT))
