#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "../detail/profiling.hpp"
#include "../detail/traits.hpp"

#include "./detail/format.hpp"
#include "./detail/storage.hpp"
#include "./detail/structures.hpp"
#include "./detail/utils.hpp"

#include "./config.hpp"
#include "./iterator.hpp"
#include "./reader.hpp"

namespace ninedb::pbt
{
    struct Writer
    {
        Writer(uint64_t global_counter, const std::string &path, const WriterConfig &config)
            : global_counter(global_counter), config(config)
        {
            if (std::filesystem::exists(path))
            {
                if (config.error_if_exists)
                {
                    throw std::runtime_error("File already exists");
                }
            }
            else
            {
                std::ofstream ofs;
                ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
                ofs.open(path, std::ios::out | std::ios::binary);
                ofs.close();
            }
            std::filesystem::resize_file(path, config.initial_pbt_size);
            storage = std::make_shared<detail::Storage>(path, false);
            storage->clear();
        }

        Writer(uint64_t global_counter, const std::shared_ptr<detail::Storage> &storage, const WriterConfig &config)
            : global_counter(global_counter), storage(storage), config(config)
        {
            storage->clear();
        }

        /**
         * Create a reader from the writer.
         * Should only be called after finish() has been called.
         */
        Reader to_reader() const
        {
            return Reader(storage);
        }

        /**
         * Add a key-value pair to the PBT.
         */
        void add(std::string_view key, std::string_view value)
        {
            ZonePbtWriter;

            buffer_leaf.add_key_value(key, value);

            num_entries++;
            if (buffer_leaf.num_children >= config.max_node_children)
            {
                flush();
            }
        }

        template <typename R>
        /**
         * Merge multiple PBTs into one.
         */
        void merge(const std::vector<R> &readers)
        {
            static_assert(ninedb::detail::is_dereferenceable_to_v<R, Reader>, "R must be dereferencable to a Reader");

            ZonePbtWriter;

            std::vector<Iterator> itrs;
            std::vector<std::string_view> keys;

            uint64_t num_storages = readers.size();
            itrs.reserve(num_storages);
            keys.resize(num_storages);

            uint64_t total_num_entries = 0;
            for (size_t i = 0; i < num_storages; i++)
            {
                ZonePbtWriterN("ninedb::pbt::Writer::merge init loop");

                itrs.push_back(readers[i]->begin());

                if (!itrs[i].is_end())
                {
                    keys[i] = itrs[i].get_key();
                }

                total_num_entries += readers[i]->count();
            }

            for (uint64_t i = 0; i < total_num_entries; i++)
            {
                ZonePbtWriterN("ninedb::pbt::Writer::merge merge loop");

                std::string_view min_key;
                uint64_t min_index = 0;
                for (size_t i = 0; i < num_storages; i++)
                {
                    if (itrs[i].is_end())
                    {
                        continue;
                    }
                    if (min_key.empty() || keys[i].compare(min_key) < 0)
                    {
                        min_key = keys[i];
                        min_index = i;
                    }
                }

                add(keys[min_index], itrs[min_index].get_value());
                itrs[min_index].next();

                if (!itrs[min_index].is_end())
                {
                    keys[min_index] = itrs[min_index].get_key();
                }
            }
        }

        /**
         * Finish writing the PBT.
         * Required to be called before the PBT can be read.
         * After calling this method, the writer is no longer usable.
         */
        void finish()
        {
            ZonePbtWriter;

            flush();

            std::vector<uint64_t> entry_counts;
            uint64_t entry_count = num_entries;
            if (entry_count >= 1)
            {
                entry_counts.push_back(entry_count);
            }
            while (entry_count > config.max_node_children)
            {
                entry_count = detail::div_ceil(entry_count, config.max_node_children);
                entry_counts.push_back(entry_count);
            }

            detail::Footer footer = detail::Footer();
            footer.level_0_end = write_offset;
            footer.tree_height = entry_counts.size();
            footer.global_start = global_counter;
            footer.global_end = global_counter + num_entries;

            std::vector<std::string_view> keys;
            std::vector<std::string_view> values;
            std::vector<std::string> reduced_values;
            keys.resize(config.max_node_children + 1);
            values.resize(config.max_node_children);
            reduced_values.resize(config.max_node_children);
            uint64_t read_offset = 0;

            for (size_t i = 1; i < footer.tree_height; i++)
            {
                uint64_t num_level_entries = entry_counts[i];

                for (uint64_t j = 0; j < num_level_entries; j += config.max_node_children)
                {
                    uint64_t num_children = std::min(config.max_node_children, num_level_entries - j);

                    for (uint64_t k = 0; k < num_children; k++)
                    {
                        uint64_t child_entry_count;
                        uint64_t child_offset = read_offset;

                        if (k == 0)
                        {
                            read_offset = read_node_metadata(read_offset, i, child_entry_count, values, &keys[0], &keys[1]);
                        }
                        else
                        {
                            read_offset = read_node_metadata(read_offset, i, child_entry_count, values, nullptr, &keys[k + 1]);
                        }

                        if (config.reduce != nullptr)
                        {
                            config.reduce(values, reduced_values[k]);
                        }

                        if (k == 0)
                        {
                            buffer_internal.add_first_child(keys[0], keys[1], reduced_values[0], child_entry_count, child_offset);
                        }
                        else
                        {
                            buffer_internal.add_child(keys[k + 1], reduced_values[k], child_entry_count, child_offset);
                        }
                    }

                    append_node_internal(buffer_internal);
                    buffer_internal.clear();
                }
            }

            footer.root_offset = read_offset;
            append_footer(footer);
            storage->set_size(write_offset);
        }

