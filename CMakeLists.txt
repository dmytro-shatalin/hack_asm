cmake_minimum_required(VERSION 4.2)

# Includes

include(FetchContent)

# Project Config

project(hack_asm VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Variables

set(HACK_ASM_TARGET "hack_asm")
set(HACK_ASM_TESTS "hack_asm_tests")
set(HACK_ASM_CONFIG "hack_asm_config")
set(HACK_ASM_WARNINGS "hack_asm_warnings")

set(HACK_ASM_WARNING_OPTIONS "-Wall" "-Wextra" "-Wpedantic")

set(PARSER_LIB "parser_lib")
set(ARGS_PARSER_DEP "args_parser_dep")
set(GOOGLETEST_DEP "googletest_dep")

set(HACK_ASM_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")

# Options

option(COMPILE_EXECUTABLE "Whether to compile the executable" ON)
option(COMPILE_TESTS "Whether to compile the tests" ON)
option(ENABLE_WARNINGS "Whether to enable warnings for executable" ON)
option(WARNINGS_AS_ERRORS "Whether to treat warnings as errors" OFF)

# Warnings

add_library(${HACK_ASM_WARNINGS} INTERFACE)
if (WARNINGS_AS_ERRORS)
    list(APPEND HACK_ASM_WARNING_OPTIONS "-Werror")
endif ()
target_compile_options(${HACK_ASM_WARNINGS} INTERFACE ${HACK_ASM_WARNING_OPTIONS})

if (COMPILE_EXECUTABLE)
    add_executable(${HACK_ASM_TARGET})
    message(STATUS "Executable will be compiled")
    if (ENABLE_WARNINGS)
        target_link_libraries(${HACK_ASM_TARGET} PRIVATE
            hack_asm_warnings
        )
    endif ()
else ()
    message(STATUS "Executable won't be compiled")
endif ()

if (COMPILE_TESTS)
    enable_testing()
    add_executable(${HACK_ASM_TESTS})
    message(STATUS "Tests will be compiled")
else ()
    message(STATUS "Tests won't be compiled")
endif ()

# Dependencies

add_subdirectory(config)
add_subdirectory(libs)

# External Dependencies

if (COMPILE_EXECUTABLE)
    FetchContent_Declare(
            ${ARGS_PARSER_DEP}
            GIT_REPOSITORY https://github.com/dmytro-shatalin/args_parser
            GIT_TAG v1.0.3
    )
    FetchContent_MakeAvailable(${ARGS_PARSER_DEP})
endif ()

if (COMPILE_TESTS)
    FetchContent_Declare(
            ${GOOGLETEST_DEP}
            GIT_REPOSITORY https://github.com/google/googletest
            GIT_TAG v1.18.0
    )
    FetchContent_MakeAvailable(${GOOGLETEST_DEP})
endif ()

# Targets

if (COMPILE_EXECUTABLE)
    target_sources(${HACK_ASM_TARGET} PRIVATE
            src/main.cpp
    )
    target_link_libraries(${HACK_ASM_TARGET} PRIVATE
            ${HACK_ASM_CONFIG}
            ${PARSER_LIB}
            args_parser::args_parser
    )
endif ()

if (COMPILE_TESTS)
    target_link_libraries(${HACK_ASM_TESTS} PRIVATE
            ${HACK_ASM_WARNINGS}
            ${PARSER_LIB}
            GTest::gtest_main
    )
    include(GoogleTest)
    gtest_discover_tests(${HACK_ASM_TESTS} PROPERTIES
            TIMEOUT 5
    )
endif ()
