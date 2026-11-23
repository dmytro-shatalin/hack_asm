add_library(${OUTPUT_LIB} STATIC
    src/encoder.cpp
    src/writer.cpp
)

target_sources(${OUTPUT_LIB} PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/encoder.hpp include/output_format.hpp
)

target_link_libraries(${OUTPUT_LIB}
    PUBLIC
        ${PARSER_LIB}
    PRIVATE
        ${HACK_ASM_WARNINGS}
)
