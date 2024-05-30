#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ninedb::pbt::detail
{
#pragma pack(push, 1)
    struct Footer
    {
        // Tree structure values.
        uint64_t root_offset;
        uint64_t level_0_end;
        uint64_t tree_height;

        // Metadata values.
        uint64_t identifier;
        uint64_t num_entries;
        uint8_t compression;

        // Versioning values.
        uint16_t version_major;
        uint16_t version_minor;

        // Magic value.
        uint32_t magic;
    };
#pragma pack(pop)

    struct NodeLeaf
    {
        uint64_t num_children;
        std::string stem;
        std::vector<std::string> suffixes;
        std::vector<std::string> values;
    };

    struct NodeInternal
    {
        uint64_t num_children;
        std::string stem;
        std::vector<std::string> suffixes;
        std::vector<std::string> reduced_values;
        std::vector<uint64_t> offsets;
        std::vector<uint64_t> num_entries;
    };
}
