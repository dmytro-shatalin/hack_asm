//
// Created by dshatalin on 10/25/26.
//

#include "args.hpp"

#include "parser.hpp"
#include "threadlib/task.h"
#include "threadlib/thread_pool.h"
#include "util.hpp"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <utility>
#include <vector>

struct parse_result {
  std::string ipath;
  std::string opath;
  std::vector<hack_asm::instruction_t> parsed;
  std::vector<hack_asm::diagnostic> warnings;
};

std::function<parse_result(std::stop_token)>
make_worker_func(const std::filesystem::path &p, const hack_asm::args &args) {
  std::string output;
  output =
      args.output.empty() ? p.string() : args.output + p.filename().string();
  hack_asm::util::remove_suffix(output, ".asm");
  output += ".hack";
  return [path = p, output = output](std::stop_token) {
    parse_result result{
        .ipath = path.string(),
        .opath = output,
        .parsed = {},
        .warnings = {},
    };

    std::ifstream file{path, std::ios::binary};
    if (!file) {
      throw std::runtime_error{"failed to open file: " + path.string()};
    }

    const auto fileSize = std::filesystem::file_size(path);
    std::vector<std::byte> buffer(fileSize);

    if (!file.read(reinterpret_cast<char *>(buffer.data()), fileSize)) {
      throw std::runtime_error{"failed to read file: " + path.string()};
    }

    hack_asm::parser parser{std::move(buffer)};

    auto parsed = parser.parse();
    if (!parsed) {
      throw hack_asm::parse_error{std::move(parsed.error())};
    }
    result.parsed = std::move(*parsed);
    result.warnings = parser.warnings();

    return result;
  };
}

int main(const int argc, const char *argv[]) {
  hack_asm::args args{};
  if (const auto result = hack_asm::parse_args(argc, argv, args);
      result != hack_asm::args_result::run) {
    return result == hack_asm::args_result::exit_ok ? EXIT_SUCCESS
                                                    : EXIT_FAILURE;
  }

  // resolve filepaths from args
  auto paths = hack_asm::util::resolve_paths(args);

  // start worker pool and submit all files for parse
  threadlib::thread_pool pool{args.threads};

  std::vector<std::function<parse_result(std::stop_token)>> batch;
  for (const auto &p : paths) {
    batch.emplace_back(make_worker_func(p, args));
  }
  auto list = pool.submit_list(batch);

  auto results = list.all();

  // write parsed based on the args
}
