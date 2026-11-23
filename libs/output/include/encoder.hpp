//
// Created by dshatalin on 10/25/26.
//

#pragma once

#include "output_format.hpp"
#include <cstddef>
#include <vector>

namespace hack_asm {

    template<output_format Format>
    struct encoder;

    template<>
    struct encoder<output_format::binary> {
        auto operator()() -> std::vector<std::byte>;
    };

    template<>
    struct encoder<output_format::symantic> {
        auto operator()() -> std::vector<std::byte>;
    };

}
