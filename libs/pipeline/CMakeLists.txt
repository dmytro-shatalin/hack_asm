add_library(${PIPELINE_LIB} STATIC
    src/pipeline.cpp
)

target_sources(${PIPELINE_LIB} PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/pipeline.hpp
)

target_link_libraries(${PIPELINE_LIB}
    PUBLIC
        ${PARSER_LIB}
        ${ARGS_LIB}
    PRIVATE
        ${HACK_ASM_WARNINGS}
        ${UTIL_LIB}
)
