#pragma once

#include <cstdint>
#include <functional>
#include <fstream>
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
            : storage(storage)
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
        bool get(std::string_view key, std::string_view &value)
        {
            ZonePbtReader;

            if (footer.tree_height == 0)
            {
                return false;
            }

            char *node_leaf_address;
            uint64_t entry_index;

            if (!find<EXACT>(key, node_leaf_address, entry_index, nullptr))
            {
                return false;
            }

            value = detail::NodeLeaf::read_value(node_leaf_address, entry_index);
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
        bool at(uint64_t index, std::string_view &key, std::string_view &value)
        {
            ZonePbtReader;

            if (footer.tree_height == 0)
            {
                return false;
            }

            char *node_leaf_address;
            uint64_t entry_index;

            if (!find_index(index, node_leaf_address, entry_index))
            {
                return false;
            }

            key = detail::NodeLeaf::read_key(node_leaf_address, entry_index);
            value = detail::NodeLeaf::read_value(node_leaf_address, entry_index);
            return true;
        }

        /**
         * Get the key and value at the given index.
         * The first element of the pair is the key, the second element is the value.
         */
        std::optional<std::pair<std::string_view, std::string_view>> at(uint64_t index)
        {
            ZonePbtReader;

            std::string_view key;
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

            char *node_leaf_address;
            uint64_t entry_index;
            uint64_t entry_start = 0;

            if (!find<GREATER_OR_EQUAL>(key, node_leaf_address, entry_index, &entry_start))
            {
                return end();
            }

            return Iterator(node_leaf_address, entry_index, count() - entry_start);
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

            char *node_leaf_address;
            uint64_t entry_index;

            if (!find_index(index, node_leaf_address, entry_index))
            {
                return end();
            }

            return Iterator(node_leaf_address, entry_index, count() - index);
        }

        /**
         * Return an iterator over the key-value pairs in the PBT.
         * The iterator is positioned at the first key-value pair.
         */
        Iterator begin() const
        {
            ZonePbtReader;

            return Iterator(offset_to_address(0), 0, count());
        }

        /**
         * Return an iterator over the key-value pairs in the PBT.
         * The iterator is positioned at the end of the PBT.
         */
        Iterator end() const
        {
            ZonePbtReader;

            return Iterator(nullptr, 0, 0);
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

        /**
         * Read only the footer of a PBT file.
         */
        static detail::Footer read_footer_from_file(std::filesystem::path file_path)
        {
            ZonePbtReader;

            std::string buffer;
            buffer.resize(detail::Footer::size_of());
            std::ifstream file;
            file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            file.open(file_path, std::ios::binary);
            file.seekg(-detail::Footer::size_of(), std::ios::end);
            file.read(buffer.data(), detail::Footer::size_of());
            file.close();

            char *buffer_ptr = buffer.data();
            detail::Footer footer;
            detail::Footer::read(buffer_ptr, footer);
            footer.validate();

            return footer;
        }

    private:
        detail::Footer footer;
        std::shared_ptr<detail::Storage> storage;

        void traverse(const std::function<bool(std::string_view value)> &predicate, std::vector<std::string_view> &accumulator, uint64_t offset, uint64_t height)
        {
            ZonePbtReader;

            if (height >= 2)
            {
                char *node_internal_address = offset_to_address(offset);
                uint16_t num_children = detail::NodeInternal::read_num_children(node_internal_address);

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
                char *node_leaf_address = offset_to_address(offset);
                uint16_t num_children = detail::NodeLeaf::read_num_children(node_leaf_address);

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

        bool find_index(uint64_t index, char *&node_leaf_address, uint64_t &entry_index)
        {
            ZonePbtReader;

            if (index >= count())
            {
                return false;
            }

            uint64_t offset = footer.root_offset;
            uint64_t height = footer.tree_height;

            char *node_internal_address;
            uint64_t leaf_entry_start = 0;

            while (height >= 2)
            {
                node_internal_address = offset_to_address(offset);

                uint16_t num_children = detail::NodeInternal::read_num_children(node_internal_address);
                for (uint16_t i = 0; i < num_children; i++)
                {
                    if (i == num_children - 1)
                    {
                        offset = detail::NodeInternal::read_child_offset(node_internal_address, i);
                        break;
                    }

                    uint64_t read_child_entry_start = detail::NodeInternal::read_child_entry_start(node_internal_address, i + 1);

                    if (index < read_child_entry_start)
                    {
                        offset = detail::NodeInternal::read_child_offset(node_internal_address, i);
                        break;
                    }

                    leaf_entry_start = read_child_entry_start;
                }

                height--;
            }

            node_leaf_address = offset_to_address(offset);
            uint16_t num_children = detail::NodeLeaf::read_num_children(node_leaf_address);
            entry_index = index - leaf_entry_start;

            return true;
        }

        template <ReaderFindMode mode>
        bool find(std::string_view key, char *&node_leaf_address, uint64_t &entry_index, uint64_t *entry_start)
        {
            ZonePbtReader;

            uint64_t offset = footer.root_offset;
            uint64_t height = footer.tree_height;

            char *node_internal_address = nullptr;

            while (height >= 2)
            {
                node_internal_address = offset_to_address(offset);

                if (mode == EXACT)
                {
                    if (key.compare(detail::NodeInternal::read_left_key(node_internal_address)) < 0)
                    {
                        return false;
                    }
                }

                uint16_t num_children = detail::NodeInternal::read_num_children(node_internal_address);

                uint64_t lo = 0;
                uint64_t hi = num_children - 1;
                while (lo < hi)
                {
                    uint64_t mid = lo + (hi - lo) / 2;
                    if (key.compare(detail::NodeInternal::read_right_key(node_internal_address, mid)) <= 0)
                    {
                        hi = mid;
                    }
                    else
                    {
                        lo = mid + 1;
                    }
                }

                if (mode == EXACT)
                {
                    if (key.compare(detail::NodeInternal::read_right_key(node_internal_address, lo)) > 0)
                    {
                        return false;
                    }
                }

                if (entry_start != nullptr)
                {
                    *entry_start = detail::NodeInternal::read_child_entry_start(node_internal_address, lo);
                }

                offset = detail::NodeInternal::read_child_offset(node_internal_address, lo);
                height--;
            }

            node_leaf_address = offset_to_address(offset);
            uint16_t num_children = detail::NodeLeaf::read_num_children(node_leaf_address);

            for (uint64_t i = 0; i < num_children; i++)
            {
                if (mode == EXACT && detail::NodeLeaf::read_key(node_leaf_address, i).compare(key) == 0 || mode == GREATER_OR_EQUAL && detail::NodeLeaf::read_key(node_leaf_address, i).compare(key) >= 0)
                {
                    if (entry_start != nullptr)
                    {
                        *entry_start += i;
                    }
                    entry_index = i;
                    return true;
                }
            }

            return false;
        }

        void read_footer()
        {
            ZonePbtReader;

            char *address = offset_to_address(storage->get_size() - detail::Footer::size_of());
            detail::Footer::read(address, footer);
            footer.validate();
        }

        char *offset_to_address(uint64_t offset) const
        {
            ZonePbtReader;

            return reinterpret_cast<char *>(storage->get_address()) + offset;
        }

        uint64_t address_to_offset(char *address) const
        {
            ZonePbtReader;

            return (uint64_t)(address - reinterpret_cast<char *>(storage->get_address()));
        }
    };
}
