#include <iostream>

#define POSITIONAL_ARGS \
    POSITIONAL_STRING_ARG(path, 0, "source files path")
#define OPTIONAL_ARGS \
    OPTIONAL_STRING_FLAG(format, "format", "byte", "set compiled output format") \
    OPTIONAL_STRING_FLAG(output, "output", "", "set compiled output destionation")
#define BOOLEAN_FLAGS \
    BOOLEAN_FLAG(version, "version", "show version information") \
    BOOLEAN_FLAG(help, "help", "show this message")
#define SHORT_FLAGS \
    SHORT_FLAG(recursive, 'r', "compile directory's contents recursively") \
    SHORT_FLAG(verbose, 'v', "verbose output") \
    SHORT_FLAG(quiet, 'q', "quiet output") \
    SHORT_FLAG(help, 'h', "show this message")

#include "args_parser/args_parser.h"

#include "config.hpp"

namespace {
    bool should_print_help(const args_t *args, const char *program_name) {
        if (args->help_bool_arg || args->help_short_arg) {
            print_help(program_name);
            return true;
        }
        return false;
    }

    bool should_print_version(const args_t *args) {
        if (args->version_bool_arg) {
            std::printf("%s version: %s\n", HACK_ASM_PROJECT_NAME, HACK_ASM_VERSION);
            return true;
        }
        return false;
    }
}

int main(const int argc, const char *argv[]) {
    args_t args;
    if (!parse_args(argc, argv, &args)) {
        return 1;
    }
    if (should_print_help(&args, argv[0])) {
        return 0;
    }
    if (should_print_version(&args)) {
        return 0;
    }
    if (!validate_args(&args)) {
        return 1;
    }
    return 0;
}
