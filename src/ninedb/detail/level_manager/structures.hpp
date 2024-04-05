#pragma once

#include <cstdint>
#include <vector>

namespace ninedb::detail::level_manager
{
    struct Config
    {
        uint64_t max_level_count = 10;

        bool error_if_exists = false;

        bool delete_if_exists = true;
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
        uint32_t version_major;
        uint32_t version_minor;
        uint64_t next_index;
        std::vector<LevelState> levels;
    };
}
