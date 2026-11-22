//
// Created by dshatalin on 10/25/26.
//

#pragma once

#include <cstdint>
#include <string>

namespace hack_asm {
struct args {
  std::string path;
  std::string format;
  std::string output;
  std::uint16_t threads;
  bool recursive{};
  bool verbose{};
  bool quiet{};
};

enum class args_result { run, exit_ok, exit_error };

auto parse_args(int argc, const char *argv[], args &out) -> args_result;
} // namespace hack_asm
