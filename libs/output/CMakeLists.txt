add_library(${OUTPUT_LIB} INTERFACE)

target_sources(${OUTPUT_LIB} INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/encoder.hpp include/output_format.hpp
)
