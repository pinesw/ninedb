#pragma once

#include <array>
#include <list>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "./profiling.hpp"

namespace ninedb::detail
{
    template <typename K, typename V>
    class RLRUCache
    {
    public:
        typedef typename std::tuple<K, V, uint64_t> array_item;

        RLRUCache(uint64_t max_size)
            : max_size(max_size)
        {
            stride = std::max<uint64_t>(2ull, max_size / sample_size);
            cache_items_array.resize(max_size);
        }

        void put(const K &key, const V &value)
        {
            ZoneLruCache;

            auto it = cache_items_map.find(key);
            if (it != cache_items_map.end())
            {
                std::get<1>(cache_items_array[it->second]) = value;
                std::get<2>(cache_items_array[it->second]) = monotonic_time++;
                return;
            }

            if (cache_items_map.size() < max_size)
            {
                uint64_t index = cache_items_map.size();
                cache_items_array[index] = std::make_tuple(key, value, monotonic_time++);
                cache_items_map[key] = index;
                return;
            }

            uint64_t evict_index = evict();
            cache_items_map.erase(std::get<0>(cache_items_array[evict_index]));
            cache_items_array[evict_index] = std::make_tuple(key, value, monotonic_time++);
            cache_items_map[key] = evict_index;
        }

        bool try_get(const K &key, V &value)
        {
            ZoneLruCache;

            auto it = cache_items_map.find(key);
            if (it == cache_items_map.end())
            {
                return false;
            }
            else
            {
                std::get<2>(cache_items_array[it->second]) = monotonic_time++;
                value = std::get<1>(cache_items_array[it->second]);
                return true;
            }
        }

        bool exists(const K &key) const
        {
            ZoneLruCache;

            return cache_items_map.find(key) != cache_items_map.end();
        }

    private:
        static constexpr uint64_t sample_size = 8;

        uint64_t max_size;
        uint64_t stride;
        uint64_t monotonic_time = 0;
        uint64_t evict_sample_offset = 0;
        std::vector<array_item> cache_items_array;
        std::unordered_map<K, uint64_t> cache_items_map;

        uint64_t evict()
        {
            uint64_t min_index = 0;
            uint64_t min_time = std::numeric_limits<uint64_t>::max();
            for (uint64_t i = 0; i < sample_size; i++)
            {
                uint64_t index = (evict_sample_offset + i * stride) % max_size;
                if (std::get<2>(cache_items_array[index]) < min_time)
                {
                    min_index = index;
                    min_time = std::get<2>(cache_items_array[index]);
                }
            }
            evict_sample_offset++;
            return min_index;
        }
    };
}
