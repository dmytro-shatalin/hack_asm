//
// Created by dshatalin on 10/25/26.
//

#include "args.hpp"
#include "pipeline.hpp"

#include "parser.hpp"
#include "threadlib/thread_pool.h"
#include "util.hpp"
#include <filesystem>
#include <functional>
#include <stop_token>
#include <vector>

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

  std::vector<std::function<hack_asm::file_result(std::stop_token)>> batch;
  for (const auto &p : paths) {
    batch.emplace_back(hack_asm::make_worker_func(p, args));
  }
  auto list = pool.submit_list(batch);

  auto results = list.all();

  // write parsed based on the args
  // todo: implement
}
