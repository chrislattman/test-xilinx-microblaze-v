//! MicroBlaze-V application: no_std Rust port of `hello_world/src/helloworld.c`.
//!
//! Blinks LED0 on the AXI GPIO and prints over the PS UART0 every second, and
//! kicks the AXI Timebase Watchdog every two seconds.
//!
//! The PS7 UART is already configured to 115200 baud by ps7_init/the FSBL, and
//! `init_platform()`/`cleanup_platform()` from the C version's platform.c do
//! nothing on the MicroBlaze-V (no caches), so they have no equivalent here.
#![no_std]
#![no_main]

use core::convert::Infallible;
use core::ffi::{CStr, c_int};

use xil_mbv_0_sys::*;

/// axi_gpio_0 in the block design: channel 1 is leds_4bits
const LED_CHANNEL: u32 = 1;

type Result<T = ()> = core::result::Result<T, ()>;

/// `XST_SUCCESS` -> `Ok`, anything else -> `Err`
fn check(status: c_int) -> Result {
    if status == XST_SUCCESS { Ok(()) } else { Err(()) }
}

/// Prints over the PS7 UART0 (the Arty Z7's FT2232HQ USB-UART)
fn puts(s: &CStr) {
    unsafe { print(s.as_ptr()) }
}

fn run() -> Result<Infallible> {
    // Initialize AXI GPIO for LEDs
    let mut leds = XGpio::default();
    unsafe {
        check(XGpio_Initialize(&mut leds, XPAR_XGPIO_0_BASEADDR as UINTPTR))?;
        XGpio_SetDataDirection(&mut leds, LED_CHANNEL, 0x0);
    }

    // Initialize the AXI Timebase Watchdog. Its wdt_reset resets the PL (including
    // this core) through rst_ps7_0_100M if it isn't restarted in time
    let mut wdt = XWdtTb::default();
    let wdt_config = unsafe { XWdtTb_LookupConfig(XPAR_XWDTTB_0_BASEADDR as UINTPTR).as_mut() }.ok_or(())?;
    let base = wdt_config.BaseAddr;
    unsafe {
        check(XWdtTb_CfgInitialize(&mut wdt, wdt_config, base))?;
        XWdtTb_Start(&mut wdt);
    }

    loop {
        unsafe {
            // 0x1 is the bitmask for LED0; 0x2, 0x4, 0x8 for the other LEDs
            XGpio_DiscreteWrite(&mut leds, LED_CHANNEL, 0x1);
            sleep(1);
            puts(c"Hello\r\n");
            // 0x0 turns off all LEDs
            XGpio_DiscreteWrite(&mut leds, LED_CHANNEL, 0x0);
            sleep(1);
            puts(c"World!\r\n");
            XWdtTb_RestartWdt(&wdt);
        }
    }
}

/// Called by the BSP's startup code (boot.S/xil-crt0.S) after it sets up the
/// stack, .bss and the trap handler
#[unsafe(no_mangle)]
extern "C" fn main() -> c_int {
    match run() {
        Ok(never) => match never {},
        Err(()) => XST_FAILURE,
    }
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    puts(c"MicroBlaze-V panicked\r\n");
    loop {}
}
