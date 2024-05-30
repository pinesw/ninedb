#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "./structures.hpp"
#include "../version.hpp"

namespace ninedb::detail::level_manager
{
    struct LevelManager
    {
        static LevelManager open(const std::string &path, const Config &config)
        {
            std::string state_path = get_state_path(path);

            State state;
            state.version_major = VERSION_MAJOR;
            state.version_minor = VERSION_MINOR;
            state.next_index = 0;
            state.identity_counter = 0;

            LevelManager level_manager(path, state_path, config, state);

            if (std::filesystem::is_regular_file(state_path))
            {
                if (config.error_if_exists)
                {
                    throw std::runtime_error("level manager already exists");
                }
                if (config.delete_if_exists)
                {
                    std::filesystem::remove(state_path);
                }
                else
                {
                    level_manager.load_state();
                }
            }
            else
            {
                level_manager.add_new_level();
                level_manager.save_state();
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

        uint64_t get_identity_counter() const
        {
            return state.identity_counter;
        }

        void set_identity_counter(uint64_t identity_counter)
        {
            state.identity_counter = identity_counter;
        }

        void load_state()
        {
            std::ifstream file;
            file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            file.open(state_path);
            nlohmann::json json;
            file >> json;
            file.close();

            state.version_major = json["version_major"].get<uint32_t>();
            state.version_minor = json["version_minor"].get<uint32_t>();
            state.next_index = json["next_index"].get<uint64_t>();
            state.identity_counter = json["identity_counter"].get<uint64_t>();
            for (auto &level_json : json["levels"])
            {
                LevelState level;
                level.level = level_json["level"];
                level.indices = level_json["indices"].get<std::vector<uint64_t>>();
                state.levels.push_back(std::move(level));
            }
        }

        void save_state() const
        {
            nlohmann::json json;
            json["version_major"] = state.version_major;
            json["version_minor"] = state.version_minor;
            json["next_index"] = state.next_index;
            json["identity_counter"] = state.identity_counter;
            json["levels"] = nlohmann::json::array();
            for (const auto &level : state.levels)
            {
                nlohmann::json level_json;
                level_json["level"] = level.level;
                level_json["indices"] = level.indices;
                json["levels"].push_back(std::move(level_json));
            }

            std::ofstream file;
            file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            file.open(state_path);
            file << json;
            file.close();
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
        std::string state_path;

        LevelManager(const std::string &path, const std::string &state_path, const Config &config, const State &state)
            : path(path), state_path(state_path), config(config), state(state) {}

        static std::string get_state_path(const std::string &path)
        {
            return path + "/state.json";
        }

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
