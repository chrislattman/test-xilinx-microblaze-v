use std::env;
use std::path::PathBuf;

fn main() {
    // From xil-mbv-0-sys (`links = "xil_mbv_0"`): the BSP's Xilinx.spec swaps GCC's
    // startup files for the ones the BSP's boot code expects, like in Vitis
    let specs = env::var("DEP_XIL_MBV_0_SPECS").unwrap();
    println!("cargo:rustc-link-arg-bins=-specs={specs}");

    // This app's linker script (memory map), copied from the C application's
    // hello_world/src/lscript.ld
    let linker_script = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap()).join("linker.ld");
    println!("cargo:rerun-if-changed={}", linker_script.display());
    println!("cargo:rustc-link-arg-bins=-Wl,-T,{}", linker_script.display());
}
