//
// Created by dshatalin on 10/25/26.
//

#include "engine.hpp"
#include "diagnostic_report.hpp"
#include "encoder.hpp"
#include "output_format.hpp"
#include "parser.hpp"

#include "threadlib/thread_pool.h"
#include "writer.hpp"
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <print>
#include <stop_token>
#include <string_view>
#include <utility>
#include <vector>

namespace {

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
  hack_asm::util::remove_suffix(output, ".asm");
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
    lines = hack_asm::util::read_source_lines(ip);
    std::println(
        "{}",
        hack_asm::diagnostic_report{ip, fr.error.value(), lines.value()}.str());
  }
  if (fr.parsed && fr.parsed->has_warnings()) {
    if (!lines) {
      lines = hack_asm::util::read_source_lines(ip);
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

  std::vector<std::filesystem::path> ipaths_;

  std::string odir_;

  bool q_{};

public:
  explicit impl(plan plan)
      : pool_(plan.threads), ipaths_(plan.input), odir_(plan.output_dir),
        q_(plan.quiet) {}

  ~impl() { pool_.stop_forced(); }

  impl(impl &) = delete;

  impl(impl &&other) noexcept = default;

  impl &operator=(impl &) = delete;

  impl &operator=(impl &&) noexcept = default;

  template <output_format Format> auto run() -> bool {
    constexpr auto ext = extention_for<Format>;
    const encoder<Format> encode;

    std::vector<std::function<file_result(std::stop_token)>> jobs =
        ::make_jobs(ipaths_, odir_, ext);
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
