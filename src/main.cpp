//
// Created by dshatalin on 10/25/26.
//

#include "args.hpp"
#include "engine.hpp"

#include <cstdlib>

int main(const int argc, const char *argv[]) {
  hack_asm::args args{};
  if (const auto result = hack_asm::parse_args(argc, argv, args);
      result != hack_asm::args_result::run) {
    return result == hack_asm::args_result::exit_ok ? EXIT_SUCCESS
                                                    : EXIT_FAILURE;
  }

  return hack_asm::assembler::from_args(args).run() ? EXIT_SUCCESS
                                                    : EXIT_FAILURE;
}
