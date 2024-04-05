#pragma once

#include <array>
#include <list>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "../detail/profiling.hpp"

#include "./detail/structure.hpp"
#include "./iterator.hpp"

namespace ninedb::skip_list
{
    struct SkipList
    {
    private:
        typedef typename std::list<detail::Entry> entry_list;
        typedef typename std::list<detail::Entry>::iterator list_iterator;

        constexpr static std::size_t MAX_LEVEL = 32;
        constexpr static std::double_t P = 0.5;

    public:
        SkipList(uint32_t seed = 0)
            : rng(std::mt19937(seed))
        {
            lists.reserve(MAX_LEVEL);
            lists.push_back(std::make_shared<entry_list>());
        }

        /**
         * Add a new key-value pair to the skip list.
         * If the key already exists, the new pair will be added at the end of the existing ones.
         */
        void add_after(std::string_view key, std::string_view value)
        {
            ZoneSkipList;

            detail::Entry entry{std::make_shared<std::string>(key), std::make_shared<std::string>(value)};
            uint64_t level = new_entry_level();

            std::array<list_iterator, MAX_LEVEL> result;
            search<true>(key, result);
            for (uint64_t i = 0; i < level; i++)
            {
                ZoneSkipListN("ninedb::skip_list::SkipList::add_after loop");

                entry.value = lists[i]->insert(result[i], entry);
            }

            size += key.size() + value.size();
        }

        /**
         * Add a new key-value pair to the skip list.
         * If the key already exists, the new pair will be inserted at the beginning of the existing ones.
         */
        void add_before(std::string_view key, std::string_view value)
        {
            ZoneSkipList;

            detail::Entry entry{std::make_shared<std::string>(key), std::make_shared<std::string>(value)};
            uint64_t level = new_entry_level();

            std::array<list_iterator, MAX_LEVEL> result;
            search<false>(key, result);
            for (uint64_t i = 0; i < level; i++)
            {
                ZoneSkipListN("ninedb::skip_list::SkipList::add_before loop");

                entry.value = lists[i]->insert(result[i], entry);
            }

            size += key.size() + value.size();
        }

        /**
         * Get the value of the given key.
         * If there are multiple values for the same key, the first one will be returned.
         * Returns true if the key exists, false otherwise.
         */
        bool get_first(std::string_view key, std::string_view &value) const
        {
            ZoneSkipList;

            std::array<list_iterator, MAX_LEVEL> result;
            search<false>(key, result);

            auto result_0 = result[0];
            if (result_0 == lists[0]->end() || result_0->key->compare(key) != 0)
            {
                return false;
            }
            else
            {
                value = *std::get<std::shared_ptr<std::string>>(result_0->value);
                return true;
            }
        }

        /**
         * Get the value of the given key.
         * If there are multiple values for the same key, the last one will be returned.
         * Returns true if the key exists, false otherwise.
         */
        bool get_last(std::string_view key, std::string_view &value) const
        {
            ZoneSkipList;

            std::array<list_iterator, MAX_LEVEL> result;
            search<true>(key, result);

            auto result_0 = result[0];
            if (result_0 == lists[0]->begin() || (--result_0)->key->compare(key) != 0)
            {
                return false;
            }
            else
            {
                value = *std::get<std::shared_ptr<std::string>>(result_0->value);
                return true;
            }
        }

        /**
         * Create a new iterator pointing to the first element in the skip list.
         */
        Iterator begin() const
        {
            return lists[0]->begin();
        }

        /**
         * Create a new iterator pointing to the first element with the given key in the skip list.
         * If the key does not exist, the iterator will point to the end of the list.
         */
        Iterator seek_first(std::string_view key) const
        {
            std::array<list_iterator, MAX_LEVEL> result;
            search<false>(key, result);

            auto result_0 = result[0];
            if (result_0 == lists[0]->end() || result_0->key->compare(key) != 0)
            {
                return lists[0]->end();
            }
            else
            {
                return result_0;
            }
        }

        /**
         * Create a new iterator pointing to end of the skip list.
         */
        Iterator end() const
        {
            return lists[0]->end();
        }

        /**
         * Clear the skip list.
         */
        void clear()
        {
            lists.clear();
            lists.push_back(std::make_shared<entry_list>());
            size = 0;
        }

        /**
         * Get the size of the skip list.
         * The size is the sum of the lengths of all keys and values.
         * This is to be used as an approximation of the memory usage of the key-value pairs.
         */
        uint64_t get_size() const
        {
            return size;
        }

    private:
        std::mt19937 rng;
        std::uniform_real_distribution<double> dist = std::uniform_real_distribution<double>(0, 1);
        std::vector<std::shared_ptr<entry_list>> lists;
        uint64_t size = 0;

        template <bool last>
        void search(std::string_view key, std::array<list_iterator, MAX_LEVEL> &result) const
        {
            ZoneSkipList;

            uint64_t size = lists.size();
            std::optional<list_iterator> prev_begin;
            std::optional<list_iterator> prev_end;
            list_iterator begin;
            list_iterator end;
            for (uint64_t i = size - 1; i < size; i--)
            {
                ZoneSkipListN("ninedb::skip_list::SkipList::search outer_loop");

                if (prev_begin.has_value())
                {
                    begin = std::get<list_iterator>(prev_begin.value()->value);
                }
                else
                {
                    begin = lists[i]->begin();
                }

                if (prev_end.has_value())
                {
                    end = std::get<list_iterator>(prev_end.value()->value);
                }
                else
                {
                    end = lists[i]->end();
                }

                prev_begin = std::nullopt;
                prev_end = std::nullopt;

                auto it = begin;
                while (it != end)
                {
                    ZoneSkipListN("ninedb::skip_list::SkipList::search inner_loop");

                    if (last && it->key->compare(key) > 0 || !last && it->key->compare(key) >= 0)
                    {
                        prev_end = it;
                        break;
                    }
                    prev_begin = it;
                    ++it;
                }
                result[i] = it;
            }
        }

        uint64_t new_entry_level()
        {
            ZoneSkipList;

            uint64_t level = 1;
            while (dist(rng) < P && level < MAX_LEVEL)
            {
                ++level;
            }
            if (level > lists.size())
            {
                for (uint64_t i = lists.size(); i < level; i++)
                {
                    lists.push_back(std::make_shared<entry_list>());
                }
            }
            return level;
        }
    };
}
