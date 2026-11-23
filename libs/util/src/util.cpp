//
// Created by dshatalin on 11/23/26.
//

#include <filesystem>
#include <fstream>
#include <vector>

namespace hack_asm::util {

auto read_source_lines(const std::filesystem::path &path)
    -> std::vector<std::string> {
  std::ifstream file{path};
  std::vector<std::string> lines;
  std::string line;

  while (std::getline(file, line)) {
    if (line.ends_with('\r')) {
      line.pop_back();
    }
    lines.push_back(std::move(line));
  }

  return lines;
}

} // namespace hack_asm::util