#pragma once

#include <cstdint>
#include <memory>
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
        uint64_t global_counter;
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
        std::vector<std::string> values;
        std::vector<std::string> suffixes;
    };

    struct NodeInternal
    {
        uint64_t num_children;
        std::string stem;
        std::vector<uint64_t> offsets;
        std::vector<uint64_t> num_entries;
        std::vector<std::string> reduced_values;
        std::vector<std::string> suffixes;
    };

    struct NodeLeafShallow
    {
        uint64_t num_children;
        std::string stem;

        std::unique_ptr<uint8_t[]> data;
        uint8_t *strings_ptr;
        std::unique_ptr<uint64_t[]> string_offsets;

        std::string_view suffix(uint64_t index) const
        {
            return std::string_view((char *)strings_ptr + string_offsets[num_children + index - 1], string_offsets[num_children + index] - string_offsets[num_children + index - 1]);
        }

        std::string_view value(uint64_t index) const
        {
            if (index == 0)
            {
                return std::string_view((char *)strings_ptr, string_offsets[0]);
            }
            else
            {
                return std::string_view((char *)strings_ptr + string_offsets[index - 1], string_offsets[index] - string_offsets[index - 1]);
            }
        }
    };

    struct NodeInternalShallow
    {
        uint64_t num_children;
        std::string stem;

        std::unique_ptr<uint8_t[]> data;
        uint8_t *strings_ptr;
        std::unique_ptr<uint64_t[]> offsets;
        std::unique_ptr<uint64_t[]> num_entries;
        std::unique_ptr<uint64_t[]> string_offsets;

        std::string_view suffix(uint64_t index) const
        {
            return std::string_view((char *)strings_ptr + string_offsets[num_children + index - 1], string_offsets[num_children + index] - string_offsets[num_children + index - 1]);
        }

        std::string_view reduced_value(uint64_t index) const
        {
            if (index == 0)
            {
                return std::string_view((char *)strings_ptr, string_offsets[0]);
            }
            else
            {
                return std::string_view((char *)strings_ptr + string_offsets[index - 1], string_offsets[index] - string_offsets[index - 1]);
            }
        }
    };
}
