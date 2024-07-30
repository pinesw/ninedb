#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "./detail/structures.hpp"

namespace ninedb::pbt
{
    struct Iterator
    {
        Iterator(uint8_t *node_address, uint64_t entry_index, uint64_t remaining_entries)
            : node_address(node_address), entry_index(entry_index), remaining_entries(remaining_entries)
        {
            ZonePbtIterator;

            if (remaining_entries >= 1)
            {
                current_num_children = detail::NodeLeaf::read_num_children(node_address);
            }
        }

        /**
         * Get the key at the current position.
         */
        std::string_view get_key() const
        {
            ZonePbtIterator;

            return detail::NodeLeaf::read_key(node_address, entry_index);
        }

        /**
         * Get the value at the current position.
         */
        std::string_view get_value() const
        {
            ZonePbtIterator;

            return detail::NodeLeaf::read_value(node_address, entry_index);
        }

        /**
         * Move the iterator to the next key-value pair.
         */
        void next()
        {
            ZonePbtIterator;

            entry_index++;
            remaining_entries--;
            if (entry_index >= current_num_children)
            {
                entry_index = 0;
                if (remaining_entries >= 1)
                {
                    node_address += detail::NodeLeaf::size_of(node_address);
                    current_num_children = detail::NodeLeaf::read_num_children(node_address);
                }
            }
        }

        /**
         * Check if the iterator is at the end.
         */
        bool is_end() const
        {
            ZonePbtIterator;

            return remaining_entries <= 0;
        }

    private:
        uint8_t *node_address;
        uint64_t entry_index;
        uint64_t remaining_entries;
        uint64_t current_num_children;
    };
}