    private:
        WriterConfig config;
        std::shared_ptr<detail::Storage> storage;
        uint64_t global_counter;
        uint64_t write_offset = 0;
        uint64_t num_entries = 0;
        detail::NodeLeaf buffer_leaf;
        detail::NodeInternal buffer_internal;

        /**
         * Write the leaf node buffer to the storage and clear it.
         */
        void flush()
        {
            ZonePbtWriter;

            if (buffer_leaf.num_children <= 0)
            {
                return;
            }

            append_node_leaf(buffer_leaf);
            buffer_leaf.clear();
        }

        void ensure_space(uint64_t size)
        {
            ZonePbtWriter;

            if (storage->get_size() < write_offset + size)
            {
                uint64_t new_size = std::max<uint64_t>(write_offset + size, 2 * storage->get_size());
                storage->set_size(new_size);
            }
        }

        uint64_t address_to_offset(uint8_t *address) const
        {
            ZonePbtReader;

            return (uint64_t)(address - (uint8_t *)storage->get_address());
        }

        void append_footer(const detail::Footer &footer)
        {
            ZonePbtWriter;

            ensure_space(detail::Footer::size_of());
            uint8_t *address = (uint8_t *)storage->get_address() + write_offset;
            address = footer.write(address);
            write_offset = address_to_offset(address);
        }

        void append_node_leaf(const detail::NodeLeaf &node)
        {
            ZonePbtWriter;

            uint8_t *address = (uint8_t *)storage->get_address() + write_offset;
            ensure_space(node.size_of());
            address = node.write(address);
            write_offset = address_to_offset(address);
        }

        void append_node_internal(const detail::NodeInternal &node)
        {
            ZonePbtWriter;

            uint8_t *address = (uint8_t *)storage->get_address() + write_offset;
            ensure_space(node.size_of());
            address = node.write(address);
            write_offset = address_to_offset(address);
        }

        uint64_t read_node_metadata(uint64_t offset, uint64_t level, uint64_t &entry_count, std::vector<std::string_view> &values, std::string_view *first_key, std::string_view *last_key) const
        {
            ZonePbtWriter;

            uint8_t *address = (uint8_t *)storage->get_address() + offset;
            if (level <= 1)
            {
                return offset + read_node_leaf_metadata(address, entry_count, values, first_key, last_key);
            }
            else
            {
                return offset + read_node_internal_metadata(address, entry_count, values, first_key, last_key);
            }
        }

        uint64_t read_node_leaf_metadata(uint8_t *address, uint64_t &entry_count, std::vector<std::string_view> &values, std::string_view *first_key, std::string_view *last_key) const
        {
            detail::NodeLeafShallow node(address);
            entry_count = node.num_children();
            values.resize(entry_count);
            for (uint64_t i = 0; i < entry_count; i++)
            {
                values[i] = node.value(i);
            }
            if (first_key != nullptr)
            {
                *first_key = node.key(0);
            }
            if (last_key != nullptr)
            {
                *last_key = node.key(entry_count - 1);
            }
            return node.size_of();
        }

        uint64_t read_node_internal_metadata(uint8_t *address, uint64_t &entry_count, std::vector<std::string_view> &values, std::string_view *first_key, std::string_view *last_key) const
        {
            detail::NodeInternalShallow node(address);
            uint64_t num_children = node.num_children();
            entry_count = 0;
            for (uint64_t i = 0; i < num_children; i++)
            {
                entry_count += node.child_entry_count(i);
            }
            values.resize(num_children);
            for (uint64_t i = 0; i < num_children; i++)
            {
                values[i] = node.reduced_value(i);
            }
            if (first_key != nullptr)
            {
                *first_key = node.left_key();
            }
            if (last_key != nullptr)
            {
                *last_key = node.right_key(num_children - 1);
            }
            return node.size_of();
        }
    };
}
