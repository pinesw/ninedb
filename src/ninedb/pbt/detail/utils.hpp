#pragma once

#include <cstdint>

namespace ninedb::pbt::detail
{
    constexpr uint64_t div_ceil(uint64_t x, uint64_t y)
    {
        return (x + y - 1) / y;
    }
}
