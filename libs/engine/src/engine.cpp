//
// Created by dshatalin on 10/25/26.
//

#include "engine.hpp"
#include "diagnostic_report.hpp"
#include "encoder.hpp"
#include "output_format.hpp"
#include "parser.hpp"
#include "writer.hpp"

#include "threadlib/thread_pool.h"
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <print>
#include <queue>
#include <span>
#include <stop_token>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

namespace fs = std::filesystem;

void remove_suffix(std::string &str, std::string_view suffix) {
  if (str.ends_with(suffix)) {
    str.erase(str.size() - suffix.size());
  }
}

auto read_source_lines(const fs::path &path) -> std::vector<std::string> {
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

class path_resolver {
  std::string ext_;
  bool r_;

public:
  explicit path_resolver(std::string_view ext, bool recur)
      : ext_(ext.starts_with('.') ? ext : "." + std::string(ext)), r_(recur) {}

  [[nodiscard]] auto resolve(std::span<std::string> inputs)
      -> std::expected<std::vector<fs::path>, std::error_code> {
    std::vector<fs::path> paths;
    for (const auto &p : inputs) {
      if (auto ec = resolve_paths(p, paths); ec) {
        return std::unexpected(ec);
      }
    }
    return paths;
  }

private:
  bool is_acceptable(fs::path p) { return p.extension().string() == ext_; }

  auto resolve_paths(std::string_view p, std::vector<fs::path> &paths)
      -> std::error_code {
    std::error_code ec;

    fs::path path{p};
    if (!fs::exists(path)) {
      return std::make_error_code(std::errc::no_such_file_or_directory);
    }

    if (!fs::is_directory(path)) {
      if (!is_acceptable(path)) {
        return {};
      }
      paths.push_back(std::move(path));
      return {};
    }

    std::queue<fs::path> dirs{{path}};
    while (!dirs.empty()) {
      auto dir = dirs.front();
      dirs.pop();
      for (const auto &p : fs::directory_iterator{dir}) {
        if (p.is_directory() && r_) {
          dirs.push(p);
          continue;
        }
        if (is_acceptable(p)) {
          paths.push_back(p);
        }
      }
    }

    return {};
  };
};

auto to_diagnostic(const std::exception &e) -> hack_asm::diagnostic {
  return hack_asm::diagnostic{
      .level = hack_asm::severity::error, .line = 0, .msg = e.what()};
}

struct file_result {
  std::filesystem::path ipath;
  std::string opath;
  std::optional<hack_asm::parse_result> parsed;
  std::optional<hack_asm::diagnostic> error;

  [[nodiscard]] bool has_diagnostics() const {
    return error.has_value() || (parsed.has_value() && parsed->has_warnings());
  }
};

auto make_job(std::filesystem::path p, std::string_view odir,
              std::string_view ext) {
  const auto ipath = std::filesystem::absolute(p).lexically_normal();

  std::string output = odir.empty() ? p.string() : odir + p.filename().string();
  remove_suffix(output, ".asm");
  output += ext;

  return [ipath, path = p,
          output = std::move(output)](std::stop_token) mutable -> file_result {
    try {
      std::ifstream file{path, std::ios::binary};
      if (!file) {
        throw std::runtime_error{"failed to open file: " + ipath.string()};
      }

      const auto file_size = std::filesystem::file_size(path);
      std::vector<std::byte> buffer(file_size);

      if (!file.read(reinterpret_cast<char *>(buffer.data()),
                     static_cast<std::streamsize>(file_size))) {
        throw std::runtime_error{"failed to read file: " + ipath.string()};
      }

      hack_asm::parser parser{std::move(buffer)};

      auto parsed = parser.parse();
      if (!parsed) {
        return file_result{
            .ipath = ipath,
            .opath = output,
            .parsed = std::nullopt,
            .error = parsed.error(),
        };
      }
      return file_result{
          .ipath = ipath,
          .opath = output,
          .parsed = std::move(*parsed),
          .error = std::nullopt,
      };
    } catch (std::exception &e) {
      return file_result{
          .ipath = ipath,
          .opath = output,
          .parsed = std::nullopt,
          .error = to_diagnostic(e),
      };
    }
  };
}

auto make_jobs(std::span<std::filesystem::path> ipaths, std::string_view odir,
               std::string_view ext)
    -> std::vector<std::function<file_result(std::stop_token)>> {
  std::vector<std::function<file_result(std::stop_token)>> jobs;
  for (const auto &p : ipaths) {
    jobs.push_back(make_job(p, odir, ext));
  }
  return jobs;
}

void report(const file_result &fr, bool quiet) {
  if (!quiet) {
    return;
  }
  const auto ip = fr.ipath;
  std::optional<std::vector<std::string>> lines;
  if (fr.error) {
    lines = read_source_lines(ip);
    std::println(
        stderr, "{}",
        hack_asm::diagnostic_report{ip, fr.error.value(), lines.value()}.str());
  }
  if (fr.parsed && fr.parsed->has_warnings()) {
    if (!lines) {
      lines = read_source_lines(ip);
    }
    for (const auto &w : fr.parsed->warnings_) {
      std::println("{}",
                   hack_asm::diagnostic_report{ip, w, lines.value()}.str());
    }
  }
}

} // namespace

namespace hack_asm {

class assembler::impl {
  threadlib::thread_pool pool_;

  path_resolver resolver_;

  std::span<std::string> ipaths_;

  std::string odir_;

  bool q_{};

public:
  explicit impl(plan plan)
      : pool_(plan.threads),
        resolver_(assembler::source_extension, plan.recursive),
        ipaths_(plan.inputs), odir_(plan.output_dir), q_(plan.quiet) {}

  ~impl() { pool_.stop_forced(); }

  impl(impl &) = delete;

  impl(impl &&other) noexcept = default;

  impl &operator=(impl &) = delete;

  impl &operator=(impl &&) noexcept = default;

  template <output_format Format> auto run() -> bool {
    constexpr auto ext = extention_for<Format>;
    const encoder<Format> encode;

    auto paths = resolver_.resolve(ipaths_);
    if (!paths) {
      std::println(stderr, "failed to resolve paths");
      return false;
    }

    std::vector<std::function<file_result(std::stop_token)>> jobs =
        ::make_jobs(paths.value(), odir_, ext);
    auto list = pool_.submit_list(jobs);
    const auto results = list.all();

    bool failed = false;
    for (const auto &r : results) {
      ::report(r, q_);
      if (!r.parsed) {
        failed = true;
        continue;
      }
      auto encoded = encode(r.parsed->instructions_);
      write_file(r.opath, encoded);
    }

    return !failed;
  }
};

assembler::assembler(plan plan)
    : pimpl_(std::make_unique<assembler::impl>(plan)), f_(plan.format) {};

assembler::~assembler() = default;

assembler::assembler(assembler &&) noexcept = default;

assembler &assembler::operator=(assembler &&) noexcept = default;

auto assembler::run() -> bool {
  switch (f_) {
  case hack_asm::output_format::binary:
    return pimpl_->run<hack_asm::output_format::binary>();
  case hack_asm::output_format::symbolic:
    return pimpl_->run<hack_asm::output_format::symbolic>();
  }
  std::unreachable();
}

} // namespace hack_asm
