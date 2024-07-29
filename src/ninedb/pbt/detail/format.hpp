#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <lz4.h>

#include "../../detail/profiling.hpp"
#include "./storage.hpp"

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

        static uint64_t write_string_data_only(uint8_t *address, std::string_view value)
        {
            ZonePbtFormat;

            memcpy(address, value.data(), value.size());
            return value.size();
        }

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

        static uint64_t read_uint16(StorageMmap &storage, uint64_t offset, uint16_t &value)
        {
            ZonePbtFormat;

            storage.read(offset, sizeof(uint16_t), reinterpret_cast<uint8_t *>(&value));
            return sizeof(uint16_t);
        }

        static uint64_t read_uint32(StorageMmap &storage, uint64_t offset, uint32_t &value)
        {
            ZonePbtFormat;

            storage.read(offset, sizeof(uint32_t), reinterpret_cast<uint8_t *>(&value));
            return sizeof(uint32_t);
        }

        static uint64_t read_uint64(StorageMmap &storage, uint64_t offset, uint64_t &value)
        {
            ZonePbtFormat;

            storage.read(offset, sizeof(uint64_t), reinterpret_cast<uint8_t *>(&value));
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
