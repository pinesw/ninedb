#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>

#include "./profiling.hpp"

namespace ninedb::detail
{
    struct Buffer
    {
        typedef typename std::multimap<std::string, std::string> buffer_map;

        void insert(std::string_view key, std::string_view value)
        {
            ZoneBuffer;

            size += key.size() + value.size();
            map.insert(buffer_map::value_type(key, value));
        }

        void clear()
        {
            ZoneBuffer;

            map.clear();
            size = 0;
        }

        uint64_t get_size() const
        {
            ZoneBuffer;

            return size;
        }

        buffer_map::iterator begin()
        {
            ZoneBuffer;

            return map.begin();
        }

        buffer_map::iterator end()
        {
            ZoneBuffer;

            return map.end();
        }

        uint64_t get_count() const
        {
            ZoneBuffer;

            return map.size();
        }

    private:
        buffer_map map;
        uint64_t size = 0;
    };
}
