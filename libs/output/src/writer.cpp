//
// Created by dshatalin on 11/23/26.
//

#include "writer.hpp"
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <system_error>

namespace hack_asm {

void write_file(std::filesystem::path p, std::span<std::byte> bytes) {
  std::error_code ec;

  if (p.has_parent_path()) {
    const auto parent_path = p.parent_path();
    std::filesystem::create_directories(parent_path, ec);
    if (ec) {
      throw std::runtime_error{"failed to create parent path: " +
                               parent_path.string()};
    }
  }

  std::ofstream file{p, std::ios::binary | std::ios::trunc};

  file.write(reinterpret_cast<char *>(bytes.data()),
             static_cast<std::streamsize>(bytes.size_bytes()));
  if (file.bad()) {
    throw std::runtime_error("failed to write to the file: " + p.string());
  }
  file.close();
}

} // namespace hack_asm
