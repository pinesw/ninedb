#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "./structures.hpp"
#include "../../pbt/reader.hpp"

namespace ninedb::detail::level_manager
{
    struct LevelManager
    {
        static LevelManager open(const std::string &path, const Config &config)
        {
            State initial_state;
            initial_state.next_index = 0;
            initial_state.global_start = 0;
            initial_state.levels = {};

            LevelManager level_manager(path, config, initial_state);
            level_manager.load_state();
            if (level_manager.state.levels.empty())
            {
                level_manager.add_new_level();
            }

            return level_manager;
        }

        std::string get_next_level_0_file_path() const
        {
            return get_file_path(state.next_index, 0);
        }

        void advance_level_0()
        {
            state.levels[0].indices.push_back(state.next_index);
            state.next_index++;
        }

        uint64_t get_global_start() const
        {
            return state.global_start;
        }

        void set_global_start(uint64_t global_start)
        {
            state.global_start = global_start;
        }

        void load_state()
        {
            std::filesystem::path max_index_file_path;
            for (const auto &entry : std::filesystem::directory_iterator(path))
            {
                if (!entry.is_regular_file())
                {
                    continue;
                }

                std::string file_name = entry.path().filename().string();
                uint64_t index, level;
                parse_file_path(file_name, index, level);

                if (index >= state.next_index)
                {
                    max_index_file_path = entry.path();
                    state.next_index = index + 1;
                }
                while (level >= state.levels.size())
                {
                    add_new_level();
                }
                state.levels[level].indices.push_back(index);
            }
            if (state.levels.empty())
            {
                add_new_level();
                return;
            }
            for (auto &level : state.levels)
            {
                std::sort(level.indices.begin(), level.indices.end());
            }
            auto footer = pbt::Reader::read_footer_from_file(max_index_file_path);
            state.global_start = footer.global_end;
        }

        std::vector<std::string> get_unmerged_files() const
        {
            std::vector<std::string> files;
            for (const auto &level : state.levels)
            {
                for (uint64_t index : level.indices)
                {
                    files.push_back(get_file_path(index, level.level));
                }
            }
            return files;
        }

        std::optional<MergeOperation> get_cascaded_merge_operation(bool reverse) const
        {
            if (!should_merge_level(0, false))
            {
                return std::nullopt;
            }
            MergeOperation merge_operation;
            merge_operation.dst_index = state.levels[0].indices.back();
            uint64_t level = 0;
            do
            {
                merge_operation.dst_level = level + 1;
                for (uint64_t i = 0; i < state.levels[level].indices.size(); i++)
                {
                    merge_operation.src_levels_and_indices.push_back({level, state.levels[level].indices[i]});
                }
                level++;
            } while (should_merge_level(level, true));
            if (reverse)
            {
                std::reverse(merge_operation.src_levels_and_indices.begin(), merge_operation.src_levels_and_indices.end());
            }
            return merge_operation;
        }

        std::optional<MergeOperation> get_full_merge(bool reverse) const
        {
            if (!can_merge_all())
            {
                return std::nullopt;
            }

            MergeOperation merge_operation;
            merge_operation.dst_level = state.levels.size() - 1;
            merge_operation.dst_index = 0;
            for (auto it = state.levels.rbegin(); it != state.levels.rend(); ++it)
            {
                for (uint64_t i = 0; i < it->indices.size(); i++)
                {
                    merge_operation.src_levels_and_indices.push_back({it->level, it->indices[i]});
                }
                if (!it->indices.empty())
                {
                    merge_operation.dst_index = it->indices.back();
                    if (merge_operation.dst_level == it->level)
                    {
                        merge_operation.dst_index++;
                    }
                }
            }
            if (reverse)
            {
                std::reverse(merge_operation.src_levels_and_indices.begin(), merge_operation.src_levels_and_indices.end());
            }
            return merge_operation;
        }

        std::string get_file_path(uint64_t index, uint64_t level) const
        {
            std::string index_name = std::to_string(index);
            std::string index_name_padded(20 - index_name.size(), '0');
            index_name_padded += index_name;

            std::string level_name = std::to_string(level);
            std::string level_name_padded(8 - level_name.size(), '0');
            level_name_padded += level_name;

            return path + "/" + index_name_padded + "-" + level_name_padded + ".pbt";
        }

        void parse_file_path(const std::string &file_path, uint64_t &index, uint64_t &level) const
        {
            std::string file_name = std::filesystem::path(file_path).filename().string();
            if (file_name.size() != 33 || file_name[20] != '-')
            {
                throw std::runtime_error("Invalid file name: " + file_name);
            }
            index = std::stoull(file_name.substr(0, 20));
            level = std::stoull(file_name.substr(21, 8));
        }

        void apply_merge_operation(const MergeOperation &merge_operation)
        {
            while (merge_operation.dst_level >= state.levels.size())
            {
                add_new_level();
            }
            for (auto &level_and_index : merge_operation.src_levels_and_indices)
            {
                state.levels[level_and_index.level].indices.erase(
                    std::remove(state.levels[level_and_index.level].indices.begin(),
                                state.levels[level_and_index.level].indices.end(),
                                level_and_index.index),
                    state.levels[level_and_index.level].indices.end());
            }
            state.levels[merge_operation.dst_level].indices.push_back(merge_operation.dst_index);
            state.next_index = merge_operation.dst_index + 1;
        }

    private:
        Config config;
        State state;
        std::string path;

        LevelManager(const std::string &path, const Config &config, const State &state)
            : path(path), config(config), state(state) {}

        bool should_merge_level(uint64_t level, bool plus_one) const
        {
            if (level >= state.levels.size())
            {
                return false;
            }
            uint64_t count = state.levels[level].indices.size();
            if (plus_one)
            {
                count++;
            }
            return count >= config.max_level_count;
        }

        bool can_merge_all() const
        {
            uint64_t sum = 0;
            for (auto &level : state.levels)
            {
                sum += level.indices.size();
                if (sum >= 2)
                {
                    return true;
                }
            }
            return false;
        }

        void add_new_level()
        {
            state.levels.push_back({state.levels.size(), {}});
        }
    };
}
