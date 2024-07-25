#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <lz4.h>

#include "../../detail/profiling.hpp"

namespace ninedb::pbt::detail
{
    struct Format
    {
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

        static uint64_t write_string_data_only(uint8_t *address, std::string_view value)
        {
            ZonePbtFormat;

            memcpy(address, value.data(), value.size());
            return value.size();
        }

        // static void write_data_compressed(uint8_t *&address, const uint8_t *data, uint64_t size)
        // {
        //     ZonePbtFormat;

        //     uint64_t max_size_compressed = LZ4_compressBound((int)size);
        //     std::unique_ptr<uint8_t[]> buffer_compressed = std::make_unique<uint8_t[]>(max_size_compressed);
        //     uint64_t compressed_size = LZ4_compress_default((char *)data, (char *)buffer_compressed.get(), (int)size, (int)max_size_compressed);
        //     write_varint64(address, compressed_size);
        //     write_varint64(address, size);
        //     memcpy(address, buffer_compressed.get(), compressed_size);
        //     address += compressed_size;
        // }

        // static void read_data_compressed(uint8_t *&address, std::unique_ptr<uint8_t[]> &buffer, uint64_t &size)
        // {
        //     ZonePbtFormat;

        //     uint64_t compressed_size = read_varint64(address);
        //     uint64_t size_uncompressed = read_varint64(address);
        //     buffer = std::make_unique<uint8_t[]>(size_uncompressed);
        //     LZ4_decompress_safe((char *)address, (char *)buffer.get(), (int)compressed_size, (int)size_uncompressed);
        //     size = size_uncompressed;
        //     address += compressed_size;
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

        static uint64_t read_uint16(uint8_t *address, uint16_t &value)
        {
            ZonePbtFormat;

            value = *(uint16_t *)(address);
            return sizeof(uint16_t);
        }

        static uint64_t read_uint32(uint8_t *address, uint32_t &value)
        {
            ZonePbtFormat;

            value = *(uint32_t *)(address);
            return sizeof(uint32_t);
        }

        static uint64_t read_uint64(uint8_t *address, uint64_t &value)
        {
            ZonePbtFormat;

            value = *(uint64_t *)(address);
            return sizeof(uint64_t);
        }

        static constexpr uint64_t skip_uint16(uint64_t count = 1)
        {
            return count * sizeof(uint16_t);
        }

        static constexpr uint64_t skip_uint32(uint64_t count = 1)
        {
            return count * sizeof(uint32_t);
        }

        static constexpr uint64_t skip_uint64(uint64_t count = 1)
        {
            return count * sizeof(uint64_t);
        }

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
    };
}
