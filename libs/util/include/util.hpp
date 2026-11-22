//
// Created by dshatalin on 10/15/26.
//

#pragma once

#include "args.hpp"
#include <filesystem>
#include <string_view>
#include <system_error>
#include <vector>

namespace hack_asm {

namespace util {

inline void remove_suffix(std::string &str, std::string_view suffix) {
  if (str.ends_with(suffix)) {
    str.erase(str.size() - suffix.size());
  }
}

inline std::error_code resolve_paths(std::string_view p, bool recursive,
                                     std::vector<std::filesystem::path> &v) {
  std::error_code ec;

  std::filesystem::path path{p};
  if (!std::filesystem::exists(path)) {
    return std::make_error_code(std::errc::no_such_file_or_directory);
  }

  if (!std::filesystem::is_directory(path)) {
    v.push_back(std::move(path));
  }
  for (auto const &entry : std::filesystem::directory_iterator{path, ec}) {
    if (ec) {
      return ec;
    }
    if (ec = resolve_paths(entry.path().string(), recursive, v); ec) {
      return ec;
    }
  }
  return {};
}

inline std::vector<std::filesystem::path> resolve_paths(hack_asm::args &args) {
  std::vector<std::filesystem::path> paths;
  for (auto const &p : args.files) {
    if (auto ec = hack_asm::util::resolve_paths(p, args.recursive, paths); ec) {
      throw std::filesystem::filesystem_error{p, ec};
    }
  }
  return paths;
}

} // namespace util

} // namespace hack_asm
