//
// Created by dshatalin on 11/23/26.
//

#pragma once

#include "parser.hpp"

#include <filesystem>
#include <iosfwd>
#include <span>
#include <string>

namespace hack_asm {

// Colors are used only when stderr is a terminal and NO_COLOR is not set.
[[nodiscard]] auto color_supported() noexcept -> bool;

class diagnostic_report {
  std::filesystem::path path_;
  diagnostic diag_;
  std::span<const std::string> lines_;
  bool use_color_;

public:
  diagnostic_report(std::filesystem::path path, const diagnostic &diag,
                    std::span<const std::string> lines,
                    bool use_color = color_supported());

  [[nodiscard]] auto str() const -> std::string;

  friend auto operator<<(std::ostream &os, const diagnostic_report &report)
      -> std::ostream &;
};

} // namespace hack_asm
