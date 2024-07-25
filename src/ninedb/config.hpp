#pragma once

#include <cstdint>

#include "./pbt/config.hpp"

namespace ninedb
{
    struct Config
    {
        /**
         * The maximum size of the in-memory write buffer in bytes.
         * When the buffer size exceeds this value, the buffer will be flushed to disk.
         */
        uint64_t max_buffer_size = 1 << 22;

        /**
         * The maximum number of PBTs in a level before a merge operation is triggered.
         */
        uint64_t max_level_count = 10;

        /**
         * The config for writers of the db.
         */
        pbt::WriterConfig writer = {
            16,
            1 << 23,
            nullptr,
            false,
        };

        /**
         * If true, an error will be thrown if the db already exists.
         */
        bool error_if_exists = false;

        /**
         * If true, the db will be deleted if it already exists.
         */
        bool delete_if_exists = false;

        /**
         * If true, the db will be created if it does not exist.
         */
        bool create_if_missing = true;
    };
}
