# Prepare the board for the "MicroBlaze-V Debug" configuration in .vscode/launch.json.
# Run from the repository root with the Vitis environment sourced: xsdb vscode_debug.tcl
# It leaves hello_world loaded and halted at its entry point; GDB then attaches to
# hw_server's RISC-V 32-bit GDB server on localhost:3004.

# "xsdb vscode_debug.tcl rust" loads the Rust application (sw/rust_apps_libs,
# debug build) instead of the C one; use the "Rust: ..." launch configuration then.
if {[info exists argv] && [lsearch -exact $argv rust] >= 0} {
    set elf ./sw/target/riscv32i-unknown-none-elf/debug/hello-world-rs
} else {
    set elf ./sw/hello_world/build/hello_world.elf
}

# connect -host 173.673.3.2 -port 3121 if remote debugging to hw_server at 173.673.3.2:3121
connect
bpremove -all
targets -set -nocase -filter {name =~"APU*" && jtag_cable_name =~ "Digilent Arty Z7 003017BB13CFA" && jtag_device_ctx=="jsn-Arty Z7-003017BB13CFA-4ba00477-0"}
rst
after 3000

targets -set -filter {jtag_cable_name =~ "Digilent Arty Z7 003017BB13CFA" && level==0 && jtag_device_ctx=="jsn-Arty Z7-003017BB13CFA-23727093-0"}
fpga ./sw/platform/hw/sdt/design_1_wrapper.bit
targets -set -nocase -filter {name =~"APU*" && jtag_cable_name =~ "Digilent Arty Z7 003017BB13CFA" && jtag_device_ctx=="jsn-Arty Z7-003017BB13CFA-4ba00477-0"}
loadhw -hw ./sw/platform/export/platform/hw/design_1_wrapper.xsa -mem-ranges [list {0x40000000 0xbfffffff}]

configparams force-mem-access 1
# Initialize the PS (clocks including FCLK_CLK0, MIO, UART0, PS-PL level shifters)
# from the current XSA instead of running the FSBL
targets -set -nocase -filter {name =~"APU*" && jtag_cable_name =~ "Digilent Arty Z7 003017BB13CFA" && jtag_device_ctx=="jsn-Arty Z7-003017BB13CFA-4ba00477-0"}
source ./sw/platform/hw/sdt/ps7_init.tcl
ps7_init
ps7_post_config

# The Hart target has no jtag_cable_name property, so filter on its MDM parent instead
targets -set -nocase -filter {name =~ "*Hart**#0" && parent=~"**USER2"}
rst -processor
dow $elf
configparams force-mem-access 0
