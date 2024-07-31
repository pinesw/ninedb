#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <lz4.h>

#include "../../detail/profiling.hpp"

namespace ninedb::pbt::detail
{
    // TODO: replace all uint8_t * with void *

    struct Format
    {
        static uint64_t write_uint8(uint8_t *address, uint8_t value)
        {
            ZonePbtFormat;

            *(uint8_t *)(address) = value;
            return sizeof(uint8_t);
        }

        static uint64_t write_uint16(uint8_t *address, uint16_t value)
        {
            ZonePbtFormat;

            *(uint16_t *)(address) = value;
            return sizeof(uint16_t);
        }

        static uint64_t write_uint32(uint8_t *address, uint32_t value)
        {
            ZonePbtFormat;

            *(uint32_t *)(address) = value;
            return sizeof(uint32_t);
        }

        static uint64_t write_uint64(uint8_t *address, uint64_t value)
        {
            ZonePbtFormat;

            *(uint64_t *)(address) = value;
            return sizeof(uint64_t);
        }

        static uint64_t write_string_data_only(uint8_t *address, std::string_view value)
        {
            ZonePbtFormat;

            memcpy(address, value.data(), value.size());
            return value.size();
        }

        static uint64_t compressed_frame_bound(uint64_t size)
        {
            return 2 * sizeof(uint64_t) + (uint64_t)LZ4_compressBound((int)size);
        }

        static uint64_t write_data_compressed(uint8_t *address, std::string_view value)
        {
            ZonePbtFormat;

            std::string buffer_compressed = std::string(LZ4_compressBound((int)value.size()), 0);
            uint64_t compressed_size = LZ4_compress_default((char *)value.data(), (char *)buffer_compressed.data(), (int)value.size(), (int)buffer_compressed.size());
            address += write_uint64(address, compressed_size);
            address += write_uint64(address, value.size());
            memcpy(address, buffer_compressed.data(), compressed_size);

            return 2 * sizeof(uint64_t) + compressed_size;
        }

        static uint64_t read_data_compressed(const uint8_t *address, std::string &buffer)
        {
            ZonePbtFormat;

            uint64_t compressed_size;
            uint64_t size_uncompressed;

            address += read_uint64(address, compressed_size);
            address += read_uint64(address, size_uncompressed);
            buffer.resize(size_uncompressed);
            LZ4_decompress_safe((char *)address, (char *)buffer.data(), (int)compressed_size, (int)buffer.size());

            return 2 * sizeof(uint64_t) + compressed_size;
        }

        static uint64_t compressed_frame_size(const uint8_t *address)
        {
            ZonePbtFormat;

            uint64_t compressed_size;
            read_uint64(address, compressed_size);

            return 2 * sizeof(uint64_t) + compressed_size;
        }

        static uint64_t read_uint8(const uint8_t *address, uint8_t &value)
        {
            ZonePbtFormat;

            value = *(uint8_t *)(address);
            return sizeof(uint8_t);
        }

        static uint64_t read_uint16(const uint8_t *address, uint16_t &value)
        {
            ZonePbtFormat;

            value = *(uint16_t *)(address);
            return sizeof(uint16_t);
        }

        static uint64_t read_uint32(const uint8_t *address, uint32_t &value)
        {
            ZonePbtFormat;

            value = *(uint32_t *)(address);
            return sizeof(uint32_t);
        }

        static uint64_t read_uint64(const uint8_t *address, uint64_t &value)
        {
            ZonePbtFormat;

            value = *(uint64_t *)(address);
            return sizeof(uint64_t);
        }

        static constexpr uint64_t size_uint16(uint64_t count = 1)
        {
            return count * sizeof(uint16_t);
        }

        static constexpr uint64_t size_uint32(uint64_t count = 1)
        {
            return count * sizeof(uint32_t);
        }

        static constexpr uint64_t size_uint64(uint64_t count = 1)
        {
            return count * sizeof(uint64_t);
        }

        // static void write_varint64(uint8_t *&address, uint64_t value)
        // {
        //     ZonePbtFormat;

        //     while (value >= 0x80)
        //     {
        //         *address++ = static_cast<uint8_t>(value) | 0x80;
        //         value >>= 7;
        //     }
        //     *address++ = static_cast<uint8_t>(value);
        // }

        // static void write_string(uint8_t *&address, std::string_view value)
        // {
        //     ZonePbtFormat;

        //     write_varint64(address, value.size());
        //     memcpy(address, value.data(), value.size());
        //     address += value.size();
        // }

        // static uint64_t read_varint64(uint8_t *&address)
        // {
        //     ZonePbtFormat;

        //     uint64_t value = 0;
        //     uint64_t shift = 0;
        //     while ((*address & 0x80) != 0)
        //     {
        //         value |= (*address++ & 0x7F) << shift;
        //         shift += 7;
        //     }
        //     value |= *address++ << shift;
        //     return value;
        // }

        // static void read_string(uint8_t *&address, std::string &value)
        // {
        //     ZonePbtFormat;

        //     uint64_t string_size = read_varint64(address);
        //     value.assign((char *)address, string_size);
        //     address += string_size;
        // }

        // static void skip_varint64(uint8_t *&address)
        // {
        //     ZonePbtFormat;

        //     while ((*address & 0x80) != 0)
        //     {
        //         address++;
        //     }
        //     address++;
        // }
    };

    struct FrameView
    {
        std::string buffer;
        std::string_view original;

        FrameView() {}

        FrameView(uint8_t *address, uint64_t size, bool compression_enabled)
        {
            ZonePbtStructures;

            original = std::string_view((char *)address, size);

            if (compression_enabled)
            {
                uint64_t compressed_size;
                uint64_t size_uncompressed;

                address += Format::read_uint64(address, compressed_size);
                address += Format::read_uint64(address, size_uncompressed);
                buffer.resize(size_uncompressed);
                LZ4_decompress_safe((char *)address, buffer.data(), (int)compressed_size, (int)buffer.size());
            }
        }

        std::string_view get_view() const
        {
            if (buffer.empty())
            {
                return original;
            }
            else
            {
                return buffer;
            }
        }
    };
}
