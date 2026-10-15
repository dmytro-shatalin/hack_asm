configure_file(
        "config.hpp.in"
        "${CMAKE_BINARY_DIR}/generated/include/config.hpp"
        @ONLY
        ESCAPE_QUOTES
)

add_library(hack_asm_config INTERFACE)
target_include_directories(hack_asm_config INTERFACE "${HACK_ASM_GENERATED_DIR}")