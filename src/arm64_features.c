#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/arm64_features.h"
#include "include/debug.h"

#ifdef ARM64

// Callbacks para recursos
typedef struct {
    int feature_id;
    void (*callback)(void);
} arm64_feature_callback_t;

#define MAX_CALLBACKS 20
static arm64_feature_callback_t feature_callbacks[MAX_CALLBACKS];
static int num_callbacks = 0;

// Plataforma detectada
static int detected_platform = ARM64_PLATFORM_GENERIC;
static const char* platform_names[] = {
    "Genérico",
    "Qualcomm Snapdragon",
    "Samsung Exynos",
    "Apple Silicon",
    "Ampere Altra"
};

// Inicializa e detecta recursos ARM64 disponíveis
void arm64_init_features() {
    // Já configurado em wowbox64.c, aqui apenas complementamos
    
    // Determinar plataforma
    if (arm64_snapdragon) {
        detected_platform = ARM64_PLATFORM_SNAPDRAGON;
    } else if (arm64_exynos) {
        detected_platform = ARM64_PLATFORM_EXYNOS;
    } else if (arm64_apple) {
        detected_platform = ARM64_PLATFORM_APPLE;
    } else if (arm64_ampere) {
        detected_platform = ARM64_PLATFORM_AMPERE;
    }
    
    // Imprimir informações de recursos ARM64
    printf_log(LOG_INFO, "ARM64: Plataforma detectada: %s\n", arm64_get_platform_name());
    printf_log(LOG_INFO, "ARM64: Recursos disponíveis:\n");
    printf_log(LOG_INFO, "  - ASIMD (Advanced SIMD): %s\n", arm64_asimd ? "Sim" : "Não");
    printf_log(LOG_INFO, "  - AES: %s\n", arm64_aes ? "Sim" : "Não");
    printf_log(LOG_INFO, "  - PMULL: %s\n", arm64_pmull ? "Sim" : "Não");
    printf_log(LOG_INFO, "  - CRC32: %s\n", arm64_crc32 ? "Sim" : "Não");
    printf_log(LOG_INFO, "  - SHA1: %s\n", arm64_sha1 ? "Sim" : "Não");
    printf_log(LOG_INFO, "  - SHA2: %s\n", arm64_sha2 ? "Sim" : "Não");
    printf_log(LOG_INFO, "  - SVE: %s\n", arm64_sve ? "Sim" : "Não");
    if (arm64_sve) {
        printf_log(LOG_INFO, "    - Tamanho do vetor SVE: %d bits\n", arm64_sve_vl);
    }
    printf_log(LOG_INFO, "  - SVE2: %s\n", arm64_sve2 ? "Sim" : "Não");
    printf_log(LOG_INFO, "  - BF16: %s\n", arm64_bf16 ? "Sim" : "Não");
    printf_log(LOG_INFO, "  - I8MM: %s\n", arm64_i8mm ? "Sim" : "Não");
    printf_log(LOG_INFO, "  - SME: %s\n", arm64_sme ? "Sim" : "Não");
    
    // Inicializar callbacks
    memset(feature_callbacks, 0, sizeof(feature_callbacks));
    num_callbacks = 0;
}

// Habilita otimizações para jogos
void arm64_set_game_mode(int enable) {
    arm64_game_mode = enable;
    if (enable) {
        arm64_enhanced_simd = 1;
        arm64_physics_opt = 1;
        arm64_neon_fma = 1;
        
        printf_log(LOG_INFO, "ARM64: Modo de jogo ativado com todas as otimizações\n");
    } else {
        arm64_enhanced_simd = 0;
        arm64_physics_opt = 0;
        arm64_neon_fma = 0;
        
        printf_log(LOG_INFO, "ARM64: Modo de jogo desativado\n");
    }
}

