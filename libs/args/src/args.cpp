//
// Created by dshatalin on 10/25/26.
//

#include <cstdio>
#include "args.hpp"

#define POSITIONAL_ARGS \
    POSITIONAL_STRING_ARG(path, 0, "source files path")
#define OPTIONAL_FLAGS \
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

auto hack_asm::parse_args(const int argc, const char *argv[], args &out) -> args_result {
    args_t args;
    if (!::parse_args(argc, argv, &args)) {
        return args_result::exit_error;
    }

    if (args.help_bool_arg || args.help_short_arg) {
        print_help(argv[0]);
        return args_result::exit_ok;
    }
    if (args.version_bool_arg) {
        std::printf("%s version: %s\n", HACK_ASM_PROJECT_NAME, HACK_ASM_VERSION);
        return args_result::exit_ok;
    }
    if (!validate_args(&args)) {
        return args_result::exit_error;
    }

    out = {
        .path = std::string{args.path_pos_arg},
        .format = std::string{args.format_optional_arg},
        .output = std::string{args.output_optional_arg},
        .recursive = args.recursive_short_arg,
        .verbose = args.verbose_short_arg,
        .quiet = args.quiet_short_arg,
    };
    return args_result::run;
}
