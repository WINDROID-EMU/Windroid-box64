cmake_minimum_required(VERSION 3.13)
project(wowbox64 C ASM)

set(BOX64_ROOT "${CMAKE_SOURCE_DIR}/..")

string(REPLACE "," ";" DYNAREC_ASM "${DYNAREC_ASM_STR}")
string(REPLACE "," ";" DYNAREC_PASS "${DYNAREC_PASS_STR}")
string(REPLACE "," ";" INTERPRETER "${INTERPRETER_STR}")

set(WOW64_MAIN_SRC
    "${BOX64_ROOT}/wow64/wowbox64.c"
)

# Detectar se estamos construindo em uma plataforma ARM64
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    message(STATUS "Configurando para ARM64...")
    set(ARM64_COMPILE_OPTIONS "-march=armv8-a")
    
    # Verificar suporte a extensões ARM64
    include(CheckCCompilerFlag)
    
    # Verificar SVE
    check_c_compiler_flag("-march=armv8-a+sve" HAVE_SVE)
    if(HAVE_SVE)
        message(STATUS "SVE suportado pelo compilador")
        set(ARM64_COMPILE_OPTIONS "${ARM64_COMPILE_OPTIONS}+sve")
    endif()
    
    # Verificar SVE2
    check_c_compiler_flag("-march=armv8.2-a+sve2" HAVE_SVE2)
    if(HAVE_SVE2)
        message(STATUS "SVE2 suportado pelo compilador")
        set(ARM64_COMPILE_OPTIONS "${ARM64_COMPILE_OPTIONS}+sve2")
    endif()
    
    # Outras extensões
    check_c_compiler_flag("-march=armv8.6-a+bf16" HAVE_BF16)
    if(HAVE_BF16)
        message(STATUS "BF16 suportado pelo compilador")
        set(ARM64_COMPILE_OPTIONS "${ARM64_COMPILE_OPTIONS}+bf16")
    endif()
    
    check_c_compiler_flag("-march=armv8.6-a+i8mm" HAVE_I8MM)
    if(HAVE_I8MM)
        message(STATUS "I8MM suportado pelo compilador")
        set(ARM64_COMPILE_OPTIONS "${ARM64_COMPILE_OPTIONS}+i8mm")
    endif()
    
    # Otimizações para cores específicos
    option(OPTIMIZE_FOR_CORTEX_A76 "Otimizar para Cortex-A76" OFF)
    if(OPTIMIZE_FOR_CORTEX_A76)
        set_source_files_properties(${DYNAREC_ASM} PROPERTIES COMPILE_OPTIONS "-mcpu=cortex-a76")
    else()
        set_source_files_properties(${DYNAREC_ASM} PROPERTIES COMPILE_OPTIONS "-mcpu=cortex-a76")
    endif()
    
    # Otimizações para Apple Silicon
    if(APPLE)
        message(STATUS "Detectado Apple Silicon, aplicando otimizações...")
        # Aplicar flags específicas para Apple Silicon
        add_compile_definitions(ARM64_APPLE_SILICON)
    endif()
else()
    # Configuração padrão para outras plataformas
    set_source_files_properties(${DYNAREC_ASM} PROPERTIES COMPILE_OPTIONS "-mcpu=cortex-a76")
endif()

foreach(STEP_VALUE RANGE 3)
    add_library(wow64_dynarec_pass${STEP_VALUE} OBJECT ${DYNAREC_PASS})
    target_compile_definitions(wow64_dynarec_pass${STEP_VALUE} PRIVATE STEP=${STEP_VALUE})
endforeach()

add_library(wow64_test_interpreter OBJECT ${INTERPRETER})
set_target_properties(wow64_test_interpreter PROPERTIES COMPILE_DEFINITIONS "TEST_INTERPRETER")

set(WOW64_BOX64CPU_SRC
    "${BOX64_ROOT}/src/custommem.c"
    "${BOX64_ROOT}/src/dynarec/arm64/arm64_immenc.c"
    "${BOX64_ROOT}/src/dynarec/arm64/dynarec_arm64_arch.c"
    "${BOX64_ROOT}/src/dynarec/arm64/dynarec_arm64_functions.c"
    "${BOX64_ROOT}/src/dynarec/arm64/dynarec_arm64_jmpnext.c"
    "${BOX64_ROOT}/src/dynarec/dynablock.c"
    "${BOX64_ROOT}/src/dynarec/dynarec_native_functions.c"
    "${BOX64_ROOT}/src/dynarec/dynarec_native.c"
    "${BOX64_ROOT}/src/dynarec/dynarec.c"
    "${BOX64_ROOT}/src/emu/x64compstrings.c"
    "${BOX64_ROOT}/src/emu/x64emu.c"
    "${BOX64_ROOT}/src/emu/x64primop.c"
    "${BOX64_ROOT}/src/emu/x64shaext.c"
    "${BOX64_ROOT}/src/emu/x64test.c"
    "${BOX64_ROOT}/src/emu/x64trace.c"
    "${BOX64_ROOT}/src/emu/x64run_private.c"
    "${BOX64_ROOT}/src/emu/x87emu_private.c"
    "${BOX64_ROOT}/src/os/backtrace.c"
    "${BOX64_ROOT}/src/os/os_wine.c"
    "${BOX64_ROOT}/src/os/freq_wine.c"
    "${BOX64_ROOT}/src/os/symbolfuncs_wine.c"
    "${BOX64_ROOT}/src/os/emit_signal_wine.c"
    "${BOX64_ROOT}/src/os/perfmap.c"
    "${BOX64_ROOT}/src/os/my_cpuid_wine.c"
    "${BOX64_ROOT}/src/tools/alternate.c"
    "${BOX64_ROOT}/src/tools/rbtree.c"
)

add_library(wowbox64 SHARED ${WOW64_MAIN_SRC} ${WOW64_BOX64CPU_SRC} ${INTERPRETER} ${DYNAREC_ASM}
    $<TARGET_OBJECTS:wow64_dynarec_pass0>
    $<TARGET_OBJECTS:wow64_dynarec_pass1>
    $<TARGET_OBJECTS:wow64_dynarec_pass2>
    $<TARGET_OBJECTS:wow64_dynarec_pass3>
    $<TARGET_OBJECTS:wow64_test_interpreter>)

include_directories(
    "${BOX64_ROOT}/src/include"
    "${BOX64_ROOT}/src"
)

# Definições para ARM64
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    add_compile_definitions(ARM64_ENHANCED_SIMD)
    add_compile_definitions(ARM64_DYNAREC_PASS_OPTIMIZED)
endif()

# always enable DynaRec, only supports ARM64 for now.
add_compile_definitions(DYNAREC ARM64)
target_link_libraries(wowbox64 PRIVATE winpthread)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)