//
// Created by dshatalin on 10/25/26.
//

#pragma once

#include "args.hpp"
#include "output_format.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hack_asm {

template <output_format Format> constexpr std::string_view extention_for;

template <>
inline constexpr std::string_view extention_for<output_format::binary> =
    ".hack";

template <>
inline constexpr std::string_view extention_for<output_format::symbolic> =
    ".symbolic.asm";

struct plan {
  std::vector<std::string> inputs;
  std::string output_dir;
  output_format format;
  std::uint16_t threads;
  bool recursive{};
  bool quiet{};
};

inline auto make_plan(const args &args) -> plan {
  return {
      .inputs = args.inputs,
      .output_dir = args.output,
      .format = args.format,
      .threads = args.threads,
      .recursive = args.recursive,
      .quiet = args.quiet,
  };
}

class assembler {
  class impl;

  std::unique_ptr<impl> pimpl_;

  output_format f_;

public:
  static constexpr std::string_view source_extension = ".asm";

  static auto from_args(const args &args) -> assembler {
    return assembler(make_plan(args));
  }

  explicit assembler(plan plan);

  ~assembler();

  assembler(const assembler &) = delete;

  assembler(assembler &&) noexcept;

  assembler &operator=(const assembler &) = delete;

  assembler &operator=(assembler &&) noexcept;

  [[nodiscard]] auto run() -> bool;
};

} // namespace hack_asm
