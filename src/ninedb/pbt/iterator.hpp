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
        Iterator(uint8_t *node_address, uint64_t entry_index, uint64_t global_index, uint64_t global_end)
            : node_address(node_address), entry_index(entry_index), global_index(global_index), global_end(global_end)
        {
            ZonePbtIterator;

            if (global_index < global_end)
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
            global_index++;
            if (entry_index >= current_num_children)
            {
                entry_index = 0;
                if (global_index < global_end)
                {
                    node_address += detail::NodeLeaf::size_of(node_address);
                }
            }
        }

        /**
         * Check if the iterator is at the end.
         */
        bool is_end() const
        {
            ZonePbtIterator;

            return global_index >= global_end;
        }

    private:
        std::shared_ptr<detail::Storage> storage;
        uint64_t entry_index;
        uint64_t global_index;
        uint64_t global_end;
        uint64_t current_num_children;
        uint8_t *node_address;
    };
}
