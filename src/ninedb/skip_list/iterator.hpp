#pragma once

#include <list>
#include <memory>
#include <string>
#include <string_view>

#include "./detail/structure.hpp"

namespace ninedb::skip_list
{
    struct Iterator
    {
    private:
        typedef typename std::list<detail::Entry>::iterator list_iterator;

        list_iterator it;

    public:
        Iterator(list_iterator it) : it(it) {}

        /**
         * Get the key of the current entry.
         */
        void get_key(std::string_view &key) const
        {
            key = *it->key;
        }

        /**
         * Get the key of the current entry.
         */
        void get_key(std::string &key) const
        {
            key = *it->key;
        }

        /**
         * Get the value of the current entry.
         */
        void get_value(std::string_view &value) const
        {
            value = *std::get<std::shared_ptr<std::string>>(it->value);
        }

        /**
         * Advance the iterator to the next entry.
         */
        Iterator &operator++()
        {
            ++it;
            return *this;
        }

        /**
         * Returns true if the two iterators point to the same entry.
         */
        friend bool operator==(const Iterator &a, const Iterator &b)
        {
            return a.it == b.it;
        }

        /**
         * Returns true if the two iterators point to different entries.
         */
        friend bool operator!=(const Iterator &a, const Iterator &b)
        {
            return a.it != b.it;
        }
    };
}
