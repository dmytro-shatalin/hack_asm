//
// Created by dshatalin on 11/23/26.
//

#pragma once

#include <cstddef>
#include <filesystem>
#include <span>

namespace hack_asm {

void write_file(std::filesystem::path p, std::span<std::byte> bytes);

}
