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
        Reader(const std::string &path, const ReaderConfig &config)
            : Reader(std::make_shared<detail::Storage>(path, true), config) {}

        Reader(const std::shared_ptr<detail::Storage> &storage, const ReaderConfig &config)
            : config(config), storage(storage), node_internal_cache(config.internal_node_cache_size), node_leaf_cache(config.leaf_node_cache_size)
        {
            read_footer();
        }

        uint64_t get_global_counter() const
        {
            ZonePbtReader;

            return footer.global_counter;
        }

        /**
         * Get the value for the given key.
         * Returns true if the key exists, false otherwise.
         * If the key exists, the value is written to the given string.
         * If the key occurs multiple times, the value of the first occurrence is written.
         */
        bool get(std::string_view key, std::string_view &value)
        {
            ZonePbtReader;

            if (footer.tree_height == 0)
            {
                return false;
            }

            uint64_t node_offset = footer.level_0_end;
            uint64_t entry_index = 0;
            std::shared_ptr<detail::NodeLeaf> node_leaf;
            find<EXACT>(key, node_leaf, node_offset, entry_index);

            if (node_offset >= footer.level_0_end)
            {
                return false;
            }
            value = node_leaf->values[entry_index];
            return true;
        }

        /**
         * Get the value for the given key.
         */
        std::optional<std::string_view> get(std::string_view key)
        {
            ZonePbtReader;

            std::string_view value;
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
        bool at(uint64_t index, std::string &key, std::string_view &value)
        {
            ZonePbtReader;

            if (footer.tree_height == 0)
            {
                return false;
            }

            uint64_t node_offset = footer.level_0_end;
            uint64_t entry_index = 0;
            std::shared_ptr<detail::NodeLeaf> node_leaf;
            find_index(index, key, node_leaf, node_offset, entry_index);

            if (node_offset >= footer.level_0_end)
            {
                return false;
            }
            value = node_leaf->values[entry_index];
            return true;
        }

        /**
         * Get the key and value at the given index.
         * The first element of the pair is the key, the second element is the value.
         */
        std::optional<std::pair<std::string, std::string_view>> at(uint64_t index)
        {
            ZonePbtReader;

            std::string key;
            std::string_view value;
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
            std::shared_ptr<detail::NodeLeaf> node_leaf;
            find<GREATER_OR_EQUAL>(key, node_leaf, node_offset, entry_index);

            return Iterator(storage, is_compressed(), entry_index, node_offset, footer.level_0_end);
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

            std::string key;
            uint64_t node_offset = footer.level_0_end;
            uint64_t entry_index = 0;
            std::shared_ptr<detail::NodeLeaf> node_leaf;
            find_index(index, key, node_leaf, node_offset, entry_index);

            return Iterator(storage, is_compressed(), entry_index, node_offset, footer.level_0_end);
        }

        /**
         * Return an iterator over the key-value pairs in the PBT.
         * The iterator is positioned at the first key-value pair.
         */
        Iterator begin() const
        {
            ZonePbtReader;

            return Iterator(storage, is_compressed(), 0, 0, footer.level_0_end);
        }

        /**
         * Return an iterator over the key-value pairs in the PBT.
         * The iterator is positioned at the end of the PBT.
         */
        Iterator end() const
        {
            ZonePbtReader;

            return Iterator(storage, is_compressed(), 0, footer.level_0_end, footer.level_0_end);
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

            return footer.num_entries;
        }

    private:
        ReaderConfig config;
        detail::Footer footer;
        std::shared_ptr<detail::Storage> storage;
        ninedb::detail::RLRUCache<uint64_t, std::shared_ptr<detail::NodeInternal>> node_internal_cache;
        ninedb::detail::RLRUCache<uint64_t, std::shared_ptr<detail::NodeLeaf>> node_leaf_cache;

        bool is_compressed() const
        {
            ZonePbtReader;

            return footer.compression != 0;
        }

        void traverse(const std::function<bool(std::string_view value)> &predicate, std::vector<std::string_view> &accumulator, uint64_t offset, uint64_t height)
        {
            ZonePbtReader;

            if (height == 1)
            {
                std::shared_ptr<detail::NodeLeaf> node_leaf;
                get_node_leaf(offset, node_leaf);

                for (uint64_t i = 0; i < node_leaf->num_children; i++)
                {
                    if (predicate(node_leaf->values[i]))
                    {
                        accumulator.push_back(node_leaf->values[i]);
                    }
                }
            }
            else
            {
                std::shared_ptr<detail::NodeInternal> node_internal;
                get_node_internal(offset, node_internal);

                for (uint64_t i = 0; i < node_internal->num_children; i++)
                {
                    if (predicate(node_internal->reduced_values[i]))
                    {
                        traverse(predicate, accumulator, node_internal->offsets[i], height - 1);
                    }
                }
            }
        }

        void find_index(uint64_t index, std::string &key, std::shared_ptr<detail::NodeLeaf> &node_leaf, uint64_t &node_offset, uint64_t &entry_index)
        {
            ZonePbtReader;

            uint64_t offset = footer.root_offset;
            uint64_t height = footer.tree_height;

            std::shared_ptr<detail::NodeInternal> node_internal;

            while (height >= 2)
            {
                get_node_internal(offset, node_internal);

                for (uint64_t i = 0; i < node_internal->num_children; i++)
                {
                    if (index < node_internal->num_entries[i])
                    {
                        offset = node_internal->offsets[i];
                        height--;
                        goto next_level;
                    }

                    index -= node_internal->num_entries[i];
                }

                return;

            next_level:;
            }

            get_node_leaf(offset, node_leaf);

            if (node_leaf->num_children <= 0 || index >= node_leaf->num_children)
            {
                return;
            }

            key.resize(node_leaf->stem.size() + node_leaf->suffixes[index].size());
            key.replace(0, node_leaf->stem.size(), node_leaf->stem);
            key.replace(node_leaf->stem.size(), node_leaf->suffixes[index].size(), node_leaf->suffixes[index]);

            entry_index = index;
            node_offset = offset;
        }

        template <ReaderFindMode mode>
        void find(std::string_view key, std::shared_ptr<detail::NodeLeaf> &node_leaf, uint64_t &node_offset, uint64_t &entry_index)
        {
            ZonePbtReader;

            uint64_t offset = footer.root_offset;
            uint64_t height = footer.tree_height;

            std::string_view key_suffix;
            std::shared_ptr<detail::NodeInternal> node_internal;

            while (height >= 2)
            {
                get_node_internal(offset, node_internal);

                if (mode == EXACT)
                {
                    if (key.substr(0, node_internal->stem.size()).compare(node_internal->stem) != 0)
                    {
                        return;
                    }
                }
                if (mode == GREATER_OR_EQUAL)
                {
                    auto comparison = key.substr(0, node_internal->stem.size()).compare(node_internal->stem);
                    if (comparison < 0)
                    {
                        offset = node_internal->offsets[0];
                        height--;
                        goto next_level;
                    }
                    else if (comparison > 0)
                    {
                        return;
                    }
                }
                key_suffix = key.substr(node_internal->stem.size());

                if (mode == EXACT)
                {
                    if (key_suffix.compare(node_internal->suffixes[0]) < 0)
                    {
                        return;
                    }
                }

                for (uint64_t i = 0; i < node_internal->num_children; i++)
                {
                    if (key_suffix.compare(node_internal->suffixes[i + 1]) <= 0)
                    {
                        offset = node_internal->offsets[i];
                        height--;
                        goto next_level;
                    }
                }

                return;

            next_level:;
            }

            get_node_leaf(offset, node_leaf);

            if (node_leaf->num_children <= 0)
            {
                return;
            }
            if (mode == EXACT)
            {
                if (key.substr(0, node_leaf->stem.size()).compare(node_leaf->stem) != 0)
                {
                    return;
                }
            }
            if (mode == GREATER_OR_EQUAL)
            {
                auto comparison = key.substr(0, node_leaf->stem.size()).compare(node_leaf->stem);
                if (comparison < 0)
                {
                    node_offset = offset;
                    entry_index = 0;
                    return;
                }
                else if (comparison > 0)
                {
                    return;
                }
            }
            key_suffix = key.substr(node_leaf->stem.size());

            for (uint64_t i = 0; i < node_leaf->num_children; i++)
            {
                if (mode == EXACT)
                {
                    if (node_leaf->suffixes[i].compare(key_suffix) == 0)
                    {
                        node_offset = offset;
                        entry_index = i;
                        return;
                    }
                }
                if (mode == GREATER_OR_EQUAL)
                {
                    if (node_leaf->suffixes[i].compare(key_suffix) >= 0)
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

            uint8_t *address = offset_to_address(storage->get_size() - detail::Format::sizeof_footer());
            detail::Format::read_footer(address, footer);
            detail::Format::validate_footer(footer);
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

        void get_node_internal(uint64_t offset, std::shared_ptr<detail::NodeInternal> &node)
        {
            ZonePbtReader;

            if (config.internal_node_cache_size == 0)
            {
                node = std::make_shared<detail::NodeInternal>();
                uint8_t *address = offset_to_address(offset);
                if (is_compressed())
                {
                    detail::Format::read_node_internal_compressed(address, *node);
                }
                else
                {
                    detail::Format::read_node_internal_uncompressed(address, *node);
                }
            }
            else
            {
                if (!node_internal_cache.try_get(offset, node))
                {
                    node = std::make_shared<detail::NodeInternal>();
                    uint8_t *address = offset_to_address(offset);
                    if (is_compressed())
                    {
                        detail::Format::read_node_internal_compressed(address, *node);
                    }
                    else
                    {
                        detail::Format::read_node_internal_uncompressed(address, *node);
                    }
                    node_internal_cache.put(offset, node);
                }
            }
        }

        void get_node_leaf(uint64_t offset, std::shared_ptr<detail::NodeLeaf> &node)
        {
            ZonePbtReader;

            if (config.leaf_node_cache_size == 0)
            {
                node = std::make_shared<detail::NodeLeaf>();
                uint8_t *address = offset_to_address(offset);
                if (is_compressed())
                {
                    detail::Format::read_node_leaf_compressed(address, *node);
                }
                else
                {
                    detail::Format::read_node_leaf_uncompressed(address, *node);
                }
            }
            else
            {
                if (!node_leaf_cache.try_get(offset, node))
                {
                    node = std::make_shared<detail::NodeLeaf>();
                    uint8_t *address = offset_to_address(offset);
                    if (is_compressed())
                    {
                        detail::Format::read_node_leaf_compressed(address, *node);
                    }
                    else
                    {
                        detail::Format::read_node_leaf_uncompressed(address, *node);
                    }
                    node_leaf_cache.put(offset, node);
                }
            }
        }
    };
}
