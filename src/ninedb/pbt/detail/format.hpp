#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <lz4.h>

#include "../../detail/profiling.hpp"

#include "./structures.hpp"

namespace ninedb::pbt::detail
{
    struct Format
    {
        static const uint32_t MAGIC = 0x1EAF1111;

        static void validate_footer(const Footer &footer)
        {
            ZonePbtFormat;

            if (footer.magic != MAGIC)
            {
                throw std::runtime_error("Invalid magic number");
            }
            if (footer.version_major != ninedb::detail::VERSION_MAJOR)
            {
                throw std::runtime_error("Invalid major version");
            }
            if (footer.version_minor != ninedb::detail::VERSION_MINOR)
            {
                throw std::runtime_error("Invalid minor version");
            }
        }

        static Footer create_footer()
        {
            ZonePbtFormat;

            Footer footer;
            footer.version_major = ninedb::detail::VERSION_MAJOR;
            footer.version_minor = ninedb::detail::VERSION_MINOR;
            footer.magic = MAGIC;
            return footer;
        }

        static constexpr uint64_t sizeof_footer()
        {
            return sizeof(Footer);
        }

        static constexpr uint64_t size_upper_bound_varint64()
        {
            return 10;
        }

        static uint64_t size_upper_bound_string(std::string_view value)
        {
            ZonePbtFormat;

            return size_upper_bound_varint64() + value.size();
        }

        template <bool compressed>
        static uint64_t size_upper_bound_node_leaf(const NodeLeaf &node)
        {
            ZonePbtFormat;

            uint64_t size = size_upper_bound_varint64() + size_upper_bound_string(node.stem);
            size += 2 * node.num_children * size_upper_bound_varint64();
            for (uint64_t i = 0; i < node.num_children; i++)
            {
                size += node.suffixes[i].size();
                size += node.values[i].size();
            }

            if (compressed)
            {
                return LZ4_compressBound((int)size);
            }
            else
            {
                return size;
            }
        }

        template <bool compressed>
        static uint64_t size_upper_bound_node_internal(const NodeInternal &node)
        {
            ZonePbtFormat;

            uint64_t size = size_upper_bound_varint64() + size_upper_bound_string(node.stem);
            for (uint64_t i = 0; i < node.num_children; i++)
            {
                size += size_upper_bound_string(node.suffixes[i]);
                size += size_upper_bound_string(node.reduced_values[i]);
                size += sizeof(uint64_t);
                size += size_upper_bound_varint64();
            }
            size += size_upper_bound_string(node.suffixes[node.num_children]);

            if (compressed)
            {
                return LZ4_compressBound((int32_t)size);
            }
            else
            {
                return size;
            }
        }

        static void write_footer(uint8_t *&address, const Footer &footer)
        {
            ZonePbtFormat;

            *(uint64_t *)(address) = footer.root_offset;
            address += sizeof(uint64_t);
            *(uint64_t *)(address) = footer.level_0_end;
            address += sizeof(uint64_t);
            *(uint64_t *)(address) = footer.tree_height;
            address += sizeof(uint64_t);
            *(uint64_t *)(address) = footer.identifier;
            address += sizeof(uint64_t);
            *(uint64_t *)(address) = footer.num_entries;
            address += sizeof(uint64_t);
            *(uint8_t *)(address) = footer.compression;
            address += sizeof(uint8_t);
            *(uint16_t *)(address) = footer.version_major;
            address += sizeof(uint16_t);
            *(uint16_t *)(address) = footer.version_minor;
            address += sizeof(uint16_t);
            *(uint32_t *)(address) = footer.magic;
            address += sizeof(uint32_t);
        }

        static void write_uint64(uint8_t *&address, uint64_t value)
        {
            ZonePbtFormat;

            *(uint64_t *)(address) = value;
            address += sizeof(uint64_t);
        }

        static void write_varint64(uint8_t *&address, uint64_t value)
        {
            ZonePbtFormat;

            while (value >= 0x80)
            {
                *address++ = static_cast<uint8_t>(value) | 0x80;
                value >>= 7;
            }
            *address++ = static_cast<uint8_t>(value);
        }

        static void write_string(uint8_t *&address, std::string_view value)
        {
            ZonePbtFormat;

            write_varint64(address, value.size());
            memcpy(address, value.data(), value.size());
            address += value.size();
        }

        static void write_data_compressed(uint8_t *&address, const uint8_t *data, uint64_t size)
        {
            ZonePbtFormat;

            uint64_t max_size_compressed = LZ4_compressBound((int)size);
            std::unique_ptr<uint8_t[]> buffer_compressed = std::make_unique<uint8_t[]>(max_size_compressed);
            uint64_t compressed_size = LZ4_compress_default((char *)data, (char *)buffer_compressed.get(), (int)size, (int)max_size_compressed);
            write_varint64(address, compressed_size);
            write_varint64(address, size);
            memcpy(address, buffer_compressed.get(), compressed_size);
            address += compressed_size;
        }

        static void read_data_compressed(uint8_t *&address, std::unique_ptr<uint8_t[]> &buffer, uint64_t &size)
        {
            ZonePbtFormat;

            uint64_t compressed_size = read_varint64(address);
            uint64_t size_uncompressed = read_varint64(address);
            buffer = std::make_unique<uint8_t[]>(size_uncompressed);
            LZ4_decompress_safe((char *)address, (char *)buffer.get(), (int)compressed_size, (int)size_uncompressed);
            size = size_uncompressed;
            address += compressed_size;
        }

