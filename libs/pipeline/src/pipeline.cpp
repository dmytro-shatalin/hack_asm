#include "pipeline.hpp"

#include "util.hpp"

#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace hack_asm {

auto make_worker_func(const std::filesystem::path &p, const args &args)
    -> std::function<file_result(std::stop_token)> {
  std::string output =
      args.output.empty() ? p.string() : args.output + p.filename().string();
  util::remove_suffix(output, ".asm");
  output += ".hack";

  return [path = p, output = std::move(output)](std::stop_token) -> file_result {
    std::ifstream file{path, std::ios::binary};
    if (!file) {
      throw std::runtime_error{"failed to open file: " + path.string()};
    }

    const auto file_size = std::filesystem::file_size(path);
    std::vector<std::byte> buffer(file_size);

    if (!file.read(reinterpret_cast<char *>(buffer.data()),
                   static_cast<std::streamsize>(file_size))) {
      throw std::runtime_error{"failed to read file: " + path.string()};
    }

    parser parser{std::move(buffer)};

    auto parsed = parser.parse();
    if (!parsed) {
      throw parse_error{std::move(parsed.error())};
    }
    return file_result{
      .ipath = path.string(),
      .opath = output,
      .result = *parsed,
    };
  };
}

} // namespace hack_asm
