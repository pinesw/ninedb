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

#include "./detail/format.hpp"
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
            : storage(storage), cache_leaf(64), cache_internal(64)
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

            uint64_t node_offset = footer.level_0_end;
            uint64_t entry_index = 0;
            std::shared_ptr<detail::NodeLeafRead> node_leaf(nullptr);
            find<EXACT>(key, node_leaf, node_offset, entry_index);

            if (node_offset >= footer.level_0_end)
            {
                return false;
            }
            value = node_leaf->value(entry_index);
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

            uint64_t node_offset = footer.level_0_end;
            uint64_t entry_index = 0;
            std::shared_ptr<detail::NodeLeafRead> node_leaf(nullptr);
            find_index(index, node_leaf, node_offset, entry_index);

            if (node_offset >= footer.level_0_end)
            {
                return false;
            }
            key = node_leaf->key(entry_index);
            value = node_leaf->value(entry_index);
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

            uint64_t node_offset = footer.level_0_end;
            uint64_t entry_index = 0;
            std::shared_ptr<detail::NodeLeafRead> node_leaf(nullptr);
            find<GREATER_OR_EQUAL>(key, node_leaf, node_offset, entry_index);

            return Iterator(storage, entry_index, node_offset, footer.level_0_end);
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

            uint64_t node_offset = footer.level_0_end;
            uint64_t entry_index = 0;
            std::shared_ptr<detail::NodeLeafRead> node_leaf(nullptr);
            find_index(index, node_leaf, node_offset, entry_index);

            return Iterator(storage, entry_index, node_offset, footer.level_0_end);
        }

        /**
         * Return an iterator over the key-value pairs in the PBT.
         * The iterator is positioned at the first key-value pair.
         */
        Iterator begin() const
        {
            ZonePbtReader;

            return Iterator(storage, 0, 0, footer.level_0_end);
        }

        /**
         * Return an iterator over the key-value pairs in the PBT.
         * The iterator is positioned at the end of the PBT.
         */
        Iterator end() const
        {
            ZonePbtReader;

            return Iterator(storage, 0, footer.level_0_end, footer.level_0_end);
        }

        /**
         * Visits all the nodes in the PBT in order.
         * At the leaf nodes, values tested positively against the predicate are added to the accumulator.
         * Internal nodes are also tested against the predicate on their reduced values, and their subtrees are skipped if the predicate returns false.
         */
        void traverse(const std::function<bool(std::string_view value)> &predicate, std::vector<std::string> &accumulator)
        {
            ZonePbtReader;

            if (footer.tree_height == 0)
            {
                return;
            }

            traverse(predicate, accumulator, footer.root_offset, footer.root_size, footer.tree_height);
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
        ninedb::detail::RLRUCache<uint64_t, std::shared_ptr<detail::NodeLeafRead>> cache_leaf;
        ninedb::detail::RLRUCache<uint64_t, std::shared_ptr<detail::NodeInternalRead>> cache_internal;

        void traverse(const std::function<bool(std::string_view value)> &predicate, std::vector<std::string> &accumulator, uint64_t offset, uint64_t size, uint64_t height)
        {
            ZonePbtReader;

            if (height == 1)
            {
                detail::NodeLeafRead node_leaf(offset, size, *storage);

                for (uint64_t i = 0; i < node_leaf.num_children(); i++)
                {
                    if (predicate(node_leaf.value(i)))
                    {
                        accumulator.push_back(std::string(node_leaf.value(i)));
                    }
                }
            }
            else
            {
                detail::NodeInternalRead node_internal(offset, size, *storage);

                for (uint64_t i = 0; i < node_internal.num_children(); i++)
                {
                    if (predicate(node_internal.reduced_value(i)))
                    {
                        traverse(predicate, accumulator, node_internal.child_offset(i), node_internal.child_size(i), height - 1);
                    }
                }
            }
        }

        void find_index(uint64_t index, std::shared_ptr<detail::NodeLeafRead> &node_leaf, uint64_t &node_offset, uint64_t &entry_index)
        {
            ZonePbtReader;

            uint64_t offset = footer.root_offset;
            uint64_t size = footer.root_size;
            uint64_t height = footer.tree_height;

            std::shared_ptr<detail::NodeInternalRead> node_internal(nullptr);

            while (height >= 2)
            {
                read_node_internal(offset, size, node_internal);

                for (uint64_t i = 0; i < node_internal->num_children(); i++)
                {
                    if (index < node_internal->child_entry_count(i))
                    {
                        offset = node_internal->child_offset(i);
                        size = node_internal->child_size(i);
                        height--;
                        goto next_level;
                    }

                    index -= node_internal->child_entry_count(i);
                }

                return;

            next_level:;
            }

            read_node_leaf(offset, size, node_leaf);

            if (node_leaf->num_children() <= 0 || index >= node_leaf->num_children())
            {
                return;
            }

            node_offset = offset;
            entry_index = index;
        }

        template <ReaderFindMode mode>
        void find(std::string_view key, std::shared_ptr<detail::NodeLeafRead> &node_leaf, uint64_t &node_offset, uint64_t &entry_index)
        {
            ZonePbtReader;

            uint64_t offset = footer.root_offset;
            uint64_t size = footer.root_size;
            uint64_t height = footer.tree_height;

            std::shared_ptr<detail::NodeInternalRead> node_internal(nullptr);

            while (height >= 2)
            {
                read_node_internal(offset, size, node_internal);

                if (mode == EXACT)
                {
                    if (key.compare(node_internal->left_key()) < 0)
                    {
                        return;
                    }
                }

                for (uint64_t i = 0; i < node_internal->num_children(); i++)
                {
                    if (key.compare(node_internal->right_key(i)) <= 0)
                    {
                        offset = node_internal->child_offset(i);
                        size = node_internal->child_size(i);
                        height--;
                        goto next_level;
                    }
                }

                return;

            next_level:;
            }

            read_node_leaf(offset, size, node_leaf);

            if (node_leaf->num_children() <= 0)
            {
                return;
            }

            for (uint64_t i = 0; i < node_leaf->num_children(); i++)
            {
                if (mode == EXACT)
                {
                    if (node_leaf->key(i).compare(key) == 0)
                    {
                        node_offset = offset;
                        entry_index = i;
                        return;
                    }
                }
                if (mode == GREATER_OR_EQUAL)
                {
                    if (node_leaf->key(i).compare(key) >= 0)
                    {
                        node_offset = offset;
                        entry_index = i;
                        return;
                    }
                }
            }
        }

        void read_footer()
        {
            ZonePbtReader;

            detail::Footer::read(storage->get_size() - detail::Footer::size_of(), *storage, footer);
            footer.validate();
        }

        void read_node_leaf(uint64_t offset, uint64_t size, std::shared_ptr<detail::NodeLeafRead> &node_leaf)
        {
            ZonePbtReader;

            if (!cache_leaf.try_get(offset, node_leaf))
            {
                node_leaf = std::make_shared<detail::NodeLeafRead>(offset, size, *storage);
                cache_leaf.put(offset, node_leaf);
            }
        }

        void read_node_internal(uint64_t offset, uint64_t size, std::shared_ptr<detail::NodeInternalRead> &node_internal)
        {
            ZonePbtReader;

            if (!cache_internal.try_get(offset, node_internal))
            {
                node_internal = std::make_shared<detail::NodeInternalRead>(offset, size, *storage);
                cache_internal.put(offset, node_internal);
            }
        }
    };
}
