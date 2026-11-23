//
// Created by dshatalin on 11/23/26.
//

#pragma once

#include "args.hpp"
#include "parser.hpp"

#include <filesystem>
#include <functional>
#include <stop_token>
#include <string>
#include <vector>

namespace hack_asm {

struct file_result {
  std::string ipath;
  std::string opath;
  parse_result result;
};

auto make_worker_func(const std::filesystem::path &p, const args &args)
    -> std::function<file_result(std::stop_token)>;

} // namespace hack_asm