        template <bool compressed>
        static void write_node_leaf(uint8_t *&address, const NodeLeaf &node)
        {
            ZonePbtFormat;

            if (compressed)
            {
                std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(size_upper_bound_node_leaf<false>(node));
                uint8_t *buffer_address = buffer.get();
                write_node_leaf<false>(buffer_address, node);
                uint64_t size = buffer_address - buffer.get();
                write_data_compressed(address, buffer.get(), size);
            }
            else
            {
                write_varint64(address, node.num_children);
                write_string(address, node.stem);
                for (uint64_t i = 0; i < node.num_children; i++)
                {
                    write_string(address, node.suffixes[i]);
                    write_string(address, node.values[i]);
                }
            }
        }

        template <bool compressed>
        static void write_node_internal(uint8_t *&address, const NodeInternal &node)
        {
            ZonePbtFormat;

            if (compressed)
            {
                std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(size_upper_bound_node_internal<false>(node));
                uint8_t *buffer_address = buffer.get();
                write_node_internal<false>(buffer_address, node);
                uint64_t size = buffer_address - buffer.get();
                write_data_compressed(address, buffer.get(), size);
            }
            else
            {
                write_varint64(address, node.num_children);
                write_string(address, node.stem);
                for (uint64_t i = 0; i < node.num_children; i++)
                {
                    write_string(address, node.suffixes[i]);
                    write_string(address, node.reduced_values[i]);
                    write_uint64(address, node.offsets[i]);
                    write_varint64(address, node.num_entries[i]);
                }
                write_string(address, node.suffixes[node.num_children]);
            }
        }

        static void skip_string(uint8_t *&address)
        {
            ZonePbtFormat;

            uint64_t string_size = read_varint64(address);
            address += string_size;
        }

        static void skip_uint64(uint8_t *&address)
        {
            ZonePbtFormat;

            address += sizeof(uint64_t);
        }

        static void skip_varint64(uint8_t *&address)
        {
            ZonePbtFormat;

            while ((*address & 0x80) != 0)
            {
                address++;
            }
            address++;
        }

        static void read_footer(uint8_t *&address, Footer &footer)
        {
            ZonePbtFormat;

            footer.root_offset = *(uint64_t *)address;
            address += sizeof(uint64_t);
            footer.level_0_end = *(uint64_t *)address;
            address += sizeof(uint64_t);
            footer.tree_height = *(uint64_t *)address;
            address += sizeof(uint64_t);
            footer.identifier = *(uint64_t *)address;
            address += sizeof(uint64_t);
            footer.num_entries = *(uint64_t *)address;
            address += sizeof(uint64_t);
            footer.compression = *(uint8_t *)address;
            address += sizeof(uint8_t);
            footer.version_major = *(uint16_t *)address;
            address += sizeof(uint16_t);
            footer.version_minor = *(uint16_t *)address;
            address += sizeof(uint16_t);
            footer.magic = *(uint32_t *)address;
            address += sizeof(uint32_t);
        }

        static uint64_t read_uint64(uint8_t *&address)
        {
            ZonePbtFormat;

            uint64_t value = *(uint64_t *)(address);
            address += sizeof(uint64_t);
            return value;
        }

        static uint64_t read_varint64(uint8_t *&address)
        {
            ZonePbtFormat;

            uint64_t value = 0;
            uint64_t shift = 0;
            while ((*address & 0x80) != 0)
            {
                value |= (*address++ & 0x7F) << shift;
                shift += 7;
            }
            value |= *address++ << shift;
            return value;
        }

        static void read_string(uint8_t *&address, std::string &value)
        {
            ZonePbtFormat;

            uint64_t string_size = read_varint64(address);
            value.assign((char *)address, string_size);
            address += string_size;
        }

        // static void read_string(uint8_t *&address, std::string_view &value)
        // {
        //     ZonePbtFormat;

        //     uint64_t string_size = read_varint64(address);
        //     value = std::string_view((char *)address, string_size);
        //     address += string_size;
        // }

        template <bool compressed>
        static void read_node_leaf(uint8_t *&address, NodeLeaf &node)
        {
            ZonePbtFormat;

            if (compressed)
            {
                std::unique_ptr<uint8_t[]> buffer;
                uint64_t size;
                read_data_compressed(address, buffer, size);
                uint8_t *buffer_address = buffer.get();
                read_node_leaf<false>(buffer_address, node);
            }
            else
            {
                node.num_children = read_varint64(address);
                read_string(address, node.stem);
                node.suffixes.resize(node.num_children);
                node.values.resize(node.num_children);
                for (uint64_t i = 0; i < node.num_children; i++)
                {
                    read_string(address, node.suffixes[i]);
                    read_string(address, node.values[i]);
                }
            }
        }

        template <bool compressed>
        static void read_node_internal(uint8_t *&address, NodeInternal &node)
        {
            ZonePbtFormat;

            if (compressed)
            {
                std::unique_ptr<uint8_t[]> buffer;
                uint64_t size;
                read_data_compressed(address, buffer, size);
                uint8_t *buffer_address = buffer.get();
                read_node_internal<false>(buffer_address, node);
            }
            else
            {
                node.num_children = read_varint64(address);
                read_string(address, node.stem);
                node.suffixes.resize(node.num_children + 1);
                node.reduced_values.resize(node.num_children);
                node.offsets.resize(node.num_children);
                node.num_entries.resize(node.num_children);
                for (uint64_t i = 0; i < node.num_children; i++)
                {
                    read_string(address, node.suffixes[i]);
                    read_string(address, node.reduced_values[i]);
                    node.offsets[i] = read_uint64(address);
                    node.num_entries[i] = read_varint64(address);
                }
                read_string(address, node.suffixes[node.num_children]);
            }
        }
    };
}
