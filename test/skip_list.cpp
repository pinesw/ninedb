#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

// #define NINEDB_PROFILING

#include "../src/ninedb/skip_list/skip_list.hpp"

using namespace ninedb;

void generate_keys_sequence(uint64_t count, std::vector<std::string> &keys)
{
    for (uint64_t i = 0; i < count; i++)
    {
        keys.push_back("key_" + std::to_string(i));
    }
    std::sort(keys.begin(), keys.end());
}

void generate_values_sequence(uint64_t count, std::vector<std::string> &values)
{
    for (uint64_t i = 0; i < count; i++)
    {
        values.push_back("value_" + std::to_string(i));
    }
    std::sort(values.begin(), values.end());
}

void add_all_after(const std::vector<std::string> &keys, const std::vector<std::string> &values, skip_list::SkipList &skip_list)
{
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        skip_list.add_after(keys[i], values[i]);
    }
}

void add_all_after(const std::vector<std::string> &keys, const std::vector<std::string> &values, std::multimap<std::string, std::string> &multimap)
{
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        multimap.insert({keys[i], values[i]});
    }
}

void test_seek_not_found()
{
    skip_list::SkipList skip_list;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(10000, keys);
    generate_values_sequence(10000, values);
    add_all_after(keys, values, skip_list);

    auto itr = skip_list.seek_first("key_not_found");
    if (itr != skip_list.end())
    {
        std::cout << "test_seek_not_found failed" << std::endl;
        exit(1);
    }

    itr = skip_list.seek_first("key");
    if (itr != skip_list.end())
    {
        std::cout << "test_seek_not_found failed" << std::endl;
        exit(1);
    }

    std::cout << "test_seek_not_found done" << std::endl;
}

void test_seek_iterator()
{
    skip_list::SkipList skip_list;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(10000, keys);
    generate_values_sequence(10000, values);
    add_all_after(keys, values, skip_list);

    std::string key;
    std::string_view value;
    auto itr = skip_list.seek_first(keys[5000]);
    auto end = skip_list.end();
    uint64_t count = 5000;
    while (itr != end)
    {
        itr.get_key(key);
        itr.get_value(value);

        if (key != keys[count])
        {
            std::cout << "key mismatch: " << key << " " << keys[count] << std::endl;
            exit(1);
        }
        if (value != values[count])
        {
            std::cout << "value mismatch: " << key << " " << value << " " << values[count] << std::endl;
            exit(1);
        }

        ++itr;
        count++;
    }

    if (count != keys.size())
    {
        std::cout << "count mismatch: " << count << " " << keys.size() << std::endl;
        exit(1);
    }

    std::cout << "test_seek_iterator done" << std::endl;
}

void test_duplicates_add_after()
{
    std::vector<std::pair<std::string, std::string>> entries;
    entries.push_back(std::make_pair("key_0", "value_0"));
    entries.push_back(std::make_pair("key_0", "value_1"));
    entries.push_back(std::make_pair("key_0", "value_2"));

    skip_list::SkipList skip_list;
    for (auto entry : entries)
    {
        skip_list.add_after(entry.first, entry.second);
    }

    std::string_view value;

    skip_list.get_first("key_0", value);
    if (value != "value_0")
    {
        std::cout << "test_duplicates_add_after failed" << std::endl;
        exit(1);
    }

    skip_list.get_last("key_0", value);
    if (value != "value_2")
    {
        std::cout << "test_duplicates_add_after failed" << std::endl;
        exit(1);
    }

    std::cout << "test_duplicates_add_after done" << std::endl;
}

void test_duplicates_add_before()
{
    std::vector<std::pair<std::string, std::string>> entries;
    entries.push_back(std::make_pair("key_0", "value_0"));
    entries.push_back(std::make_pair("key_0", "value_1"));
    entries.push_back(std::make_pair("key_0", "value_2"));

    skip_list::SkipList skip_list;
    for (auto entry : entries)
    {
        skip_list.add_before(entry.first, entry.second);
    }

    std::string_view value;

    skip_list.get_first("key_0", value);
    if (value != "value_2")
    {
        std::cout << "test_duplicates_add_before failed" << std::endl;
        exit(1);
    }

    skip_list.get_last("key_0", value);
    if (value != "value_0")
    {
        std::cout << "test_duplicates_add_before failed" << std::endl;
        exit(1);
    }

    std::cout << "test_duplicates_add_before done" << std::endl;
}

void benchmark_add()
{
    skip_list::SkipList skip_list;

    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
    {
        skip_list.add_after("key_" + std::to_string(i), "value_" + std::to_string(i));
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::cout << "benchmark_add: " << duration.count() << "μs" << std::endl;
}

void benchmark_add_multimap()
{
    std::multimap<std::string, std::string> multimap;

    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
    {
        multimap.insert({"key_" + std::to_string(i), "value_" + std::to_string(i)});
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::cout << "benchmark_add_multimap: " << duration.count() << "μs" << std::endl;
}

void benchmark_get_first()
{
    skip_list::SkipList skip_list;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(100000, keys);
    generate_values_sequence(100000, values);
    add_all_after(keys, values, skip_list);

    std::string_view value;
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
    {
        skip_list.get_first(keys[i], value);
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::cout << "benchmark_get_first: " << duration.count() << "μs" << std::endl;
}

void benchmark_get_first_multimap()
{
    std::multimap<std::string, std::string> multimap;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(100000, keys);
    generate_values_sequence(100000, values);
    add_all_after(keys, values, multimap);

    std::string_view value;
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
    {
        value = multimap.find(keys[i])->second;
        if (rand() == 0xdeadbeef) // prevent optimization
        {
            std::cout << value << std::endl;
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::cout << "benchmark_get_first_multimap: " << duration.count() << "μs" << std::endl;
}

void benchmark_get_last()
{
    skip_list::SkipList skip_list;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(100000, keys);
    generate_values_sequence(100000, values);
    add_all_after(keys, values, skip_list);

    std::string_view value;
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
    {
        skip_list.get_last(keys[i], value);
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::cout << "benchmark_get_last: " << duration.count() << "μs" << std::endl;
}

void benchmark_get_last_multimap()
{
    std::multimap<std::string, std::string> multimap;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(100000, keys);
    generate_values_sequence(100000, values);
    add_all_after(keys, values, multimap);

    std::string_view value;
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
    {
        value = (--multimap.equal_range(keys[i]).second)->second;
        if (rand() == 0xdeadbeef) // prevent optimization
        {
            std::cout << value << std::endl;
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::cout << "benchmark_get_last_multimap: " << duration.count() << "μs" << std::endl;
}

int main()
{
    // test_seek_iterator();
    // test_seek_not_found();
    // test_duplicates_add_after();
    // test_duplicates_add_before();

    benchmark_add();
    benchmark_add_multimap();
    benchmark_get_first();
    benchmark_get_first_multimap();
    benchmark_get_last();
    benchmark_get_last_multimap();

    return 0;
}
