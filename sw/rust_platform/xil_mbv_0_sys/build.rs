fn main() {
    xil_bsp_build::Bsp {
        default_dir: "../../platform/microblaze_riscv_0/standalone_microblaze_riscv_0/bsp",
        dir_env: "XIL_MBV_0_BSP_DIR",
        // Must match TOOLCHAIN_C_FLAGS in the BSP's microblaze_riscv_toolchain.cmake
        defines: &[],
    }
    .build();
}
