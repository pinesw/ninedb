#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "./format.hpp"

namespace ninedb::pbt::detail
{
#pragma pack(push, 1)
    struct Footer
    {
        static const uint32_t MAGIC = 0x1EAF1111;
        static const uint32_t VERSION_MAJOR = 0;
        static const uint32_t VERSION_MINOR = 1;

        // Root node handle.
        uint64_t root_offset;
        uint64_t root_size;

        // Tree structure values.
        uint16_t tree_height;
        // uint8_t enable_lz4_compression; // TODO: implement
        // uint8_t enable_varint_compression; // TODO: implement
        // uint8_t enable_prefix_compression; // TODO: implement

        // Metadata values.
        uint64_t global_start;
        uint64_t global_end;

        // Versioning values.
        uint16_t version_major;
        uint16_t version_minor;

        // Magic value.
        uint32_t magic;

        Footer()
        {
            ZonePbtStructures;

            this->version_major = VERSION_MAJOR;
            this->version_minor = VERSION_MINOR;
            this->magic = MAGIC;
        }

        static uint64_t write(char *address, const Footer &footer)
        {
            ZonePbtStructures;

            address += Format::write_uint64(address, footer.root_offset);
            address += Format::write_uint64(address, footer.root_size);
            address += Format::write_uint16(address, footer.tree_height);
            address += Format::write_uint64(address, footer.global_start);
            address += Format::write_uint64(address, footer.global_end);
            address += Format::write_uint16(address, footer.version_major);
            address += Format::write_uint16(address, footer.version_minor);
            address += Format::write_uint32(address, footer.magic);

            return Footer::size_of();
        }

        static uint64_t read(char *address, Footer &footer)
        {
            ZonePbtFormat;

            address += Format::read_uint64(address, footer.root_offset);
            address += Format::read_uint64(address, footer.root_size);
            address += Format::read_uint16(address, footer.tree_height);
            address += Format::read_uint64(address, footer.global_start);
            address += Format::read_uint64(address, footer.global_end);
            address += Format::read_uint16(address, footer.version_major);
            address += Format::read_uint16(address, footer.version_minor);
            address += Format::read_uint32(address, footer.magic);

            return Footer::size_of();
        }

        static constexpr uint64_t size_of()
        {
            return sizeof(Footer);
        }

        void validate()
        {
            ZonePbtStructures;

            if (this->magic != MAGIC)
            {
                throw std::runtime_error("Invalid magic number");
            }
            if (this->version_major != VERSION_MAJOR)
            {
                throw std::runtime_error("Invalid major version");
            }
            if (this->version_minor != VERSION_MINOR)
            {
                throw std::runtime_error("Invalid minor version");
            }
        }
    };
#pragma pack(pop)

    /**
     * Write-only structure for leaf nodes.
     */
    struct NodeLeafBuilder
    {
        uint16_t num_children;
        std::vector<uint64_t> data_offsets;
        std::vector<uint64_t> key_sizes;
        std::vector<uint64_t> value_sizes;
        std::string data;

        NodeLeafBuilder() : num_children(0) {}

        void add_key_value(const std::string_view &key, const std::string_view &value)
        {
            ZonePbtStructures;

            num_children++;
            data_offsets.push_back(data.size());
            key_sizes.push_back(key.size());
            value_sizes.push_back(value.size());
            data.append(key);
            data.append(value);
        }

        void clear()
        {
            ZonePbtStructures;

            num_children = 0;
            data_offsets.clear();
            key_sizes.clear();
            value_sizes.clear();
            data.clear();
        }

        uint64_t size_of() const
        {
            ZonePbtStructures;

            uint64_t size = sizeof(uint16_t);
            size += 3 * sizeof(uint64_t) * this->num_children;
            size += this->data.size();

            return size;
        }
    };

