#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

// #define NINEDB_PROFILING

#include "../src/ninedb/kvdb.hpp"

using namespace ninedb;

Config get_test_config(bool delete_if_exists = true)
{
    Config config;
    config.max_buffer_size = 1 << 16;
    config.max_level_count = 2;
    config.writer.enable_lz4_compression = true;
    config.delete_if_exists = delete_if_exists;
    return config;
}

Config get_benchmark_config()
{
    Config config;
    config.delete_if_exists = true;
    // config.writer.max_node_children = 16;
    return config;
}

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

void test_get_by_key()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(10000, keys);
    generate_values_sequence(10000, values);

    KvDb db = KvDb::open("test_get_by_key", get_test_config());
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        db.add(keys[i], values[i]);
    }
    db.flush();

    std::string_view value;
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        bool found = db.get(keys[i], value);
        if (!found)
        {
            std::cout << "not found" << std::endl;
            exit(1);
        }
        if (value != values[i])
        {
            std::cout << "value mismatch" << std::endl;
            exit(1);
        }
    }

    std::cout << "test_get_by_key done" << std::endl;
}

void test_get_by_index()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(10000, keys);
    generate_values_sequence(10000, values);

    KvDb db = KvDb::open("test_get_by_index", get_test_config());
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        db.add(keys[i], values[i]);
    }
    db.flush();

    std::string_view key;
    std::string_view value;
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        bool found = db.at(i, key, value);
        if (!found)
        {
            std::cout << "not found" << std::endl;
            exit(1);
        }
        if (key != keys[i])
        {
            std::cout << "key mismatch" << std::endl;
            exit(1);
        }
        if (value != values[i])
        {
            std::cout << "value mismatch" << std::endl;
            exit(1);
        }
    }

    std::cout << "test_get_by_index done" << std::endl;
}

void test_iterator_begin()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(10000, keys);
    generate_values_sequence(10000, values);

    KvDb db = KvDb::open("test_iterator_begin", get_test_config());
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        db.add(keys[i], values[i]);
    }
    db.flush();

    Iterator it = db.begin();
    std::string_view key;
    std::string_view value;
    uint64_t count = 0;
    while (!it.is_end())
    {
        it.get_key(key);
        it.get_value(value);

        if (key != keys[count])
        {
            std::cout << "key mismatch" << std::endl;
            exit(1);
        }

        if (value != values[count])
        {
            std::cout << "value mismatch" << std::endl;
            exit(1);
        }

        it.next();
        count++;
    }

    if (count != keys.size())
    {
        std::cout << "count mismatch" << std::endl;
        exit(1);
    }

    std::cout << "test_iterator_begin done" << std::endl;
}

void test_iterator_seek_key()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(10000, keys);
    generate_values_sequence(10000, values);

    KvDb db = KvDb::open("test_iterator_seek_key", get_test_config());
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        db.add(keys[i], values[i]);
    }
    db.flush();

    Iterator it = db.seek(db.at(5000).value().first);
    std::string_view key;
    std::string_view value;
    uint64_t count = 5000;
    while (!it.is_end())
    {
        it.get_key(key);
        it.get_value(value);

        if (key != keys[count])
        {
            std::cout << "key mismatch" << std::endl;
            exit(1);
        }

        if (value != values[count])
        {
            std::cout << "value mismatch" << std::endl;
            exit(1);
        }

        it.next();
        count++;
    }

    if (count != keys.size())
    {
        std::cout << "count mismatch" << std::endl;
        exit(1);
    }

    std::cout << "test_iterator_seek_key done" << std::endl;
}

void test_iterator_seek_index()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(10000, keys);
    generate_values_sequence(10000, values);

    KvDb db = KvDb::open("test_iterator_seek_index", get_test_config());
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        db.add(keys[i], values[i]);
    }
    db.flush();

    Iterator it = db.seek(5000);
    std::string_view key;
    std::string_view value;
    uint64_t count = 5000;
    while (!it.is_end())
    {
        it.get_key(key);
        it.get_value(value);

        if (key != keys[count])
        {
            std::cout << "key mismatch" << std::endl;
            exit(1);
        }

        if (value != values[count])
        {
            std::cout << "value mismatch" << std::endl;
            exit(1);
        }

        it.next();
        count++;
    }

    if (count != keys.size())
    {
        std::cout << "count mismatch" << std::endl;
        exit(1);
    }

    std::cout << "test_iterator_seek_index done" << std::endl;
}

void test_iterator_end()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(10000, keys);
    generate_values_sequence(10000, values);

    KvDb db = KvDb::open("test_iterator_end", get_test_config());
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        db.add(keys[i], values[i]);
    }
    db.flush();

    Iterator it = db.seek("key_z");
    if (!it.is_end())
    {
        std::cout << "not end" << std::endl;
        exit(1);
    }

    std::cout << "test_iterator_end done" << std::endl;
}

void test_reopen()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(10000, keys);
    generate_values_sequence(10000, values);

    KvDb db1 = KvDb::open("test_reopen", get_test_config());
    for (uint64_t i = 0; i < keys.size() / 2; i++)
    {
        db1.add(keys[i], values[i]);
    }
    db1.flush();

    KvDb db2 = KvDb::open("test_reopen", get_test_config(false));
    for (uint64_t i = keys.size() / 2; i < keys.size(); i++)
    {
        db2.add(keys[i], values[i]);
    }
    db2.flush();

    std::string_view value;
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        bool found = db2.get(keys[i], value);
        if (!found)
        {
            std::cout << "not found" << std::endl;
            exit(1);
        }
        if (value != values[i])
        {
            std::cout << "value mismatch" << std::endl;
            exit(1);
        }
    }

    std::cout << "test_reopen done" << std::endl;
}

void benchmark_add()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(100000, keys);
    generate_values_sequence(100000, values);

    KvDb db = KvDb::open("benchmark_add", get_benchmark_config());
    auto t1 = std::chrono::high_resolution_clock::now();
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        db.add(keys[i], values[i]);
    }
    db.compact();
    auto t2 = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::cout << "benchmark_add: " << duration.count() << " μs" << std::endl;
}

void benchmark_get()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(100000, keys);
    generate_values_sequence(100000, values);

    KvDb db = KvDb::open("benchmark_get", get_benchmark_config());
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        db.add(keys[i], values[i]);
    }
    db.compact();

    auto t1 = std::chrono::high_resolution_clock::now();
    std::string_view value;
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        bool found = db.get(keys[i], value);
        if (!found)
        {
            std::cout << "not found" << std::endl;
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::cout << "benchmark_get: " << duration.count() << " μs" << std::endl;
}

void benchmark_iterator()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(100000, keys);
    generate_values_sequence(100000, values);

    KvDb db = KvDb::open("benchmark_iterator", get_benchmark_config());
    for (uint64_t i = 0; i < keys.size(); i++)
    {
        db.add(keys[i], values[i]);
    }
    db.compact();

    auto t1 = std::chrono::high_resolution_clock::now();
    auto it = db.begin();
    std::string_view key;
    std::string_view value;
    while (!it.is_end())
    {
        it.get_key(key);
        it.get_value(value);
        it.next();

        // prevent optimization
        if (rand() == 0xC0FFEE)
        {
            std::cout << key << " " << value << std::endl;
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::cout << "benchmark_iterator: " << duration.count() << " μs" << std::endl;
}

int main()
{
    test_get_by_key();
    test_get_by_index();
    test_iterator_begin();
    test_iterator_seek_key();
    test_iterator_seek_index();
    test_iterator_end();
    test_reopen();

    benchmark_add();
    benchmark_get();
    benchmark_iterator();

    return 0;
}