// Verifica se a plataforma atual suporta uma extensão específica
int arm64_has_feature(int feature_id) {
    switch (feature_id) {
        case ARM64_FEATURE_SVE:
            return arm64_sve;
        case ARM64_FEATURE_SVE2:
            return arm64_sve2;
        case ARM64_FEATURE_BF16:
            return arm64_bf16;
        case ARM64_FEATURE_I8MM:
            return arm64_i8mm;
        case ARM64_FEATURE_SME:
            return arm64_sme;
        case ARM64_FEATURE_ASIMD:
            return arm64_asimd;
        case ARM64_FEATURE_ATOMICS:
            return arm64_atomics;
        case ARM64_FEATURE_AES:
            return arm64_aes;
        case ARM64_FEATURE_SHA1:
            return arm64_sha1;
        case ARM64_FEATURE_SHA2:
            return arm64_sha2;
        case ARM64_FEATURE_CRC32:
            return arm64_crc32;
        default:
            return 0;
    }
}

// Registra uma função de callback para ser chamada quando uma extensão específica for usada
int arm64_register_feature_callback(int feature_id, void (*callback)(void)) {
    if (num_callbacks >= MAX_CALLBACKS) {
        return -1; // Sem espaço para mais callbacks
    }
    
    // Verificar se o recurso existe
    if (!arm64_has_feature(feature_id)) {
        return -1; // Recurso não disponível
    }
    
    // Adicionar callback
    feature_callbacks[num_callbacks].feature_id = feature_id;
    feature_callbacks[num_callbacks].callback = callback;
    num_callbacks++;
    
    return 0;
}

// Retorna o ID da plataforma ARM64 atual
int arm64_get_platform() {
    return detected_platform;
}

// Obtém o nome da plataforma ARM64 atual
const char* arm64_get_platform_name() {
    if (detected_platform < 0 || detected_platform >= sizeof(platform_names)/sizeof(platform_names[0])) {
        return "Desconhecido";
    }
    return platform_names[detected_platform];
}

// Obtém o tamanho do vetor SVE em bits
int arm64_get_sve_vector_length() {
    if (!arm64_sve) {
        return 0;
    }
    return arm64_sve_vl;
}

// Aplica otimizações específicas para a plataforma atual
int arm64_apply_platform_optimizations() {
    // Aplicar otimizações baseadas na plataforma detectada
    switch (detected_platform) {
        case ARM64_PLATFORM_SNAPDRAGON:
            // Otimizações para Snapdragon
            arm64_enhanced_simd = 1;
            arm64_neon_fma = 1;
            // Verificar se é um modelo específico
            if (arm64_a76) {
                // Otimizações para Cortex-A76
                printf_log(LOG_INFO, "ARM64: Aplicando otimizações para Cortex-A76\n");
            } else if (arm64_x1) {
                // Otimizações para Cortex-X1
                printf_log(LOG_INFO, "ARM64: Aplicando otimizações para Cortex-X1\n");
            }
            break;
            
        case ARM64_PLATFORM_EXYNOS:
            // Otimizações para Exynos
            arm64_enhanced_simd = 1;
            arm64_neon_fma = 1;
            break;
            
        case ARM64_PLATFORM_APPLE:
            // Otimizações específicas para Apple Silicon
            arm64_enhanced_simd = 1;
            arm64_neon_fma = 1;
            // Apple Silicon geralmente tem melhor desempenho com certas otimizações
            printf_log(LOG_INFO, "ARM64: Aplicando otimizações para Apple Silicon\n");
            break;
            
        case ARM64_PLATFORM_AMPERE:
            // Otimizações para servidores Ampere Altra
            arm64_enhanced_simd = 1;
            arm64_neon_fma = 1;
            // Otimizações específicas para servidores
            printf_log(LOG_INFO, "ARM64: Aplicando otimizações para Ampere Altra\n");
            break;
            
        default:
            // Otimizações genéricas
            arm64_enhanced_simd = 1;
            break;
    }
    
    // Executar callbacks para recursos disponíveis
    for (int i = 0; i < num_callbacks; i++) {
        int feature_id = feature_callbacks[i].feature_id;
        if (arm64_has_feature(feature_id)) {
            if (feature_callbacks[i].callback) {
                feature_callbacks[i].callback();
            }
        }
    }
    
    return 0;
}

#endif // ARM64 