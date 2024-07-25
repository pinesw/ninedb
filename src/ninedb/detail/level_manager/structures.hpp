#pragma once

#include <cstdint>
#include <vector>

namespace ninedb::detail::level_manager
{
    struct Config
    {
        uint64_t max_level_count = 10;
    };

    struct LevelAndIndex
    {
        uint64_t level;
        uint64_t index;
    };

    struct MergeOperation
    {
        std::vector<LevelAndIndex> src_levels_and_indices;
        uint64_t dst_level;
        uint64_t dst_index;
    };

    struct LevelState
    {
        uint64_t level;
        std::vector<uint64_t> indices;
    };

    struct State
    {
        uint64_t next_index;
        uint64_t global_start;
        std::vector<LevelState> levels;
    };
}
