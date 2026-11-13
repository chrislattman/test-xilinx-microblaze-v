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

Launch Vitis from a shell where `source <Vitis install>/settings64.sh` has run, so Vitis's own `riscv64-unknown-elf-gcc` comes first on `PATH`. A distro RISC-V GCC (e.g. Ubuntu's `gcc-riscv64-unknown-elf`) has no C library, and the BSP fails with `string.h: No such file or directory`. If that happens, delete `bsp/libsrc/build_configs/gen_bsp` so CMake searches for the compiler again.

## Running

Use the `hello_world` launch configuration in Vitis. It:

1. programs the FPGA with `sw/platform/export/platform/hw/design_1_wrapper.bit`
2. initializes the PS over JTAG with `sw/platform/export/platform/hw/ps7_init.tcl` (`ps7_init` and `ps7_post_init`): PLLs and clocks (including `FCLK_CLK0`, which clocks the MicroBlaze-V), MIO, UART0 at 115200 and the PS-PL level shifters. The FSBL isn't used
3. downloads and runs `hello_world.elf` on `microblaze_riscv_0`

Both files are the platform's copies, which Vitis regenerates when the platform is updated from a new XSA and rebuilt, so the launch always matches the current hardware.

Then open the UART, e.g. `picocom -b 115200 /dev/ttyUSB1`. `/dev/ttyUSB0` is the FTDI's JTAG channel.

## Notes

- The MicroBlaze-V BSP's `standalone_stdout` / `standalone_stdin` must be `ps7_uart_0`. If they name a UART that isn't enabled in the hardware, the BSP builds without a console (`STDOUT_BASEADDRESS` is undefined in `bspconfig.h`) and nothing prints
- The watchdog's timeout is set by the IP's WDT Interval parameter (2^N clock cycles). The first timeout sets the "expired" flag and the second asserts `wdt_reset`, which goes to `aux_reset_in` on `rst_ps7_0_100M`. Only the PL logic is reset, so the JTAG and UART connections stay up and the app restarts from `0x0` in BRAM
- The FSBL (`sw/platform/zynq_fsbl`) is only needed to boot from SD or QSPI. Vitis writes its `ps7_init.c` / `ps7_init.h` only when the FSBL is created, so they are symlinks to `sw/platform/hw/sdt/`, which every XSA update regenerates. If Vitis ever recreates the FSBL and replaces the links with files, recreate them with `ln -sf ../hw/sdt/ps7_init.c ps7_init.c` and `ln -sf ../hw/sdt/ps7_init.h ps7_init.h` in `sw/platform/zynq_fsbl`
- Generated BSP headers in `bsp/include/` are ignored by git, except `xparameters.h`. It's generated only when a domain is created, so a build can't regenerate it if it's deleted
