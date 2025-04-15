# Melhorias do Suporte ARM64 para Box64

Este documento descreve as melhorias implementadas para aprimorar o suporte a processadores ARM64 no emulador Box64, com foco especial em otimizações para jogos e aplicações que exigem alto desempenho.

## Visão Geral das Melhorias

As melhorias implementadas concentram-se em quatro áreas principais:

1. **Suporte a extensões ARM64 avançadas**: SVE, SVE2, BF16, I8MM e SME
2. **Otimizações específicas para plataformas**: Snapdragon, Exynos, Apple Silicon e Ampere Altra
3. **Otimizações para jogos**: Melhorias de SIMD, cálculos de física e uso de FMA
4. **API unificada**: Sistema modular para detecção e configuração de recursos ARM64

## Detalhes das Implementações

### 1. Novas Extensões ARM64 Suportadas

Foram adicionadas as seguintes extensões ARM64:

- **SVE (Scalable Vector Extension)**: Suporte a operações vetoriais de tamanho escalável
- **SVE2**: Segunda geração da extensão SVE
- **BF16 (Brain Float 16)**: Formato de ponto flutuante de 16 bits para IA/ML
- **I8MM (Int8 Matrix Multiplication)**: Multiplicação de matrizes de 8 bits para IA
- **SME (Scalable Matrix Extension)**: Operações matriciais escaláveis

Cada uma dessas extensões pode ser ativada individualmente via variáveis de ambiente:

```bash
# Exemplo: ativar SVE
export BOX64_ARM64_SVE=1
# Definir o tamanho do vetor SVE
export BOX64_ARM64_SVE_VL=256
```

### 2. Detecção Automática de Plataformas

O código agora detecta automaticamente a plataforma ARM64 e aplica otimizações específicas:

- **Qualcomm Snapdragon**: Otimizado para jogos e aplicações móveis
- **Samsung Exynos**: Otimizações para chipsets Exynos
- **Apple Silicon**: Otimizações para M1/M2/M3
- **Ampere Altra**: Otimizações para servidores ARM64

Além disso, há detecção específica para núcleos:
- **Cortex-A76**: Otimizações para este tipo de núcleo comum em dispositivos móveis
- **Cortex-X1/X2**: Otimizações para núcleos de alto desempenho

### 3. Modo de Jogo Otimizado

Foi implementado um modo de jogo que ativa automaticamente otimizações para:

- **SIMD Aprimorado**: Melhoria no processamento de shaders
- **Cálculos de Física**: Maior precisão e velocidade
- **FMA NEON**: Uso intensivo de operações Fused Multiply-Add

O modo de jogo pode ser ativado via variável de ambiente:
```bash
export BOX64_ARM64_GAME_MODE=1
```

### 4. API Unificada para Recursos ARM64

Foi criada uma API centralizada para gerenciar todos os recursos ARM64:

- **arm64_features.h**: Declarações de todas as flags e funções
- **arm64_features.c**: Implementação das funções de detecção e configuração

A API inclui:
- Detecção automática de recursos
- Sistema de callbacks para extensões
- Funções de consulta para verificar suporte
- Aplicação automática de otimizações baseadas em plataforma

### 5. Melhorias de Compilação

Adições ao sistema de compilação (CMake):

- Detecção automática de extensões no compilador
- Flags específicas para ARM64
- Configurações otimizadas para diferentes chipsets
- Opções para ativar/desativar recursos específicos

## Impacto das Melhorias

As melhorias implementadas têm os seguintes benefícios:

1. **Melhor desempenho em jogos**: As otimizações específicas para jogos resultam em renderização mais rápida e cálculos de física mais precisos.

2. **Suporte a hardware moderno**: O suporte para extensões como SVE e BF16 permite aproveitar ao máximo os recursos de chips ARM64 recentes.

3. **Adaptação automática**: A detecção e otimização automática para diferentes plataformas (Snapdragon, Apple Silicon, etc.) garante o melhor desempenho possível sem configuração manual.

4. **Flexibilidade**: O sistema modular permite ativar/desativar recursos individualmente, adaptando-se a diferentes casos de uso.

## Uso

Para aproveitar ao máximo estas melhorias:

1. **Habilitação geral**:
   ```bash
   # Ativar modo de jogo (recomendado para jogos)
   export BOX64_ARM64_GAME_MODE=1
   ```

2. **Ajustes finos**:
   ```bash
   # Ativar recursos específicos
   export BOX64_ARM64_SVE=1
   export BOX64_ARM64_SVE_VL=256  # Tamanho do vetor SVE em bits
   export BOX64_ARM64_ENHANCED_SIMD=1
   export BOX64_ARM64_PHYSICS_OPT=1
   export BOX64_ARM64_NEON_FMA=1
   ```

3. **Compilação otimizada**:
   ```bash
   # Para compilar com otimizações para Cortex-A76
   cmake -DOPTIMIZE_FOR_CORTEX_A76=ON ..
   ```

## Trabalhos Futuros

Para desenvolvimento futuro, estão previstos:

1. Implementação completa das instruções SVE para o dynarec
2. Suporte a mais plataformas ARM64 específicas
3. Otimizações específicas para aplicações de IA usando BF16 e I8MM
4. Perfis de otimização automáticos baseados em tipos de aplicações
5. Suporte a mais extensões ARM64 (como MTE, BTI, etc.)

---

## Referências

- [ARM Architecture Reference Manual for A-profile architecture](https://developer.arm.com/documentation/ddi0487/latest)
- [Arm SVE](https://developer.arm.com/architectures/instruction-sets/sve)
- [Arm Neon](https://developer.arm.com/architectures/instruction-sets/simd-isas/neon)
- [Apple Silicon](https://developer.apple.com/documentation/apple-silicon) 