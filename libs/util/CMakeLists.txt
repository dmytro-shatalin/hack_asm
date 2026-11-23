add_library(${UTIL_LIB} STATIC
    src/util.cpp
)

target_sources(${UTIL_LIB} PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/util.hpp
)
