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
        Iterator(const std::shared_ptr<detail::Storage> &storage, bool is_compressed, uint64_t index, uint64_t offset, uint64_t end_offset)
            : storage(storage), current_leaf(std::make_shared<detail::NodeLeafShallow>()), is_compressed(is_compressed), current_index(index)
        {
            ZonePbtIterator;

            next_address = current_address = (uint8_t *)storage->get_address() + offset;
            end_address = (uint8_t *)storage->get_address() + end_offset;

            if (offset < end_offset)
            {
                if (is_compressed)
                {
                    detail::Format::read_node_leaf_compressed(next_address, *current_leaf);
                }
                else
                {
                    detail::Format::read_node_leaf_uncompressed(next_address, *current_leaf);
                }
            }
        }

        /**
         * Get the key at the current position.
         */
        void get_key(std::string &key) const
        {
            ZonePbtIterator;

            std::string_view suffix = current_leaf->suffix(current_index);
            key.resize(current_leaf->stem.size() + suffix.size());
            key.replace(0, current_leaf->stem.size(), current_leaf->stem);
            key.replace(current_leaf->stem.size(), suffix.size(), suffix);
        }

        /**
         * Get the key at the current position.
         */
        std::string get_key() const
        {
            ZonePbtIterator;

            std::string key;
            get_key(key);
            return key;
        }

        /**
         * Get the value at the current position.
         */
        void get_value(std::string_view &value) const
        {
            ZonePbtIterator;

            value = current_leaf->value(current_index);
        }

        /**
         * Get the value at the current position.
         */
        std::string_view get_value() const
        {
            ZonePbtIterator;

            return current_leaf->value(current_index);
        }

        /**
         * Move the iterator to the next key-value pair.
         */
        void next()
        {
            ZonePbtIterator;

            current_index++;
            if (current_index >= current_leaf->num_children)
            {
                current_index = 0;
                current_address = next_address;
                if (current_address < end_address)
                {
                    if (is_compressed)
                    {
                        detail::Format::read_node_leaf_compressed(next_address, *current_leaf);
                    }
                    else
                    {
                        detail::Format::read_node_leaf_uncompressed(next_address, *current_leaf);
                    }
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
        std::shared_ptr<detail::NodeLeafShallow> current_leaf;
        bool is_compressed;
        uint64_t current_index;
        uint8_t *current_address;
        uint8_t *next_address;
        uint8_t *end_address;
    };
}
