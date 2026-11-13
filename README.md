# MicroBlaze-V on Zynq-7000

This project is an example of a bare-metal application running on a **MicroBlaze-V** (AMD's RISC-V soft processor) in the programmable logic (PL) of a Zynq-7000 SoC, specifically the Arty Z7-20 board.
The hardware was built in Vivado 2025.2 and the software in Vitis Unified IDE 2025.2.

The `hello_world` application:

- prints `Hello` / `World!` over the PS's UART0 (the board's USB-UART, `/dev/ttyUSB1` at 115200 baud)
- blinks LD0 at 0.5 Hz through an AXI GPIO
- kicks an AXI Timebase Watchdog every 2 seconds; if the loop stops, the watchdog resets the MicroBlaze-V

## Hardware

The Cortex-A9 cores don't run the application. The PS supplies the clock and reset for the PL, and the MicroBlaze-V borrows PS peripherals (UART0) through the PS's `S_AXI_GP0` slave port.

```
Zynq PS
  FCLK_CLK0 (100 MHz) ──► clocks everything in the PL
  FCLK_RESET0_N ──► rst_ps7_0_100M ◄── wdt_reset ──┐
                          │ resets                 │
                          ▼                        │
                    MicroBlaze-V ── LMB ── 32 KB BRAM (code + data)
                          │ AXI                    │
                          ▼                        │
                    SmartConnect                   │
                     ├─ M00 ──► PS S_AXI_GP0 ──► UART0 ── MIO 14/15 ── USB-UART
                     ├─ M01 ──► AXI Timebase WDT ──┘
                     └─ M02 ──► AXI GPIO ──► LD0..LD3
```

MicroBlaze-V data address map:

| Address      | Size   | Target                                  |
|--------------|--------|-----------------------------------------|
| `0x00000000` | 32 KB  | Local BRAM (code and data)              |
| `0x10000000` | 256 MB | PS DDR (via `S_AXI_GP0`, unused)        |
| `0x40000000` | 64 KB  | AXI GPIO (`leds_4bits`)                 |
| `0x41A00000` | 64 KB  | AXI Timebase Watchdog                   |
| `0xE0000000` | 4 MB   | PS peripherals (UART0 at `0xE0000000`)  |
| `0xFC000000` | 16 MB  | PS QSPI linear (unused)                 |

On the Arty Z7, the USB-UART connects only to PS MIO 14/15 (UART0). A UART in the PL (e.g. AXI UART Lite) can't drive those pins and would need an external adapter such as a Pmod USBUART.

## Layout

- `fpga/` - Vivado project (`fpga.xpr`, block design `design_1`) and the exported `design_1_wrapper.xsa`
- `sw/` - Vitis workspace
    - `platform/` - platform component with two domains: `standalone_microblaze_riscv_0` and the `zynq_fsbl` boot domain
    - `hello_world/` - the MicroBlaze-V application (`src/helloworld.c`)

## Building

1. **Hardware:** open `fpga/fpga.xpr` in Vivado, generate the bitstream, and export the hardware (including the bitstream) to `fpga/design_1_wrapper.xsa`
2. **Software:** open `sw/` as the workspace in Vitis. If the hardware changed, update the platform from the new XSA (it's linked to `fpga/design_1_wrapper.xsa`). Build `platform`, then build `hello_world`
    - Or, from `sw/`, run `vitis -s build.py` (`--release`, `--clean` and `--distclean` are also supported). It builds the platform and `hello_world`, then embeds `hello_world.elf` into the bitstream's BRAM with `updatemem` (`sw/build/design_1_wrapper.bit`). `vitis -s` exits with 0 even if the script fails, so check its output

To boot from QSPI or SD, run `bootgen -image system.bif -arch zynq -o build/BOOT.bin -w` from `sw/` after `build.py`. The image holds the FSBL and the bitstream with the app already in BRAM: the MicroBlaze-V's BRAM isn't on the PS's bus, so the FSBL can't load the ELF itself.

Launch Vitis from a shell where `source <Vitis install>/settings64.sh` has run, so Vitis's own `riscv64-unknown-elf-gcc` comes first on `PATH`. A distro RISC-V GCC (e.g. Ubuntu's `gcc-riscv64-unknown-elf`) has no C library, and the BSP fails with `string.h: No such file or directory`. If that happens, delete `bsp/libsrc/build_configs/gen_bsp` so CMake searches for the compiler again.

