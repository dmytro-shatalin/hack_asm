#include "encoder.hpp"

namespace {

#ifdef _WIN32
constexpr std::string_view NEW_LINE = "\r\n";
#else
constexpr std::string_view NEW_LINE = "\n";
#endif

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

} // namespace

namespace hack_asm {

auto encoder<output_format::binary>::operator()(
    const std::span<const instruction_t> s) const -> std::vector<std::byte> {
  std::vector<std::byte> encoded;

  for (auto const &i : s) {
    std::visit(
        overloaded{
            [&](const a_instruction &inst) mutable {
              encoded.push_back(static_cast<std::byte>(inst.target >> 8));
              encoded.push_back(static_cast<std::byte>(inst.target & 0xFF));
            },
            [&](const c_instruction &inst) mutable {
              encoded.push_back(
                  static_cast<std::byte>(0b11100000 | (inst.comp >> 2)));
              encoded.push_back(static_cast<std::byte>(
                  ((inst.comp & 0b11) << 6) | ((inst.dest & 0b111) << 3) |
                  (inst.jump & 0b111)));
            },
        },
        i);
  }

  return encoded;
}

auto encoder<output_format::symbolic>::operator()(
    std::span<const instruction_t> const s) const -> std::vector<std::byte> {
  std::vector<std::byte> encoded;

  for (auto const &i : s) {
    std::visit(overloaded{
                   [&](const a_instruction &inst) mutable {
                     std::string str{"@" + std::to_string(inst.target)};
                     str.append(NEW_LINE);
                     for (auto b : str) {
                       encoded.push_back(static_cast<std::byte>(b));
                     }
                   },
                   [&](const c_instruction &inst) mutable {
                     std::string str{inst.mnemonic};
                     str.append(NEW_LINE);
                     for (auto b : str) {
                       encoded.push_back(static_cast<std::byte>(b));
                     }
                   },
               },
               i);
  }

  return encoded;
}

} // namespace hack_asm