    /**
     * Read-only structure for leaf nodes.
     */
    struct NodeLeaf
    {
        static uint64_t write(char *address, const NodeLeafBuilder &node)
        {
            ZonePbtStructures;

            char *base = address;
            uint64_t header_size = sizeof(uint16_t) + 3 * sizeof(uint64_t) * node.num_children;

            address += Format::write_uint16(address, node.num_children);
            for (uint16_t i = 0; i < node.num_children; i++)
            {
                address += Format::write_uint64(address, node.data_offsets[i] + header_size);
                address += Format::write_uint64(address, node.key_sizes[i]);
                address += Format::write_uint64(address, node.value_sizes[i]);
            }
            address += Format::write_string_data_only(address, node.data);

            return address - base;
        }

        static uint64_t size_of(char *address)
        {
            ZonePbtStructures;

            uint16_t num_children;
            Format::read_uint16(address, num_children);

            address += sizeof(uint16_t) + 3 * sizeof(uint64_t) * (num_children - 1);
            uint64_t data_offset;
            uint64_t key_size;
            uint64_t value_size;
            address += Format::read_uint64(address, data_offset);
            address += Format::read_uint64(address, key_size);
            address += Format::read_uint64(address, value_size);

            return data_offset + key_size + value_size;
        }

        static uint16_t read_num_children(char *address)
        {
            ZonePbtStructures;

            uint16_t num_children;
            Format::read_uint16(address, num_children);
            return num_children;
        }

        static std::string_view read_key(char *address, uint16_t i)
        {
            ZonePbtStructures;

            char *base = address;
            address += sizeof(uint16_t) + 3 * sizeof(uint64_t) * i;
            uint64_t data_offset;
            uint64_t key_size;
            address += Format::read_uint64(address, data_offset);
            address += Format::read_uint64(address, key_size);

            return std::string_view((char *)base + data_offset, key_size);
        }

        static std::string_view read_value(char *address, uint16_t i)
        {
            ZonePbtStructures;

            char *base = address;
            address += sizeof(uint16_t) + 3 * sizeof(uint64_t) * i;
            uint64_t data_offset;
            uint64_t key_size;
            uint64_t value_size;
            address += Format::read_uint64(address, data_offset);
            address += Format::read_uint64(address, key_size);
            address += Format::read_uint64(address, value_size);

            return std::string_view((char *)base + data_offset + key_size, value_size);
        }
    };

    /**
     * Write-only structure for internal nodes.
     */
    struct NodeInternalBuilder
    {
        uint16_t num_children;
        std::vector<uint64_t> data_offsets;
        std::vector<uint64_t> key_sizes;
        std::vector<uint64_t> reduced_value_sizes;
        std::vector<uint64_t> child_entry_starts;
        std::vector<uint64_t> child_offsets;
        std::vector<uint64_t> child_sizes;
        std::string data;

        NodeInternalBuilder() : num_children(0) {}

        void add_first_child(const std::string_view &left_key, const std::string_view &right_key, const std::string_view &reduced_value, uint64_t child_entry_start, uint64_t child_offset, uint64_t child_size)
        {
            ZonePbtStructures;

            data_offsets.push_back(data.size());
            key_sizes.push_back(left_key.size());
            data.append(left_key);
            add_child(right_key, reduced_value, child_entry_start, child_offset, child_size);
        }

        void add_child(const std::string_view &right_key, const std::string_view &reduced_value, uint64_t child_entry_start, uint64_t child_offset, uint64_t child_size)
        {
            ZonePbtStructures;

            num_children++;
            data_offsets.push_back(data.size());
            key_sizes.push_back(right_key.size());
            reduced_value_sizes.push_back(reduced_value.size());
            child_entry_starts.push_back(child_entry_start);
            child_offsets.push_back(child_offset);
            child_sizes.push_back(child_size);
            data.append(right_key);
            data.append(reduced_value);
        }

        void clear()
        {
            ZonePbtStructures;

            num_children = 0;
            data_offsets.clear();
            key_sizes.clear();
            reduced_value_sizes.clear();
            child_entry_starts.clear();
            child_offsets.clear();
            child_sizes.clear();
            data.clear();
        }

        uint64_t size_of() const
        {
            ZonePbtStructures;

            uint64_t size = sizeof(uint16_t);
            size += 2 * sizeof(uint64_t);
            size += 6 * sizeof(uint64_t) * this->num_children;
            size += this->data.size();

            return size;
        }
    };