## Running

Use the `hello_world` launch configuration in Vitis. It:

1. programs the FPGA with `sw/platform/hw/sdt/design_1_wrapper.bit`
2. initializes the PS over JTAG with `sw/platform/hw/sdt/ps7_init.tcl` (`ps7_init` and `ps7_post_init`): PLLs and clocks (including `FCLK_CLK0`, which clocks the MicroBlaze-V), MIO, UART0 at 115200 and the PS-PL level shifters. The FSBL isn't used
3. downloads and runs `hello_world.elf` on `microblaze_riscv_0`

Both files are the platform's copies, which Vitis regenerates whenever the platform is updated from a new XSA, so the launch always matches the current hardware.

Then open the UART, e.g. `picocom -b 115200 /dev/ttyUSB1`. `/dev/ttyUSB0` is the FTDI's JTAG channel.

To debug from VS Code instead, run `xsdb vscode_debug.tcl` from the repository root (with the Vitis environment sourced), which does the same steps and leaves the app halted at its entry point, then start the "MicroBlaze-V Debug" configuration in `.vscode/launch.json`. It attaches `gdb-multiarch` to hw_server's RISC-V 32-bit GDB server on `localhost:3004`.

## Notes

- The MicroBlaze-V BSP's `standalone_stdout` / `standalone_stdin` must be `ps7_uart_0`. If they name a UART that isn't enabled in the hardware, the BSP builds without a console (`STDOUT_BASEADDRESS` is undefined in `bspconfig.h`) and nothing prints
- The watchdog's timeout is set by the IP's WDT Interval parameter (2^N clock cycles). The first timeout sets the "expired" flag and the second asserts `wdt_reset`, which goes to `aux_reset_in` on `rst_ps7_0_100M`. Only the PL logic is reset, so the JTAG and UART connections stay up and the app restarts from `0x0` in BRAM
- The FSBL (`sw/platform/zynq_fsbl`) is only needed to boot from SD or QSPI. Vitis writes its `ps7_init.c` / `ps7_init.h` only when the FSBL is created, so they are symlinks to `sw/platform/hw/sdt/`, which every XSA update regenerates. If Vitis ever recreates the FSBL and replaces the links with files, recreate them with `ln -sf ../hw/sdt/ps7_init.c ps7_init.c` and `ln -sf ../hw/sdt/ps7_init.h ps7_init.h` in `sw/platform/zynq_fsbl`
- Generated BSP headers in `bsp/include/` are ignored by git, except `xparameters.h`. It's generated only when a domain is created, so a build can't regenerate it if it's deleted

## Rust application

`sw/` is also a Cargo workspace (target `riscv32i-unknown-none-elf`, set in `.cargo/config.toml`) with a `no_std` Rust port of the application, built on bindings to the MicroBlaze-V's standalone BSP:

- `rust_apps_libs/hello_world_rs`: port of `hello_world/src/helloworld.c`
- `rust_platform/xil_mbv_0_sys`: bindgen bindings to `platform/microblaze_riscv_0/standalone_microblaze_riscv_0/bsp`
- `rust_platform/xil_bsp_build`: host-side `build.rs` helper the sys crate uses (bindgen, C shims, link setup)

