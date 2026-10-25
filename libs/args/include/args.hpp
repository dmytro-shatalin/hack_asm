//
// Created by dshatalin on 10/25/26.
//

#pragma once
#include <string>

namespace hack_asm {
    struct args {
        std::string path;
        std::string format;
        std::string output;
        bool recursive{};
        bool verbose{};
        bool quiet{};
    };

    enum class args_result { run, exit_ok, exit_error };

    auto parse_args(int argc, const char *argv[], args &out) -> args_result;
}
