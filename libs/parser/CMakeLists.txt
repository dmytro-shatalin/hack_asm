add_library(${PARSER_LIB} STATIC
        src/parser.cpp
)

target_include_directories(${PARSER_LIB} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
