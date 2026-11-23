//
// Created by dshatalin on 10/25/26.
//

#pragma once

#include "output_format.hpp"
#include "parser.hpp"

#include <cstddef>
#include <span>
#include <utility>
#include <vector>

namespace hack_asm {

template <output_format Format> struct encoder;

template <> struct encoder<output_format::binary> {
  auto operator()(std::span<const instruction_t> s) const -> std::vector<std::byte>;
};

template <> struct encoder<output_format::symbolic> {
  auto operator()(std::span<const instruction_t> s) const -> std::vector<std::byte>;
};

inline auto encode(const output_format f, const std::span<instruction_t> s)
    -> std::vector<std::byte> {
  switch (f) {
  case output_format::binary:
    return encoder<output_format::binary>{}(s);
  case output_format::symbolic:
    return encoder<output_format::symbolic>{}(s);
  }
  std::unreachable();
}

} // namespace hack_asm
