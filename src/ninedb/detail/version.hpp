#pragma once

#include <cstdint>

namespace ninedb::detail
{
    // Semver constants for the file formats.
    // When a file format is not backwards compatible, the major version must be incremented.

    constexpr uint32_t VERSION_MAJOR = 0;
    constexpr uint32_t VERSION_MINOR = 1;
}