The C BSP must be built before running `cargo build`: build the platform in Vitis first (`vitis -s build.py` builds it along with the C application), since the bindings are generated from the BSP's `include/` and link against its `lib/*.a`. Then source the Vitis environment and run `cargo build` (or `cargo build --release`) from `sw/`. The ELF is `sw/target/riscv32i-unknown-none-elf/<debug|release>/hello-world-rs`.

To debug it, use the "Rust: MicroBlaze-V Debug" configuration in `.vscode/launch.json` (Native Debug extension with `rust-gdb`). Run `xsdb vscode_debug.tcl rust` first, which loads the Rust ELF instead of the C one.

To boot it from QSPI or SD, embed the release ELF into the bitstream and use `system_rust.bif`. `updatemem` only accepts files ending in `.elf`, so copy it first. From `sw/`:

```
cargo build --release
cp target/riscv32i-unknown-none-elf/release/hello-world-rs build/hello-world-rs.elf
cd build && updatemem -force -meminfo ../platform/hw/sdt/design_1_wrapper.mmi -bit ../platform/hw/sdt/design_1_wrapper.bit \
    -data hello-world-rs.elf -proc design_1_i/microblaze_riscv_0 -out design_1_wrapper_rust.bit && cd ..
bootgen -image system_rust.bif -arch zynq -o build/BOOT_rust.bin -w
```

### Debugging the Rust application in Vitis

`hello_world/_ide/launch.json` has a second configuration, `hello_world_rs_app_hw_1`, which is a copy of `hello_world_app_hw_1` that downloads the Rust ELF instead of the C one. Run `cargo build` from `sw/` first, then launch it in Vitis as usual.

- Vitis doesn't build the Rust application, so run `cargo build` before every launch or Vitis loads a stale ELF
- The `elfFile` path points straight at the Cargo output (`${workspaceFolder}/target/riscv32i-unknown-none-elf/debug/hello-world-rs`), without a `.elf` extension, so edit `launch.json` directly to change it (e.g. to the `release` build)
- The application kicks the AXI Timebase Watchdog every 2 seconds, so keep it halted for less than the watchdog's timeout, or the PL (including the MicroBlaze-V) is reset

Notes:

- `hello_world_rs/linker.ld` is a copy of the C application's `lscript.ld`. Keep it in sync when you change the memory map. The whole application has to fit in the 32 KB of BRAM along with its 1 KB stack and 2 KB heap
- The whole BSP is exposed: `build.rs` generates a wrapper that includes every header in the BSP's `include/` directory, so every driver, `libxil*` function, struct, enum, typedef, global and constant `#define` is available. Regenerating the platform with more drivers picks them up automatically
    - `SKIP_HEADERS` in `rust_platform/xil_bsp_build/src/lib.rs` lists headers left out because they can't be parsed for this target (`xil_cache_vxworks.h`, `xil_macroback.h`)
    - bindgen drops constant `#define`s it can't evaluate (e.g. ones with casts or function calls)
- `static inline` functions (`Xil_In32`, `Xil_Out32`, ...) are wrapped automatically. Function-like macros can't be, so only the ones in `rust_platform/xil_bsp_build/c/xilrs_shim.c` are available (`XGpio_WriteReg`, `XWdtTb_ReadReg`, `fence`, ...). Add a `xilrs_<Name>` wrapper there when you need another. Both kinds are called from Rust by their C names
- `XPAR_*` constants are `u32`, but some drivers take `UINTPTR` (`usize`), so cast with `as UINTPTR` where C would convert implicitly
- The BSP archives, `libgcc`, `libgloss` and newlib's `libc` link as one group, which requires linking with (Vitis) `riscv64-unknown-elf-gcc`. The app's `build.rs` passes the BSP's `Xilinx.spec` and its linker script
- The core has no atomic (A) or multiply (M) extensions, matching `-march=rv32i_zicsr_zifencei`, so `core::sync::atomic` has no read-modify-write operations (`fetch_add`, `compare_exchange`, ...) and multiplication and division are done in software
