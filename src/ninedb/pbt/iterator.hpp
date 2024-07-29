#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "./detail/format.hpp"
#include "./detail/storage.hpp"
#include "./detail/structures.hpp"

namespace ninedb::pbt
{
    struct Iterator
    {
        Iterator(const Iterator &other) : current_leaf(nullptr)
        {
            ZonePbtIterator;

            storage = other.storage;
            current_leaf = detail::NodeLeafRead(other.current_offset, *storage);
            current_index = other.current_index;
            current_offset = other.current_offset;
            next_offset = other.next_offset;
            end_offset = other.end_offset;
        }

        Iterator(const std::shared_ptr<detail::Storage> &storage, uint64_t index, uint64_t offset, uint64_t end_offset)
            : storage(storage), current_leaf(nullptr), current_index(index), end_offset(end_offset)
        {
            ZonePbtIterator;

            next_offset = current_offset = offset;

            if (offset < end_offset)
            {
                current_leaf.set_offset(offset, *storage);
                next_offset += current_leaf.size_of();
            }
        }

        /**
         * Get the key at the current position.
         */
        std::string get_key() const
        {
            ZonePbtIterator;

            return std::string(current_leaf.key(current_index));
        }

        /**
         * Get the value at the current position.
         */
        std::string get_value() const
        {
            ZonePbtIterator;

            return std::string(current_leaf.value(current_index));
        }

        /**
         * Move the iterator to the next key-value pair.
         */
        void next()
        {
            ZonePbtIterator;

            current_index++;
            if (current_index >= current_leaf.num_children())
            {
                current_index = 0;
                current_offset = next_offset;
                if (current_offset < end_offset)
                {
                    current_leaf.set_offset(current_offset, *storage);
                    next_offset += current_leaf.size_of();
                }
            }
        }

        /**
         * Check if the iterator is at the end.
         */
        bool is_end() const
        {
            ZonePbtIterator;

            return current_offset >= end_offset;
        }

    private:
        std::shared_ptr<detail::Storage> storage;
        detail::NodeLeafRead current_leaf;
        uint64_t current_index;
        uint64_t current_offset;
        uint64_t next_offset;
        uint64_t end_offset;
        // uint8_t *current_address;
        // uint8_t *next_address;
        // uint8_t *end_address;
    };
}
