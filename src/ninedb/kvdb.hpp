#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "./detail/profiling.hpp"
#include "./detail/buffer.hpp"
#include "./detail/level_manager/level_manager.hpp"

#include "./config.hpp"
#include "./iterator.hpp"
#include "./pbt/pbt.hpp"

namespace ninedb
{
    struct KvDb
    {
        /**
         * Open a db at the given path with the given config.
         */
        static KvDb open(const std::string &path, const Config &config)
        {
            ZoneDb;

            if (std::filesystem::is_directory(path))
            {
                if (config.error_if_exists)
                {
                    throw std::runtime_error("db already exists");
                }
                if (config.delete_if_exists)
                {
                    std::filesystem::remove_all(path);
                    std::filesystem::create_directory(path);
                }
            }
            else
            {
                if (config.create_if_missing)
                {
                    std::filesystem::create_directory(path);
                }
                else
                {
                    throw std::runtime_error("db does not exist");
                }
            }

            detail::level_manager::LevelManager level_manager = detail::level_manager::LevelManager::open(path, get_level_manager_config(config));

            return KvDb(config, level_manager);
        }

        /**
         * Add a key-value pair to the db.
         * If the key already exists, the value will be added after the existing values.
         */
        void add(std::string_view key, std::string_view value)
        {
            ZoneDb;

            // TODO: run flush/merge in a separate thread pool.

            buffer.insert(key, value);
            if (buffer.get_size() > config.max_buffer_size)
            {
                flush();
            }
        }

        /**
         * Get the first value for the given key.
         * If the key does not exist, false will be returned.
         * Otherwise, true will be returned and the value will be set.
         */
        bool get(std::string_view key, std::string_view &value) const
        {
            ZoneDb;

            for (const auto &[file_name, reader] : readers)
            {
                if (reader->get(key, value))
                {
                    return true;
                }
            }
            return false;
        }

        /**
         * Get the first value for the given key.
         */
        std::optional<std::string_view> get(std::string_view key) const
        {
            ZoneDb;

            std::string_view value;
            if (get(key, value))
            {
                return value;
            }
            return std::nullopt;
        }

        /**
         * Get the key and value at the given index.
         * If the index is out of range, false will be returned.
         * Otherwise, true will be returned and the key and value will be set.
         */
        bool at(uint64_t index, std::string_view &key, std::string_view &value) const
        {
            ZoneDb;

            for (const auto &[file_name, reader] : readers)
            {
                uint64_t num_entries = reader->count();
                if (index < num_entries)
                {
                    reader->at(index, key, value);
                    return true;
                }
                index -= num_entries;
            }
            return false;
        }

        /**
         * Get the key-value pair at the given index.
         * If the index is out of range, std::nullopt will be returned.
         */
        std::optional<std::pair<std::string_view, std::string_view>> at(uint64_t index) const
        {
            ZoneDb;

            std::pair<std::string_view, std::string_view> result;
            if (at(index, result.first, result.second))
            {
                return result;
            }
            return std::nullopt;
        }

        /**
         * Return an iterator to the first key-value pair in the db.
         */
        Iterator begin() const
        {
            ZoneDb;

            std::vector<pbt::Iterator> itrs;
            for (const auto &[file_name, reader] : readers)
            {
                itrs.push_back(reader->begin());
            }
            return Iterator(std::move(itrs));
        }

        /**
         * Return an iterator to the first key-value pair in the db with a key equal to the given key.
         * If no such key exists, the iterator will be at the first key greater than the given key.
         * If no such key exists, the iterator will be at the end.
         */
        Iterator seek(std::string_view key) const
        {
            ZoneDb;

            std::vector<pbt::Iterator> itrs;
            for (const auto &[file_name, reader] : readers)
            {
                itrs.push_back(reader->seek_first(key));
            }
            return Iterator(std::move(itrs));
        }

        /**
         * Return an iterator to the key-value pair at the given index in the db.
         * If the index is out of range, the iterator will be at the end.
         */
        Iterator seek(uint64_t index) const
        {
            ZoneDb;

            std::vector<pbt::Iterator> itrs;
            for (const auto &[file_name, reader] : readers)
            {
                itrs.push_back(reader->seek(index));
                index -= std::min(index, reader->count());
            }
            return Iterator(std::move(itrs));
        }

