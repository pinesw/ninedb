#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../detail/rlru_cache.hpp"
#include "../detail/profiling.hpp"

#include "./detail/storage.hpp"
#include "./detail/structures.hpp"

#include "./iterator.hpp"

namespace ninedb::pbt
{
    enum ReaderFindMode
    {
        EXACT,
        GREATER_OR_EQUAL,
    };

    struct Reader
    {
        Reader(const std::string &path)
            : Reader(std::make_shared<detail::Storage>(path, true)) {}

        Reader(const std::shared_ptr<detail::Storage> &storage)
            : storage(storage), leaf_cache(64), internal_cache(64)
        {
            read_footer();
        }

        uint64_t get_global_start() const
        {
            ZonePbtReader;

            return footer.global_start;
        }

        /**
         * Get the value for the given key.
         * Returns true if the key exists, false otherwise.
         * If the key exists, the value is written to the given string.
         * If the key occurs multiple times, the value of the first occurrence is written.
         */
        bool get(std::string_view key, std::string &value)
        {
            ZonePbtReader;

            if (footer.tree_height == 0)
            {
                return false;
            }

            uint64_t entry_index;
            std::shared_ptr<detail::FrameViewLeaf> leaf;

            if (!find<EXACT>(key, leaf, entry_index, nullptr))
            {
                return false;
            }

            uint8_t *node_leaf_address = (uint8_t *)leaf->frame_view.get_view().data();
            value = detail::NodeLeaf::read_value(node_leaf_address, entry_index);
            return true;
        }

        /**
         * Get the value for the given key.
         */
        std::optional<std::string> get(std::string_view key)
        {
            ZonePbtReader;

            std::string value;
            if (get(key, value))
            {
                return value;
            }
            return std::nullopt;
        }

        /**
         * Get the key and value at the given index.
         * Returns true if the index is in bounds, false otherwise.
         * If the index is in bounds, the key and value are written to the given strings.
         */
        bool at(uint64_t index, std::string &key, std::string &value)
        {
            ZonePbtReader;

            if (footer.tree_height == 0)
            {
                return false;
            }

            uint64_t entry_index;
            std::shared_ptr<detail::FrameViewLeaf> leaf; leaf;

            if (!find_index(index, leaf, entry_index))
            {
                return false;
            }

            uint8_t *node_leaf_address = (uint8_t *)leaf->frame_view.get_view().data();
            key = detail::NodeLeaf::read_key(node_leaf_address, entry_index);
            value = detail::NodeLeaf::read_value(node_leaf_address, entry_index);
            return true;
        }

        /**
         * Get the key and value at the given index.
         * The first element of the pair is the key, the second element is the value.
         */
        std::optional<std::pair<std::string, std::string>> at(uint64_t index)
        {
            ZonePbtReader;

            std::string key;
            std::string value;
            if (at(index, key, value))
            {
                return std::make_pair(key, value);
            }
            return std::nullopt;
        }

        /**
         * Return an iterator over the key-value pairs in the PBT.
         * The iterator is positioned at the first key-value pair with a key equal to the given key.
         * If such a key does not exist, the iterator is positioned at the first key-value pair with a key greater than the given key.
         * If such a key does not exist, the iterator is positioned at the end.
         */
        Iterator seek_first(std::string_view key)
        {
            ZonePbtReader;

            if (footer.tree_height == 0)
            {
                return end();
            }

            uint64_t entry_index;
            uint64_t entry_start = 0;
            std::shared_ptr<detail::FrameViewLeaf> leaf;

            if (!find<GREATER_OR_EQUAL>(key, leaf, entry_index, &entry_start))
            {
                return end();
            }

            uint8_t *node_leaf_address = (uint8_t *)leaf->frame_view.original.data();
            return Iterator(node_leaf_address, footer.enable_lz4_compression, entry_index, footer.global_end - footer.global_start - entry_start);
        }

        /**
         * Return an iterator over the key-value pairs in the PBT.
         * The iterator is positioned at the last key-value pair with a key equal to the given key.
         * If such a key does not exist, the iterator is positioned at the first key-value pair with a key greater than the given key.
         * If such a key does not exist, the iterator is positioned at the end.
         */
        Iterator seek_last(std::string_view key)
        {
            Iterator it = seek_first(key);
            if (it.is_end())
            {
                return it;
            }
            Iterator next = it;
            while (true)
            {
                next.next();
                if (next.is_end() || next.get_key().compare(key) > 0)
                {
                    return it;
                }
                it.next();
            }
        }

