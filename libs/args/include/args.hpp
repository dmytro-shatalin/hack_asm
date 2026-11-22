//
// Created by dshatalin on 10/25/26.
//

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hack_asm {
struct args {
  std::vector<std::string> input;
  std::string output;
  std::string format;
  std::uint16_t threads;
  bool recursive{};
  bool verbose{};
  bool quiet{};
};

enum class args_result { run, exit_ok, exit_error };

auto parse_args(int argc, const char *argv[], args &out) -> args_result;
} // namespace hack_asm