        /**
         * Visit all nodes in the trees in the db in order.
         * At the leaf nodes, values are tested with the given predicate and accumulated if the predicate returns true.
         * Internal nodes are also tested against the predicate on their reduced values, and their subtrees are skipped if the predicate returns false.
         */
        void traverse(const std::function<bool(std::string_view value)> &predicate, std::vector<std::string_view> &accumulator) const
        {
            ZoneDb;

            for (const auto &[file_name, reader] : readers)
            {
                reader->traverse(predicate, accumulator);
            }
        }

        /**
         * Compact the db.
         * This will merge all the files in the db into a single file.
         */
        void compact()
        {
            ZoneDb;

            flush_buffer();
            auto merge_operation = level_manager.get_full_merge(false);
            if (merge_operation.has_value())
            {
                perform_merge_operation(merge_operation.value());
            }
        }

        /**
         * Flush the buffer to disk and perform any necessary merges.
         */
        void flush()
        {
            ZoneDb;

            flush_buffer();
            auto merge_operation = level_manager.get_cascaded_merge_operation(false);
            if (merge_operation.has_value())
            {
                perform_merge_operation(merge_operation.value());
            }
        }

    private:
        Config config;
        detail::Buffer buffer;
        detail::level_manager::LevelManager level_manager;
        std::map<std::string, std::shared_ptr<pbt::Reader>> readers;

        KvDb(const Config &config, const detail::level_manager::LevelManager &level_manager)
            : config(config), level_manager(level_manager)
        {
            ZoneDb;

            std::vector<std::string> unmerged_files = level_manager.get_unmerged_files();
            for (const auto &file : unmerged_files)
            {
                readers[file] = std::make_shared<pbt::Reader>(file);
            }
        }

        static pbt::WriterConfig get_writer_config(const Config &config)
        {
            ZoneDb;

            pbt::WriterConfig writer_config;
            writer_config.max_node_children = config.writer.max_node_children;
            writer_config.initial_pbt_size = config.writer.initial_pbt_size;
            writer_config.reduce = config.writer.reduce;
            writer_config.error_if_exists = false;
            return writer_config;
        }

        static detail::level_manager::Config get_level_manager_config(const Config &config)
        {
            ZoneDb;

            detail::level_manager::Config level_manager_config;
            level_manager_config.max_level_count = config.max_level_count;
            return level_manager_config;
        }

        void flush_buffer()
        {
            ZoneDb;

            if (buffer.get_size() == 0)
            {
                return;
            }

            uint64_t num_entries = buffer.get_count();
            std::string file_name = level_manager.get_next_level_0_file_path();
            pbt::Writer writer(level_manager.get_global_counter(), file_name, get_writer_config(config));
            for (const auto &[key, value] : buffer)
            {
                writer.add(key, value);
            }
            writer.finish();
            buffer.clear();

            level_manager.advance_level_0();
            level_manager.set_global_counter(level_manager.get_global_counter() + num_entries);

            readers[file_name] = std::make_shared<pbt::Reader>(writer.to_reader());
        }

        void perform_merge_operation(const detail::level_manager::MergeOperation &merge_operation)
        {
            ZoneDb;

            std::vector<std::shared_ptr<pbt::Reader>> src_readers;
            uint64_t global_counter = std::numeric_limits<uint64_t>::max();
            for (const auto &[level, index] : merge_operation.src_levels_and_indices)
            {
                std::string file_name = level_manager.get_file_path(index, level);
                src_readers.push_back(readers[file_name]);
                global_counter = std::min(global_counter, readers[file_name]->get_global_start());
            }

            std::string target_file_name = level_manager.get_file_path(merge_operation.dst_index, merge_operation.dst_level);
            pbt::Writer writer(global_counter, target_file_name, get_writer_config(config));
            writer.merge(src_readers);
            writer.finish();

            level_manager.apply_merge_operation(merge_operation);

            readers[target_file_name] = std::make_shared<pbt::Reader>(writer.to_reader());
            for (const auto &[level, index] : merge_operation.src_levels_and_indices)
            {
                std::string file_name = level_manager.get_file_path(index, level);
                readers.erase(file_name);
                std::filesystem::remove(file_name);
            }
        }

        // virtual std::optional<std::string> seek_first(const std::string &key) = 0;
        // virtual std::optional<std::string> seek_last(const std::string &key) = 0; // TODO: implement
        // virtual std::optional<std::string> seek_next(const std::string &key) = 0; // TODO: implement
        // virtual std::optional<std::string> seek_prev(const std::string &key) = 0; // TODO: implement
    };
}
