add_library(${UTIL_LIB} INTERFACE)

target_sources(${UTIL_LIB} INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/util.hpp
)

target_link_libraries(${UTIL_LIB} INTERFACE
    ${ARGS_LIB}
)
