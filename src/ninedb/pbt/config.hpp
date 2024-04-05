#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ninedb::pbt
{
    struct WriterConfig
    {
        /**
         * The maximum number of entries in a node.
         */
        uint64_t max_node_entries = 16;

        /**
         * The initial size of the PBT file.
         */
        uint64_t initial_pbt_size = 1 << 23;

        /**
         * Whether or not to use LZ4 compression.
         * Reduces the size of the PBT file, at a slight cost to read/write performance.
         */
        bool enable_compression = false;

        /**
         * Whether or not to use prefix encoding.
         * Reduces the size of the PBT file, at a slight cost to read/write performance.
         */
        bool enable_prefix_encoding = false;

        /**
         * A function that reduces a list of values to a single value.
         * This is used to reduce the values of a node's child entries to a single value.
         * The reduced value is then stored in the parent node.
         * This can be used to implement fast queries on aggregated data.
         */
        std::function<void(const std::vector<std::string> &values, std::string &reduced_value)> reduce = nullptr;

        /**
         * If true, an error is thrown if the file already exists.
         */
        bool error_if_exists = false;
    };

    struct ReaderConfig
    {
        /**
         * Size of the internal node cache.
         * If 0, the cache is disabled.
         */
        uint64_t internal_node_cache_size = 64;

        /**
         * Size of the leaf node cache.
         * If 0, the cache is disabled.
         */
        uint64_t leaf_node_cache_size = 8;
    };
}