    /**
     * Read-only structure for internal nodes.
     */
    struct NodeInternal
    {
        static uint64_t write(char *address, const NodeInternalBuilder &node)
        {
            ZonePbtStructures;

            char *base = address;
            uint64_t header_size = sizeof(uint16_t) + 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * node.num_children;

            address += Format::write_uint16(address, node.num_children);

            address += Format::write_uint64(address, node.data_offsets[0] + header_size);
            address += Format::write_uint64(address, node.key_sizes[0]);
            for (uint16_t i = 0; i < node.num_children; i++)
            {
                address += Format::write_uint64(address, node.data_offsets[i + 1] + header_size);
                address += Format::write_uint64(address, node.key_sizes[i + 1]);
                address += Format::write_uint64(address, node.reduced_value_sizes[i]);
                address += Format::write_uint64(address, node.child_entry_starts[i]);
                address += Format::write_uint64(address, node.child_offsets[i]);
                address += Format::write_uint64(address, node.child_sizes[i]);
            }

            address += Format::write_string_data_only(address, node.data);

            return address - base;
        }

        static uint64_t size_of(char *address)
        {
            ZonePbtStructures;

            uint16_t num_children;
            Format::read_uint16(address, num_children);

            address += sizeof(uint16_t) + 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * (num_children - 1);
            uint64_t data_offset;
            uint64_t key_size;
            uint64_t reduced_value_size;
            address += Format::read_uint64(address, data_offset);
            address += Format::read_uint64(address, key_size);
            address += Format::read_uint64(address, reduced_value_size);

            return data_offset + key_size + reduced_value_size;
        }

        static uint16_t read_num_children(char *address)
        {
            ZonePbtStructures;

            uint16_t num_children;
            Format::read_uint16(address, num_children);
            return num_children;
        }

        static std::string_view read_left_key(char *address)
        {
            ZonePbtStructures;

            char *base = address;
            address += sizeof(uint16_t);
            uint64_t data_offset;
            uint64_t key_size;
            address += Format::read_uint64(address, data_offset);
            address += Format::read_uint64(address, key_size);

            return std::string_view((char *)base + data_offset, key_size);
        }

        static std::string_view read_right_key(char *address, uint16_t i)
        {
            ZonePbtStructures;

            char *base = address;
            address += sizeof(uint16_t) + 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * i;
            uint64_t data_offset;
            uint64_t key_size;
            address += Format::read_uint64(address, data_offset);
            address += Format::read_uint64(address, key_size);

            return std::string_view((char *)base + data_offset, key_size);
        }

        static std::string_view read_reduced_value(char *address, uint16_t i)
        {
            ZonePbtStructures;

            char *base = address;
            address += sizeof(uint16_t) + 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * i;
            uint64_t data_offset;
            uint64_t key_size;
            uint64_t reduced_value_size;
            address += Format::read_uint64(address, data_offset);
            address += Format::read_uint64(address, key_size);
            address += Format::read_uint64(address, reduced_value_size);

            return std::string_view((char *)base + data_offset + key_size, reduced_value_size);
        }

        static uint64_t read_child_entry_start(char *address, uint16_t i)
        {
            ZonePbtStructures;

            address += sizeof(uint16_t) + 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * i;
            uint64_t child_entry_start;
            address += Format::skip_uint64(3);
            address += Format::read_uint64(address, child_entry_start);

            return child_entry_start;
        }

        static uint64_t read_child_offset(char *address, uint16_t i)
        {
            ZonePbtStructures;

            address += sizeof(uint16_t) + 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * i;
            uint64_t child_offset;
            address += Format::skip_uint64(4);
            address += Format::read_uint64(address, child_offset);

            return child_offset;
        }

        static uint64_t read_child_size(char *address, uint16_t i)
        {
            ZonePbtStructures;

            address += sizeof(uint16_t) + 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * i;
            uint64_t child_size;
            address += Format::skip_uint64(5);
            address += Format::read_uint64(address, child_size);

            return child_size;
        }
    };
}
