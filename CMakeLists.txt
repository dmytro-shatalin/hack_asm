cmake_minimum_required(VERSION 4.2)

# Includes
include(FetchContent)

# Project Config
project(hack_asm VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_C_STANDARD 23)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Variables
set(PARSER_LIB "parser_lib")

set(HACK_ASM_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")

option(COMPILE_EXECUTABLE "Whether to compile the executable" ON)

if (COMPILE_EXECUTABLE)
    message(STATUS "Executable will be compiled")
else ()
    message(STATUS "Executable won't be compiled")
endif ()

# Dependencies
add_subdirectory(config)
add_subdirectory(libs)

# External Dependencies
FetchContent_Declare(
        args_parser
        GIT_REPOSITORY https://github.com/dmytro-shatalin/args_parser
        GIT_TAG v1.0.3
)
FetchContent_MakeAvailable(args_parser)

# Targets
if (COMPILE_EXECUTABLE)
    add_executable(hack_asm
            src/main.cpp
    )
    target_link_libraries(hack_asm PRIVATE
            hack_asm_config
            ${PARSER_LIB}
            args_parser
            ${PARSER_LIB}
    )
endif ()
