#include <iostream>

#include "args.hpp"

int main(const int argc, const char *argv[]) {
    hack_asm::args args{};
    if (const auto result = hack_asm::parse_args(argc, argv, args); result != hack_asm::args_result::run) {
        return result == hack_asm::args_result::exit_ok ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}
