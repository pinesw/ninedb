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
        Iterator(const std::shared_ptr<detail::Storage> &storage, uint64_t index, uint64_t offset, uint64_t end_offset)
            : storage(storage), current_leaf(nullptr), current_index(index)
        {
            ZonePbtIterator;

            next_address = current_address = (uint8_t *)storage->get_address() + offset;
            end_address = (uint8_t *)storage->get_address() + end_offset;

            if (offset < end_offset)
            {
                current_leaf.address = next_address;
                next_address += current_leaf.size_of();
            }
        }

        /**
         * Get the key at the current position.
         */
        std::string_view get_key() const
        {
            ZonePbtIterator;

            return current_leaf.key(current_index);
        }

        /**
         * Get the value at the current position.
         */
        std::string_view get_value() const
        {
            ZonePbtIterator;

            return current_leaf.value(current_index);
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
                current_address = next_address;
                if (current_address < end_address)
                {
                    current_leaf.address = current_address;
                    next_address += current_leaf.size_of();
                }
            }
        }

        /**
         * Check if the iterator is at the end.
         */
        bool is_end() const
        {
            ZonePbtIterator;

            return current_address >= end_address;
        }

    private:
        std::shared_ptr<detail::Storage> storage;
        detail::NodeLeafShallow current_leaf;
        uint64_t current_index;
        uint8_t *current_address;
        uint8_t *next_address;
        uint8_t *end_address;
    };
}
