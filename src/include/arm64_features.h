#ifndef __ARM64_FEATURES_H_
#define __ARM64_FEATURES_H_

#include <stdint.h>

#ifdef ARM64
// Recursos básicos ARM64
extern int arm64_asimd;        // Advanced SIMD (NEON)
extern int arm64_aes;          // Extensões AES
extern int arm64_pmull;        // Polynomial Multiply
extern int arm64_crc32;        // CRC32 Extension
extern int arm64_atomics;      // Atomic Operations
extern int arm64_sha1;         // SHA1 Extension
extern int arm64_sha2;         // SHA2 Extension
extern int arm64_uscat;        // Unaligned Atomic Scatter/Gather
extern int arm64_flagm;        // Flag Manipulation Instructions
extern int arm64_flagm2;       // Flag Manipulation v2
extern int arm64_frintts;      // Floating-point Round to Integer
extern int arm64_rndr;         // Random Number Generation

// Otimizações de jogos em ARM64
extern int arm64_game_mode;    // Otimizações para jogos
extern int arm64_enhanced_simd;// Otimizações SIMD aprimoradas
extern int arm64_physics_opt;  // Otimizações para cálculos de física
extern int arm64_neon_fma;     // Uso intensivo de FMA NEON

// Extensões ARM64 avançadas
extern int arm64_sve;          // Scalable Vector Extension
extern int arm64_sve2;         // SVE2 Extension
extern int arm64_bf16;         // Brain Float 16 support
extern int arm64_i8mm;         // Int8 Matrix Multiplication
extern int arm64_predres;      // Predicate Restrictions
extern int arm64_sme;          // Scalable Matrix Extension
extern int arm64_sve_vl;       // Tamanho do vetor SVE

// Plataformas ARM64 específicas
extern int arm64_snapdragon;   // Qualcomm Snapdragon
extern int arm64_exynos;       // Samsung Exynos
extern int arm64_apple;        // Apple Silicon
extern int arm64_ampere;       // Ampere Altra
extern int arm64_a76;          // Cortex-A76
extern int arm64_x1;           // Cortex-X1/X2

// Funções de detecção e configuração
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Inicializa e detecta recursos ARM64 disponíveis
 * Esta função deve ser chamada no início do programa
 */
void arm64_init_features();

/**
 * Habilita otimizações para jogos
 * @param enable Ativa (1) ou desativa (0) otimizações
 */
void arm64_set_game_mode(int enable);

/**
 * Verifica se a plataforma atual suporta uma extensão específica
 * @param feature_id ID da extensão a verificar
 * @return 1 se suportado, 0 caso contrário
 */
int arm64_has_feature(int feature_id);

/**
 * Registra uma função de callback para ser chamada quando uma extensão específica for usada
 * @param feature_id ID da extensão 
 * @param callback Função a ser chamada
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int arm64_register_feature_callback(int feature_id, void (*callback)(void));

// IDs de recursos para uso com arm64_has_feature
#define ARM64_FEATURE_SVE      0x01
#define ARM64_FEATURE_SVE2     0x02
#define ARM64_FEATURE_BF16     0x03
#define ARM64_FEATURE_I8MM     0x04
#define ARM64_FEATURE_SME      0x05
#define ARM64_FEATURE_ASIMD    0x06
#define ARM64_FEATURE_ATOMICS  0x07
#define ARM64_FEATURE_AES      0x08
#define ARM64_FEATURE_SHA1     0x09
#define ARM64_FEATURE_SHA2     0x0A
#define ARM64_FEATURE_CRC32    0x0B

// Plataformas
#define ARM64_PLATFORM_GENERIC    0x00
#define ARM64_PLATFORM_SNAPDRAGON 0x01
#define ARM64_PLATFORM_EXYNOS     0x02
#define ARM64_PLATFORM_APPLE      0x03
#define ARM64_PLATFORM_AMPERE     0x04

/**
 * Retorna o ID da plataforma ARM64 atual
 * @return ID da plataforma 
 */
int arm64_get_platform();

/**
 * Obtém o nome da plataforma ARM64 atual
 * @return String com o nome da plataforma
 */
const char* arm64_get_platform_name();

/**
 * Obtém o tamanho do vetor SVE em bits
 * @return Tamanho do vetor SVE em bits ou 0 se SVE não for suportado
 */
int arm64_get_sve_vector_length();

/**
 * Aplica otimizações específicas para a plataforma atual
 * Esta função configura automaticamente as melhores otimizações
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int arm64_apply_platform_optimizations();

#ifdef __cplusplus
}
#endif

#endif // ARM64
#endif // __ARM64_FEATURES_H_ 