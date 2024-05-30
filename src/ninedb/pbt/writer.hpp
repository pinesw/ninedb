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
        Writer(uint64_t identifier, const std::string &path, const WriterConfig &config)
            : identifier(identifier), config(config)
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
            init_buffers();
        }

        Writer(uint64_t identifier, const std::shared_ptr<detail::Storage> &storage, const WriterConfig &config)
            : identifier(identifier), storage(storage), config(config)
        {
            storage->clear();
            init_buffers();
        }

        /**
         * Create a reader from the writer.
         * Should only be called after finish() has been called.
         */
        Reader to_reader(const ReaderConfig &config) const
        {
            return Reader(storage, config);
        }

        template <typename K, typename V>
        /**
         * Add a key-value pair to the PBT.
         */
        void add(K &&key, V &&value)
        {
            static_assert(ninedb::detail::is_string_kind_v<K>, "K must be a string");
            static_assert(ninedb::detail::is_string_kind_v<V>, "V must be a string");

            ZonePbtWriter;

            buffer_keys[buffer_length] = std::forward<K>(key);
            buffer_values[buffer_length] = std::forward<V>(value);
            num_entries++;
            buffer_length++;
            if (buffer_length >= config.max_node_entries)
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
            std::vector<std::string> keys;

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
                    itrs[i].get_key(keys[i]);
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

                add(std::move(keys[min_index]), std::string(itrs[min_index].get_value()));
                itrs[min_index].next();

                if (!itrs[min_index].is_end())
                {
                    itrs[min_index].get_key(keys[min_index]);
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
            while (entry_count > config.max_node_entries)
            {
                entry_count = detail::div_ceil(entry_count, config.max_node_entries);
                entry_counts.push_back(entry_count);
            }

            detail::Footer footer = detail::Format::create_footer();
            footer.level_0_end = write_offset;
            footer.tree_height = entry_counts.size();
            footer.identifier = identifier;
            footer.num_entries = num_entries;
            footer.compression = config.enable_compression ? 1 : 0;

            detail::NodeInternal node_internal;
            std::vector<std::string> keys;
            std::vector<std::string> values;
            std::vector<std::string> reduced_values;
            keys.resize(config.max_node_entries + 1);
            values.resize(config.max_node_entries);
            reduced_values.resize(config.max_node_entries);
            node_internal.suffixes.resize(config.max_node_entries + 1);
            node_internal.reduced_values.resize(config.max_node_entries);
            node_internal.offsets.resize(config.max_node_entries);
            node_internal.num_entries.resize(config.max_node_entries);
            uint64_t read_offset = 0;

            for (size_t i = 1; i < footer.tree_height; i++)
            {
                uint64_t num_level_entries = entry_counts[i];

                for (uint64_t j = 0; j < num_level_entries; j += config.max_node_entries)
                {
                    node_internal.num_children = std::min(config.max_node_entries, num_level_entries - j);

                    std::string *first_key = &keys[0];
                    uint64_t num_entries;
                    for (uint64_t k = 0; k < node_internal.num_children; k++)
                    {
                        node_internal.offsets[k] = read_offset;
                        read_offset = read_node_metadata(i, read_offset, num_entries, values, first_key, &keys[k + 1]);
                        node_internal.num_entries[k] = num_entries;
                        first_key = nullptr;
                        if (config.reduce != nullptr)
                        {
                            config.reduce(values, reduced_values[k]);
                        }
                        node_internal.reduced_values[k] = reduced_values[k];
                    }

                    node_internal.stem = compute_stem(keys, node_internal.num_children + 1);
                    uint64_t stem_length = node_internal.stem.size();
                    for (uint64_t k = 0; k <= node_internal.num_children; k++)
                    {
                        node_internal.suffixes[k] = std::string_view(keys[k]).substr(stem_length);
                    }

                    append_node_internal(node_internal);
                }
            }

            footer.root_offset = read_offset;
            append_footer(footer);
            storage->set_size(write_offset);
        }

    private:
        WriterConfig config;
        std::shared_ptr<detail::Storage> storage;
        uint64_t identifier;
        uint64_t write_offset = 0;
        uint64_t num_entries = 0;
        uint64_t buffer_length = 0;
        std::vector<std::string> buffer_keys;
        std::vector<std::string> buffer_values;
        detail::NodeLeaf buffer_leaf;

        void init_buffers()
        {
            buffer_keys.resize(config.max_node_entries);
            buffer_values.resize(config.max_node_entries);
            buffer_leaf.suffixes.resize(config.max_node_entries);
            buffer_leaf.values.resize(config.max_node_entries);
        }

        uint64_t compute_stem_length(const std::vector<std::string> &keys, uint64_t num_keys) const
        {
            ZonePbtWriter;

            if (num_keys <= 0)
            {
                return 0;
            }

            uint64_t min_length = keys[0].size();
            for (uint64_t i = 1; i < num_keys; i++)
            {
                min_length = std::min<uint64_t>(min_length, keys[i].size());
            }

            uint64_t length = 0;
            while (length < min_length)
            {
                for (uint64_t i = 1; i < num_keys; i++)
                {
                    if (keys[0][length] != keys[i][length])
                    {
                        return length;
                    }
                }
                length++;
            }

            return length;
        }

        std::string compute_stem(const std::vector<std::string> &keys, uint64_t num_keys) const
        {
            ZonePbtWriter;

            if (!config.enable_prefix_encoding)
            {
                return "";
            }

            uint64_t length = compute_stem_length(keys, num_keys);
            if (length >= 1)
            {
                return keys[0].substr(0, length);
            }

            return "";
        }

        void flush()
        {
            ZonePbtWriter;

            uint64_t num_entries = buffer_length;
            if (num_entries <= 0)
            {
                return;
            }

            buffer_leaf.num_children = num_entries;
            buffer_leaf.stem = compute_stem(buffer_keys, num_entries);

            for (uint64_t i = 0; i < num_entries; i++)
            {
                buffer_leaf.suffixes[i] = std::string_view(buffer_keys[i]).substr(buffer_leaf.stem.size());
                buffer_leaf.values[i] = std::string_view(buffer_values[i]);
            }

            append_node_leaf(buffer_leaf);
            buffer_length = 0;
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

            ensure_space(detail::Format::sizeof_footer());
            uint8_t *address = (uint8_t *)storage->get_address() + write_offset;
            detail::Format::write_footer(address, footer);
            write_offset = address_to_offset(address);
        }

        void append_node_leaf(const detail::NodeLeaf &node)
        {
            ZonePbtWriter;

            uint8_t *address = (uint8_t *)storage->get_address() + write_offset;
            if (config.enable_compression)
            {
                ensure_space(detail::Format::size_upper_bound_node_leaf<true>(node));
                detail::Format::write_node_leaf<true>(address, node);
            }
            else
            {
                ensure_space(detail::Format::size_upper_bound_node_leaf<false>(node));
                detail::Format::write_node_leaf<false>(address, node);
            }
            write_offset = address_to_offset(address);
        }

        void append_node_internal(const detail::NodeInternal &node)
        {
            ZonePbtWriter;

            uint8_t *address = (uint8_t *)storage->get_address() + write_offset;
            if (config.enable_compression)
            {
                ensure_space(detail::Format::size_upper_bound_node_internal<true>(node));
                detail::Format::write_node_internal<true>(address, node);
            }
            else
            {
                ensure_space(detail::Format::size_upper_bound_node_internal<false>(node));
                detail::Format::write_node_internal<false>(address, node);
            }
            write_offset = address_to_offset(address);
        }

        uint64_t read_node_metadata(uint64_t level, uint64_t offset, uint64_t &num_entries, std::vector<std::string> &values, std::string *first_key, std::string *last_key) const
        {
            ZonePbtWriter;

            uint8_t *address = (uint8_t *)storage->get_address() + offset;
            if (level <= 1)
            {
                read_node_leaf_metadata(address, num_entries, values, first_key, last_key);
            }
            else
            {
                read_node_internal_metadata(address, num_entries, values, first_key, last_key);
            }
            return address_to_offset(address);
        }

        void read_node_leaf_metadata(uint8_t *&address, uint64_t &num_entries, std::vector<std::string> &values, std::string *first_key, std::string *last_key) const
        {
            detail::NodeLeaf node;
            if (config.enable_compression)
            {
                detail::Format::read_node_leaf<true>(address, node);
            }
            else
            {
                detail::Format::read_node_leaf<false>(address, node);
            }
            num_entries = node.num_children;
            values = std::move(node.values);
            if (first_key != nullptr)
            {
                std::string_view first_key_suffix = node.suffixes[0];
                first_key->resize(node.stem.size() + first_key_suffix.size());
                first_key->replace(0, node.stem.size(), node.stem);
                first_key->replace(node.stem.size(), first_key_suffix.size(), first_key_suffix);
            }
            if (last_key != nullptr)
            {
                std::string_view last_key_suffix = node.suffixes[num_entries - 1];
                last_key->resize(node.stem.size() + last_key_suffix.size());
                last_key->replace(0, node.stem.size(), node.stem);
                last_key->replace(node.stem.size(), last_key_suffix.size(), last_key_suffix);
            }
        }

        void read_node_internal_metadata(uint8_t *&address, uint64_t &num_entries, std::vector<std::string> &values, std::string *first_key, std::string *last_key) const
        {
            detail::NodeInternal node;
            if (config.enable_compression)
            {
                detail::Format::read_node_internal<true>(address, node);
            }
            else
            {
                detail::Format::read_node_internal<false>(address, node);
            }
            num_entries = 0;
            for (uint64_t i = 0; i < node.num_children; i++)
            {
                num_entries += node.num_entries[i];
            }
            values = std::move(node.reduced_values);
            if (first_key != nullptr)
            {
                std::string_view first_key_suffix = node.suffixes[0];
                first_key->resize(node.stem.size() + first_key_suffix.size());
                first_key->replace(0, node.stem.size(), node.stem);
                first_key->replace(node.stem.size(), first_key_suffix.size(), first_key_suffix);
            }
            if (last_key != nullptr)
            {
                std::string_view last_key_suffix = node.suffixes[node.num_children];
                last_key->resize(node.stem.size() + last_key_suffix.size());
                last_key->replace(0, node.stem.size(), node.stem);
                last_key->replace(node.stem.size(), last_key_suffix.size(), last_key_suffix);
            }
        }
    };
}