        /**
         * Return an iterator over the key-value pairs in the PBT.
         * The iterator is positioned at the first key-value pair with a key greater than the given key.
         * If such a key does not exist, the iterator is positioned at the end.
         */
        Iterator seek_next(std::string_view key)
        {
            Iterator it = seek_last(key);
            if (it.is_end())
            {
                return it;
            }
            if (it.get_key().compare(key) == 0)
            {
                it.next();
            }
            return it;
        }

        /**
         * Return an iterator over the key-value pairs in the PBT.
         * The iterator is positioned at the last key-value pair with a key less than the given key.
         * If such a key does not exist, the iterator is positioned at the end.
         */
        Iterator seek_prev(std::string_view key)
        {
            throw std::runtime_error("Not implemented");
        }

        /**
         * Return an iterator over the key-value pairs in the PBT.
         * The iterator is positioned at the given index.
         * If the index is out of bounds, the iterator is positioned at the end.
         */
        Iterator seek(uint64_t index)
        {
            ZonePbtReader;

            if (footer.tree_height == 0)
            {
                return end();
            }

            uint64_t entry_index;
            std::shared_ptr<detail::FrameViewLeaf> leaf;

            if (!find_index(index, leaf, entry_index))
            {
                return end();
            }

            uint8_t *node_leaf_address = (uint8_t *)leaf->frame_view.original.data();
            return Iterator(node_leaf_address, footer.enable_lz4_compression, entry_index, footer.global_end - footer.global_start - index);
        }

        /**
         * Return an iterator over the key-value pairs in the PBT.
         * The iterator is positioned at the first key-value pair.
         */
        Iterator begin() const
        {
            ZonePbtReader;

            return Iterator(offset_to_address(0), footer.enable_lz4_compression, 0, footer.global_end - footer.global_start);
        }

        /**
         * Return an iterator over the key-value pairs in the PBT.
         * The iterator is positioned at the end of the PBT.
         */
        Iterator end() const
        {
            ZonePbtReader;

            return Iterator(nullptr, footer.enable_lz4_compression, 0, 0);
        }

        /**
         * Visits all the nodes in the PBT in order.
         * At the leaf nodes, values tested positively against the predicate are added to the accumulator.
         * Internal nodes are also tested against the predicate on their reduced values, and their subtrees are skipped if the predicate returns false.
         */
        void traverse(const std::function<bool(std::string_view value)> &predicate, std::vector<std::string_view> &accumulator)
        {
            ZonePbtReader;

            if (footer.tree_height == 0)
            {
                return;
            }

            traverse(predicate, accumulator, footer.root_offset, footer.tree_height);
        }

        /**
         * Get the number of key-value pairs in the PBT.
         */
        uint64_t count() const
        {
            ZonePbtReader;

            return footer.global_end - footer.global_start;
        }

    private:
        detail::Footer footer;
        std::shared_ptr<detail::Storage> storage;
        ninedb::detail::RLRUCache<uint64_t, std::shared_ptr<detail::FrameViewLeaf>> leaf_cache;
        ninedb::detail::RLRUCache<uint64_t, std::shared_ptr<detail::FrameViewInternal>> internal_cache;

        void traverse(const std::function<bool(std::string_view value)> &predicate, std::vector<std::string_view> &accumulator, uint64_t offset, uint64_t height)
        {
            ZonePbtReader;

            if (height >= 2)
            {
                detail::FrameViewInternal internal(offset_to_address(offset), footer.enable_lz4_compression);
                uint8_t *node_internal_address = (uint8_t *)internal.frame_view.get_view().data();

                uint64_t num_children = detail::NodeInternal::read_num_children(node_internal_address);

                for (uint64_t i = 0; i < num_children; i++)
                {
                    if (predicate(detail::NodeInternal::read_reduced_value(node_internal_address, i)))
                    {
                        traverse(predicate, accumulator, detail::NodeInternal::read_child_offset(node_internal_address, i), height - 1);
                    }
                }
            }
            else
            {
                uint8_t *node_leaf_address = offset_to_address(offset);
                uint64_t num_children = detail::NodeLeaf::read_num_children(node_leaf_address);

                for (uint64_t i = 0; i < num_children; i++)
                {
                    std::string_view value = detail::NodeLeaf::read_value(node_leaf_address, i);

                    if (predicate(value))
                    {
                        accumulator.push_back(value);
                    }
                }
            }
        }

