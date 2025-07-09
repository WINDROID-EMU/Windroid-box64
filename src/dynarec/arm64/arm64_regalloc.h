#ifndef ARM64_REGALLOC_H
#define ARM64_REGALLOC_H

#include "dynarec_arm64_private.h"

int get_arm64_reg_for_x86(dynarec_arm_t* dyn, int x86_reg, int ninst, int for_write);
void flush_arm64_regs(dynarec_arm_t* dyn, int ninst);
void init_arm64_reg_state(dynarec_arm_t* dyn);

#endif // ARM64_REGALLOC_H 