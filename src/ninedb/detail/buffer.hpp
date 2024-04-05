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

        template <typename K, typename V>
        void insert(K &&key, V &&value)
        {
            static_assert(ninedb::detail::is_string_kind_v<K>, "key must be a string");
            static_assert(ninedb::detail::is_string_kind_v<V>, "value must be a string");

            ZoneBuffer;

            size += std::string_view(key).size() + std::string_view(value).size();
            map.insert(buffer_map::value_type(std::forward<K>(key), std::forward<V>(value)));
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

    private:
        buffer_map map;
        uint64_t size = 0;
    };
}
