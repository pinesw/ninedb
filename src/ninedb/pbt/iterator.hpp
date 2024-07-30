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
            : storage(storage), current_index(index)
        {
            ZonePbtIterator;

            next_address = current_address = (uint8_t *)storage->get_address() + offset;
            end_address = (uint8_t *)storage->get_address() + end_offset;

            if (offset < end_offset)
            {
                next_address += detail::NodeLeaf::size_of(current_address);
                current_num_children = detail::NodeLeaf::read_num_children(current_address);
            }
        }

        /**
         * Get the key at the current position.
         */
        std::string_view get_key() const
        {
            ZonePbtIterator;

            return detail::NodeLeaf::read_key(current_address, current_index);
        }

        /**
         * Get the value at the current position.
         */
        std::string_view get_value() const
        {
            ZonePbtIterator;

            return detail::NodeLeaf::read_value(current_address, current_index);
        }

        /**
         * Move the iterator to the next key-value pair.
         */
        void next()
        {
            ZonePbtIterator;

            current_index++;
            if (current_index >= current_num_children)
            {
                current_index = 0;
                current_address = next_address;
                if (current_address < end_address)
                {
                    next_address += detail::NodeLeaf::size_of(current_address);
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
        uint64_t current_index;
        uint64_t current_num_children;
        uint8_t *current_address;
        uint8_t *next_address;
        uint8_t *end_address;
    };
}
