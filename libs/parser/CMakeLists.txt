add_library(${PARSER_LIB} STATIC
        src/parser.cpp
        src/scanner.cpp
)

target_sources(${PARSER_LIB} PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES include/parser.hpp
)

target_link_libraries(${PARSER_LIB} PRIVATE
        ${HACK_ASM_WARNINGS}
)

if (COMPILE_TESTS)
    add_subdirectory(tests)
endif ()
