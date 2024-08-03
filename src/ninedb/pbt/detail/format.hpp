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
        static uint64_t write_uint16(void *address, uint16_t value)
        {
            ZonePbtFormat;

            *reinterpret_cast<uint16_t *>(address) = value;
            return sizeof(uint16_t);
        }

        static uint64_t write_uint32(void *address, uint32_t value)
        {
            ZonePbtFormat;

            *reinterpret_cast<uint32_t *>(address) = value;
            return sizeof(uint32_t);
        }

        static uint64_t write_uint64(void *address, uint64_t value)
        {
            ZonePbtFormat;

            *reinterpret_cast<uint64_t *>(address) = value;
            return sizeof(uint64_t);
        }

        static uint64_t write_string_data_only(void *address, std::string_view value)
        {
            ZonePbtFormat;

            memcpy(address, value.data(), value.size());
            return value.size();
        }

        static uint64_t read_uint16(void *address, uint16_t &value)
        {
            ZonePbtFormat;

            value = *reinterpret_cast<uint16_t *>(address);
            return sizeof(uint16_t);
        }

        static uint64_t read_uint32(void *address, uint32_t &value)
        {
            ZonePbtFormat;

            value = *reinterpret_cast<uint32_t *>(address);
            return sizeof(uint32_t);
        }

        static uint64_t read_uint64(void *address, uint64_t &value)
        {
            ZonePbtFormat;

            value = *reinterpret_cast<uint64_t *>(address);
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
    };
}
