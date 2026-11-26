add_library(${ENGINE_LIB} STATIC
    src/engine.cpp
)

target_sources(${ENGINE_LIB} PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/engine.hpp
)

target_link_libraries(${ENGINE_LIB}
    PUBLIC
        ${ARGS_LIB}
    PRIVATE
        ${HACK_ASM_WARNINGS}
        ${OUTPUT_LIB}
        ${PARSER_LIB}
        threadlib::threadlib
)
