//
// Created by dshatalin on 11/23/26.
//

#include "diagnostic_report.hpp"

#include <cstdlib>
#include <format>
#include <ostream>
#include <string_view>
#include <utility>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace {

constexpr std::string_view red{"\033[31m"};
constexpr std::string_view yellow{"\033[33m"};
constexpr std::string_view reset{"\033[0m"};

auto color_for(const hack_asm::severity level) noexcept -> std::string_view {
  switch (level) {
  case hack_asm::severity::error:
    return red;
  case hack_asm::severity::warning:
  case hack_asm::severity::weak_warning:
    return yellow;
  }
  return {};
}

} // namespace

namespace hack_asm {

auto color_supported() noexcept -> bool {
  if (std::getenv("NO_COLOR") != nullptr) {
    return false;
  }

#ifdef _WIN32
  return _isatty(_fileno(stderr)) != 0;
#else
  return isatty(STDERR_FILENO) != 0;
#endif
}

diagnostic_report::diagnostic_report(std::filesystem::path path,
                                     const diagnostic &diag,
                                     std::span<const std::string> lines,
                                     const bool use_color)
    : path_{std::move(path)}, diag_{diag}, lines_{lines},
      use_color_{use_color} {}

auto diagnostic_report::str() const -> std::string {
  auto out = std::format("{}: {}", path_.string(), diag_.msg);

  if (diag_.line == 0 || diag_.line > lines_.size()) {
    return out;
  }

  const auto target = static_cast<long>(diag_.line) - 1;
  const auto start = std::min<long>(0, target - 3);
  const auto end = std::min<long>(lines_.size(), target + 4);

  for (int i = start; i < end; i++) {
    const auto source = lines_[i];
    if (i != target || !use_color_) {
      out += std::format("\n{:>6} | {}", diag_.line, source);
    } else {
      out += std::format("\n{:>6} | {}{}{}", diag_.line, color_for(diag_.level),
                         source, reset);
    }
  }

  return out;
}

auto operator<<(std::ostream &os, const diagnostic_report &report)
    -> std::ostream & {
  return os << report.str();
}

} // namespace hack_asm