        bool find_index(uint64_t index, std::shared_ptr<detail::FrameViewLeaf> &leaf, uint64_t &entry_index)
        {
            ZonePbtReader;

            uint64_t offset = footer.root_offset;
            uint64_t height = footer.tree_height;

            std::shared_ptr<detail::FrameViewInternal> internal;
            uint8_t *node_internal_address;

            while (height >= 2)
            {
                get_node_internal(offset, internal);
                node_internal_address = (uint8_t *)internal->frame_view.get_view().data();

                uint64_t num_children = detail::NodeInternal::read_num_children(node_internal_address);

                for (uint64_t i = 0; i < num_children; i++)
                {
                    uint64_t child_entry_count = detail::NodeInternal::read_child_entry_count(node_internal_address, i);

                    if (index < child_entry_count)
                    {
                        offset = detail::NodeInternal::read_child_offset(node_internal_address, i);
                        height--;
                        goto next_level;
                    }

                    index -= child_entry_count;
                }

                return false;

            next_level:;
            }

            get_node_leaf(offset, leaf);
            uint8_t *node_leaf_address = (uint8_t *)leaf->frame_view.get_view().data();
            uint64_t num_children = detail::NodeLeaf::read_num_children(node_leaf_address);

            if (num_children <= 0 || index >= num_children)
            {
                return false;
            }

            entry_index = index;

            return true;
        }

        template <ReaderFindMode mode>
        bool find(std::string_view key, std::shared_ptr<detail::FrameViewLeaf> &leaf, uint64_t &entry_index, uint64_t *entry_start)
        {
            ZonePbtReader;

            uint64_t offset = footer.root_offset;
            uint64_t height = footer.tree_height;

            std::shared_ptr<detail::FrameViewInternal> internal;
            uint8_t *node_internal_address;

            while (height >= 2)
            {
                get_node_internal(offset, internal);
                node_internal_address = (uint8_t *)internal->frame_view.get_view().data();

                if (mode == EXACT)
                {
                    if (key.compare(detail::NodeInternal::read_left_key(node_internal_address)) < 0)
                    {
                        return false;
                    }
                }

                uint64_t num_children = detail::NodeInternal::read_num_children(node_internal_address);

                for (uint64_t i = 0; i < num_children; i++)
                {
                    if (key.compare(detail::NodeInternal::read_right_key(node_internal_address, i)) <= 0)
                    {
                        offset = detail::NodeInternal::read_child_offset(node_internal_address, i);
                        height--;
                        goto next_level;
                    }
                    if (entry_start != nullptr)
                    {
                        *entry_start += detail::NodeInternal::read_child_entry_count(node_internal_address, i);
                    }
                }

                return false;

            next_level:;
            }

            get_node_leaf(offset, leaf);
            uint8_t *node_leaf_address = (uint8_t *)leaf->frame_view.get_view().data();
            uint64_t num_children = detail::NodeLeaf::read_num_children(node_leaf_address);

            for (uint64_t i = 0; i < num_children; i++)
            {
                if (mode == EXACT)
                {
                    if (detail::NodeLeaf::read_key(node_leaf_address, i).compare(key) == 0)
                    {
                        if (entry_start != nullptr)
                        {
                            *entry_start += i;
                        }
                        entry_index = i;
                        return true;
                    }
                }
                if (mode == GREATER_OR_EQUAL)
                {
                    if (detail::NodeLeaf::read_key(node_leaf_address, i).compare(key) >= 0)
                    {
                        if (entry_start != nullptr)
                        {
                            *entry_start += i;
                        }
                        entry_index = i;
                        return true;
                    }
                }
            }

            return false;
        }

        void read_footer()
        {
            ZonePbtReader;

            uint8_t *address = offset_to_address(storage->get_size() - detail::Footer::size());
            detail::Footer::read(address, footer);
            footer.validate();
        }

        uint8_t *offset_to_address(uint64_t offset) const
        {
            ZonePbtReader;

            return (uint8_t *)storage->get_address() + offset;
        }

        uint64_t address_to_offset(uint8_t *address) const
        {
            ZonePbtReader;

            return (uint64_t)(address - (uint8_t *)storage->get_address());
        }

        void get_node_leaf(uint64_t offset, std::shared_ptr<detail::FrameViewLeaf> &leaf)
        {
            ZonePbtReader;

            if (!leaf_cache.try_get(offset, leaf))
            {
                leaf = std::make_shared<detail::FrameViewLeaf>(offset_to_address(offset), footer.enable_lz4_compression);
                leaf_cache.put(offset, leaf);
            }
        }

        void get_node_internal(uint64_t offset, std::shared_ptr<detail::FrameViewInternal> &internal)
        {
            ZonePbtReader;

            if (!internal_cache.try_get(offset, internal))
            {
                internal = std::make_shared<detail::FrameViewInternal>(offset_to_address(offset), footer.enable_lz4_compression);
                internal_cache.put(offset, internal);
            }
        }
    };
}
