/*
 * Real functions for BSP APIs that are function-like macros, which bindgen
 * cannot see. (static inline functions such as Xil_Out32() are handled
 * automatically by bindgen's wrap_static_fns instead.)
 *
 * Every function here is named xilrs_<Name>. The build helper strips the
 * prefix, so Rust code calls them by their original names, e.g.
 * XGpio_WriteReg(), and bindgen links them to the prefixed symbol through
 * #[link_name].
 */
#ifndef XILRS_SHIM_H
#define XILRS_SHIM_H

#include "xil_types.h"
#include "xgpio_l.h"
#include "xwdttb_hw.h"

/* xgpio_l.h */
void xilrs_XGpio_WriteReg(UINTPTR BaseAddress, u32 RegOffset, u32 Data);
u32 xilrs_XGpio_ReadReg(UINTPTR BaseAddress, u32 RegOffset);

/* xwdttb_hw.h */
void xilrs_XWdtTb_WriteReg(UINTPTR BaseAddress, u32 RegOffset, u32 RegisterValue);
u32 xilrs_XWdtTb_ReadReg(UINTPTR BaseAddress, u32 RegOffset);

/* xpseudo_asm_gcc.h barriers (fence / fence.i, same as the C macros) */
void xilrs_fence(void);
void xilrs_fence_i(void);

#endif
