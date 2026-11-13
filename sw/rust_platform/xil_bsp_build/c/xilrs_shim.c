/* See xilrs_shim.h */
#include "xilrs_shim.h"
#include "xpseudo_asm.h"

void xilrs_XGpio_WriteReg(UINTPTR BaseAddress, u32 RegOffset, u32 Data) { XGpio_WriteReg(BaseAddress, RegOffset, Data); }
u32 xilrs_XGpio_ReadReg(UINTPTR BaseAddress, u32 RegOffset) { return XGpio_ReadReg(BaseAddress, RegOffset); }

void xilrs_XWdtTb_WriteReg(UINTPTR BaseAddress, u32 RegOffset, u32 RegisterValue) { XWdtTb_WriteReg(BaseAddress, RegOffset, RegisterValue); }
u32 xilrs_XWdtTb_ReadReg(UINTPTR BaseAddress, u32 RegOffset) { return XWdtTb_ReadReg(BaseAddress, RegOffset); }

void xilrs_fence(void) { fence(); }
void xilrs_fence_i(void) { fence_i(); }
