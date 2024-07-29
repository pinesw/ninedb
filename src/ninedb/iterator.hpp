#pragma once

#include <memory>
#include <vector>

#include "./pbt/iterator.hpp"
#include "./pbt/reader.hpp"

namespace ninedb
{
    struct Iterator
    {
        bool is_end() const
        {
            return itrs[current].is_end();
        }

        std::string_view get_key() const
        {
            return keys[current];
        }

        void get_key(std::string &key) const
        {
            key = keys[current];
        }

        std::string_view get_value() const
        {
            return itrs[current].get_value();
        }

        void get_value(std::string &value) const
        {
            value = itrs[current].get_value();
        }

        void next()
        {
            itrs[current].next();
            if (!itrs[current].is_end())
            {
                keys[current] = itrs[current].get_key();
            }
            current = get_min_index();
        }

        Iterator(std::vector<pbt::Iterator> &&itrs)
            : itrs(std::move(itrs))
        {
            keys.resize(this->itrs.size());
            for (uint64_t i = 0; i < this->itrs.size(); i++)
            {
                if (!this->itrs[i].is_end())
                {
                    keys[i] = this->itrs[i].get_key();
                }
            }
            current = get_min_index();
        }

    private:
        std::vector<pbt::Iterator> itrs;
        std::vector<std::string> keys;
        uint64_t current;

        uint64_t get_min_index() const
        {
            uint64_t next = 0;
            for (uint64_t i = 1; i < itrs.size(); i++)
            {
                if (itrs[i].is_end())
                {
                    continue;
                }
                if (itrs[next].is_end() || keys[i].compare(keys[next]) < 0)
                {
                    next = i;
                }
            }
            return next;
        }
    };
}
