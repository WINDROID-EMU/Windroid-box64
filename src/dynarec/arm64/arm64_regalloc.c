#include "arm64_regalloc.h"
#include <string.h>

// Inicializa todos os registradores ARM64 como livres
void init_arm64_reg_state(dynarec_arm_t* dyn) {
    for (int i = 0; i < ARM64_REG_COUNT; ++i) {
        dyn->reg_state[i].x86_reg = -1;
        dyn->reg_state[i].dirty = 0;
    }
}

// Busca ou aloca um registrador ARM64 para um registrador x86
int get_arm64_reg_for_x86(dynarec_arm_t* dyn, int x86_reg, int ninst, int for_write) {
    // 1. Já está mapeado?
    for (int i = 0; i < ARM64_REG_COUNT; ++i) {
        if (dyn->reg_state[i].x86_reg == x86_reg) {
            return i;
        }
    }
    // 2. Tem algum livre?
    for (int i = 0; i < ARM64_REG_COUNT; ++i) {
        if (dyn->reg_state[i].x86_reg == -1) {
            dyn->reg_state[i].x86_reg = x86_reg;
            dyn->reg_state[i].dirty = for_write;
            // Aqui, carregue o valor da memória x86 para o reg ARM64 se necessário
            // (pode ser feito por macro/função de load)
            return i;
        }
    }
    // 3. Precisa fazer spill (escolha o primeiro para simplicidade)
    int victim = 0;
    if (dyn->reg_state[victim].dirty) {
        // Salve o valor de volta para a memória x86
        // (pode ser feito por macro/função de store)
    }
    dyn->reg_state[victim].x86_reg = x86_reg;
    dyn->reg_state[victim].dirty = for_write;
    // Carregue o valor da memória x86 se necessário
    return victim;
}

// Salva todos os registradores ARM64 dirty na memória x86
void flush_arm64_regs(dynarec_arm_t* dyn, int ninst) {
    for (int i = 0; i < ARM64_REG_COUNT; ++i) {
        if (dyn->reg_state[i].x86_reg != -1 && dyn->reg_state[i].dirty) {
            // Salve o valor de volta para a memória x86
            // (pode ser feito por macro/função de store)
            dyn->reg_state[i].dirty = 0;
        }
    }
} 