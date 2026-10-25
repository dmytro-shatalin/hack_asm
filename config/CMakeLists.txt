configure_file(
        "config.hpp.in"
        "${PROJECT_BINARY_DIR}/generated/include/config.hpp"
        @ONLY
        ESCAPE_QUOTES
)

add_library(${HACK_ASM_CONFIG} INTERFACE)
target_include_directories(${HACK_ASM_CONFIG} INTERFACE
        "${HACK_ASM_GENERATED_DIR}"
)