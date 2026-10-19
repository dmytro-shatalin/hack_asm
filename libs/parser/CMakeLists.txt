add_library(${PARSER_LIB} STATIC
        src/parser.cpp
        src/scanner.cpp
)

target_sources(${PARSER_LIB} PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES include/parser.hpp
)
