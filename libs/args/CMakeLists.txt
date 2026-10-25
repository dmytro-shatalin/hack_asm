add_library(${ARGS_LIB} STATIC
        src/args.cpp)

target_sources(${ARGS_LIB} PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES include/args.hpp
)

target_link_libraries(${ARGS_LIB} PRIVATE
        ${HACK_ASM_WARNINGS}
        ${HACK_ASM_CONFIG}
        args_parser::args_parser
)
