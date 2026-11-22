//
// Created by dshatalin on 10/25/26.
//

#include "args.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <print>
#include <string>
#include <thread>
#include <vector>

#define OPTIONAL_FLAGS                                                         \
  OPTIONAL_STRING_FLAG(format, "format", "byte", "set compiled output format") \
  OPTIONAL_STRING_FLAG(output, "output", "",                                   \
                       "set compiled output destionation")                     \
  OPTIONAL_STRING_FLAG(threads, "threads", "1",                                \
                       "set thread count for worker pool")
#define BOOLEAN_FLAGS                                                          \
  BOOLEAN_FLAG(version, "version", "show version information")                 \
  BOOLEAN_FLAG(help, "help", "show this message")
#define SHORT_FLAGS                                                            \
  SHORT_FLAG(recursive, 'r', "compile directory's contents recursively")       \
  SHORT_FLAG(verbose, 'v', "verbose output")                                   \
  SHORT_FLAG(quiet, 'q', "quiet output")                                       \
  SHORT_FLAG(help, 'h', "show this message")
#define POSITIONAL_REST REST_STRING_ARG(files, 1, 16, "input files")

#include "args_parser/args_parser.h"

#include "config.hpp"

auto hack_asm::parse_args(const int argc, const char *argv[], args &out)
    -> args_result {
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

  std::uint16_t threads = 0;
  if (std::strcmp(args.threads_optional_arg, "auto") == 0) {
    threads = std::thread::hardware_concurrency();
    if (threads == 0) {
      threads = 1;
    }
  } else {
    char *end = nullptr;
    auto parsed = std::strtoul(args.threads_optional_arg, &end, 10);
    if (*end != '\0') {
      std::println(stderr, "Error: invalid threads argument value '{}'",
                   args.threads_optional_arg);
      return args_result::exit_error;
    }
    threads = static_cast<uint16_t>(parsed);
  }

  std::vector<std::string> files(args.files_rest_arg.count);
  for (auto p : args.files_rest_arg.items) {
    files.emplace_back(p);
  }

  out = {
      .input = std::move(files),
      .format = std::string{args.format_optional_arg},
      .output = std::string{args.output_optional_arg},
      .threads = threads,
      .recursive = args.recursive_short_arg,
      .verbose = args.verbose_short_arg,
      .quiet = args.quiet_short_arg,
  };
  return args_result::run;
}
