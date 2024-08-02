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
        // TODO: take FrameViewLeaf as a parameter instead of node_address

        Iterator(uint8_t *node_address, bool compression_enabled, uint64_t entry_index, uint64_t remaining_entries)
            : compression_enabled(compression_enabled), entry_index(entry_index), remaining_entries(remaining_entries)
        {
            ZonePbtIterator;

            if (node_address != nullptr)
            {
                leaf = detail::FrameViewLeaf(node_address, compression_enabled);
            }

            if (remaining_entries >= 1)
            {
                current_num_children = detail::NodeLeaf::read_num_children((uint8_t *)leaf.frame_view.get_view().data());
            }
        }

        /**
         * Get the key at the current position.
         */
        std::string_view get_key() const
        {
            ZonePbtIterator;

            return detail::NodeLeaf::read_key((uint8_t *)leaf.frame_view.get_view().data(), entry_index);
        }

        /**
         * Get the value at the current position.
         */
        std::string_view get_value() const
        {
            ZonePbtIterator;

            return detail::NodeLeaf::read_value((uint8_t *)leaf.frame_view.get_view().data(), entry_index);
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
                    leaf = detail::FrameViewLeaf((uint8_t *)leaf.frame_view.original.data() + leaf.frame_view.original.size(), compression_enabled);
                    current_num_children = detail::NodeLeaf::read_num_children((uint8_t *)leaf.frame_view.get_view().data());
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
        bool compression_enabled;
        detail::FrameViewLeaf leaf;
        uint64_t entry_index;
        uint64_t remaining_entries;
        uint64_t current_num_children;
    };
}
