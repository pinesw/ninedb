#include <chrono>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>

// #define NINEDB_PROFILING

#include "../src/ninedb/pbt/pbt.hpp"

using namespace ninedb;

pbt::WriterConfig get_writer_config()
{
    pbt::WriterConfig config;
    return config;
}

pbt::ReaderConfig get_reader_config()
{
    pbt::ReaderConfig config;
    return config;
}

void generate_keys_sequence(std::vector<std::string> &keys, int num_entries)
{
    std::size_t max_size = std::to_string(num_entries - 1).size();
    for (int i = 0; i < num_entries; i++)
    {
        std::string digits = std::to_string(i);
        digits.insert(digits.begin(), max_size - digits.size(), '0');
        std::string key = "key_" + digits;
        keys.push_back(key);
    }
}

void generate_values_sequence(std::vector<std::string> &values, int num_entries)
{
    std::size_t max_size = std::to_string(num_entries - 1).size();
    for (int i = 0; i < num_entries; i++)
    {
        std::string digits = std::to_string(i);
        digits.insert(digits.begin(), max_size - digits.size(), '0');
        std::string key = "value_" + digits;
        values.push_back(key);
    }
}

void generate_even_keys(std::vector<std::string> &keys, int num_entries)
{
    std::size_t max_size = std::to_string(num_entries * 2 - 1).size();
    for (int i = 0; i < num_entries; i++)
    {
        std::string digits = std::to_string(i * 2);
        digits.insert(digits.begin(), max_size - digits.size(), '0');
        std::string key = "key_" + digits;
        keys.push_back(key);
    }
}

void generate_odd_keys(std::vector<std::string> &keys, int num_entries)
{
    std::size_t max_size = std::to_string(num_entries * 2 - 1).size();
    for (int i = 0; i < num_entries; i++)
    {
        std::string digits = std::to_string(i * 2 + 1);
        digits.insert(digits.begin(), max_size - digits.size(), '0');
        std::string key = "key_" + digits;
        keys.push_back(key);
    }
}

void write_key_value_pairs(pbt::Writer &writer, const std::vector<std::string> &keys, const std::vector<std::string> &values)
{
    for (int i = 0; i < keys.size(); i++)
    {
        writer.add(keys[i], values[i]);
    }
    writer.finish();
}

void test_merge()
{
    std::vector<std::string> keys_0;
    std::vector<std::string> keys_1;
    std::vector<std::string> values;
    generate_even_keys(keys_0, 5000);
    generate_odd_keys(keys_1, 5000);
    generate_values_sequence(values, 5000);

    std::string file_path_1 = "test_merge_1.pbt";
    std::string file_path_2 = "test_merge_2.pbt";
    std::string file_path_3 = "test_merge_3.pbt";
    pbt::Writer writer_1(file_path_1, get_writer_config());
    pbt::Writer writer_2(file_path_2, get_writer_config());
    pbt::Writer writer_3(file_path_3, get_writer_config());

    write_key_value_pairs(writer_1, keys_0, values);
    write_key_value_pairs(writer_2, keys_1, values);

    std::vector<std::shared_ptr<pbt::Reader>> readers{
        std::make_shared<pbt::Reader>(writer_1.to_reader(get_reader_config())),
        std::make_shared<pbt::Reader>(writer_2.to_reader(get_reader_config()))};
    writer_3.merge(readers);
    writer_3.finish();

    pbt::Reader reader_3 = writer_3.to_reader(get_reader_config());

    for (int i = 0; i < keys_0.size(); i++)
    {
        std::string_view value_0;
        std::string_view value_1;

        reader_3.get(keys_0[i], value_0);
        reader_3.get(keys_1[i], value_1);

        if (value_0 != values[i])
        {
            std::cout << "value mismatch: " << keys_0[i] << " " << value_0 << " " << values[i] << std::endl;
            exit(1);
        }
        if (value_1 != values[i])
        {
            std::cout << "value mismatch: " << keys_1[i] << " " << value_1 << " " << values[i] << std::endl;
            exit(1);
        }
    }

    for (int i = 0; i < keys_0.size(); i++)
    {
        std::string key_0;
        std::string key_1;
        std::string_view value_0;
        std::string_view value_1;

        reader_3.at(2 * i, key_0, value_0);
        reader_3.at(2 * i + 1, key_1, value_1);

        if (key_0 != keys_0[i])
        {
            std::cout << "key mismatch: " << key_0 << " " << keys_0[i] << std::endl;
            exit(1);
        }
        if (key_1 != keys_1[i])
        {
            std::cout << "key mismatch: " << key_1 << " " << keys_1[i] << std::endl;
            exit(1);
        }
    }

    std::cout << "test_merge done" << std::endl;
}

void test_reduce()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(keys, 100);
    generate_values_sequence(values, 100);

    pbt::WriterConfig config;
    config.max_node_entries = 4;
    config.reduce = [](const std::vector<std::string> &values, std::string &reduced_value)
    {
        std::string_view result;
        for (auto &value : values)
        {
            result = value;
        }
        reduced_value = std::string(result);
    };

    std::string file_path = "test_reduce.pbt";

    pbt::Writer writer(file_path, config);
    write_key_value_pairs(writer, keys, values);
    pbt::Reader reader = writer.to_reader(get_reader_config());

    std::vector<std::string_view> accumulator;
    reader.traverse([](std::string_view value)
                    { 
                        std::cout << "value: " << value << std::endl; 
                        return true; },
                    accumulator);

    std::cout << "test_reduce done" << std::endl;
}

void test_duplicate_keys()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(keys, 100);
    generate_values_sequence(values, 5);

    std::vector<std::pair<std::string, std::string>> entries;
    for (int i = 0; i < keys.size(); i++)
    {
        for (int j = 0; j < values.size(); j++)
        {
            entries.push_back(std::make_pair(keys[i], values[j]));
        }
    }

    std::string file_path = "test_duplicate_keys.pbt";
    pbt::Writer writer(file_path, get_writer_config());

    for (auto entry : entries)
    {
        writer.add(entry.first, entry.second);
    }
    writer.finish();

    pbt::Reader reader = writer.to_reader(get_reader_config());

    for (auto entry : entries)
    {
        std::string_view value;
        reader.get(entry.first, value);

        if (value != values[0])
        {
            std::cout << "value mismatch: " << entry.first << " " << value << " " << entry.second << std::endl;
            exit(1);
        }
    }

    for (int i = 0; i < entries.size(); i++)
    {
        std::string key;
        std::string_view value;
        reader.at(i, key, value);

        if (key != entries[i].first)
        {
            std::cout << "key mismatch: " << key << " " << entries[i].first << std::endl;
            exit(1);
        }
        if (value != entries[i].second)
        {
            std::cout << "value mismatch: " << key << " " << value << " " << entries[i].second << std::endl;
            exit(1);
        }
    }

    std::cout << "test_duplicate_keys done" << std::endl;
}

void benchmark_add()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(keys, 100000);
    generate_values_sequence(values, 100000);

    std::string file_path = "benchmark_add.pbt";
    pbt::Writer writer(file_path, get_writer_config());
    auto t1 = std::chrono::high_resolution_clock::now();
    write_key_value_pairs(writer, keys, values);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::cout << "benchmark_add: " << duration1.count() << "μs" << std::endl;
}

void test_get_by_key()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(keys, 10000);
    generate_values_sequence(values, 10000);

    std::string file_path = "test_get_by_key.pbt";
    pbt::Writer writer(file_path, get_writer_config());
    write_key_value_pairs(writer, keys, values);

    pbt::Reader reader = writer.to_reader(get_reader_config());

    std::string_view value;
    for (int i = 0; i < keys.size(); i++)
    {
        bool found = reader.get(std::string_view(keys[i]), value);
        if (!found)
        {
            std::cout << "not found: " << keys[i] << std::endl;
            exit(1);
        }
        if (value != values[i])
        {
            std::cout << "value mismatch: " << keys[i] << " " << value << " " << values[i] << std::endl;
            exit(1);
        }
    }

    std::vector<std::string> keys_not_found;
    generate_keys_sequence(keys_not_found, 1000);
    for (int i = 0; i < keys_not_found.size(); i++)
    {
        bool found = reader.get(std::string_view(keys_not_found[i]), value);
        if (found)
        {
            std::cout << "found: " << keys_not_found[i] << std::endl;
            exit(1);
        }
    }

    std::cout << "test_get_by_key done" << std::endl;
}

void test_get_by_index()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(keys, 10000);
    generate_values_sequence(values, 10000);

    std::string file_path = "test_get_by_index.pbt";
    pbt::Writer writer(file_path, get_writer_config());
    write_key_value_pairs(writer, keys, values);

    pbt::Reader reader = writer.to_reader(get_reader_config());

    std::string key;
    std::string_view value;
    for (int i = 0; i < keys.size(); i++)
    {
        bool found = reader.at(i, key, value);
        if (!found)
        {
            std::cout << "not found: " << keys[i] << std::endl;
            exit(1);
        }
        if (key != keys[i])
        {
            std::cout << "key mismatch: " << key << " " << keys[i] << std::endl;
            exit(1);
        }
        if (value != values[i])
        {
            std::cout << "value mismatch: " << keys[i] << " " << value << " " << values[i] << std::endl;
            exit(1);
        }
    }

    std::vector<uint64_t> indices_not_found{10000, 10001, 10002, 10010, 10100};
    for (int i = 0; i < indices_not_found.size(); i++)
    {
        bool found = reader.at(indices_not_found[i], key, value);
        if (found)
        {
            std::cout << "found: " << indices_not_found[i] << std::endl;
            exit(1);
        }
    }

    std::cout << "test_get_by_index done" << std::endl;
}

void benchmark_get_by_key()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(keys, 100000);
    generate_values_sequence(values, 100000);

    std::string file_path = "benchmark_get_by_key.pbt";
    pbt::Writer writer(file_path, get_writer_config());
    write_key_value_pairs(writer, keys, values);

    pbt::Reader reader = writer.to_reader(get_reader_config());

    std::string_view value;
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < keys.size(); i++)
    {
        bool found = reader.get(std::string_view(keys[i]), value);
        if (!found)
        {
            std::cout << "not found: " << keys[i] << std::endl;
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::cout << "benchmark_get_by_key: " << duration.count() << "μs" << std::endl;
}

void benchmark_get_by_index()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(keys, 100000);
    generate_values_sequence(values, 100000);

    std::string file_path = "benchmark_get_by_index.pbt";
    pbt::Writer writer(file_path, get_writer_config());
    write_key_value_pairs(writer, keys, values);

    pbt::Reader reader = writer.to_reader(get_reader_config());

    std::string key;
    std::string_view value;
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < keys.size(); i++)
    {
        bool found = reader.at(i, key, value);
        if (!found)
        {
            std::cout << "not found: " << keys[i] << std::endl;
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::cout << "benchmark_get_by_index: " << duration.count() << "μs" << std::endl;
}

void benchmark_iterator()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(keys, 100000);
    generate_values_sequence(values, 100000);

    std::string file_path = "benchmark_iterator.pbt";
    pbt::Writer writer(file_path, get_writer_config());
    write_key_value_pairs(writer, keys, values);

    pbt::Reader reader = writer.to_reader(get_reader_config());

    std::string key;
    std::string_view value;
    auto t1 = std::chrono::high_resolution_clock::now();
    auto itr = reader.begin();
    while (!itr.is_end())
    {
        itr.get_key(key);
        itr.get_value(value);
        itr.next();

        // prevent optimization
        if (rand() == 0xC0FFEE)
        {
            std::cout << key << " " << value << std::endl;
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::cout << "benchmark_iterator: " << duration.count() << "μs" << std::endl;
}

void test_iterator_begin()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(keys, 10000);
    generate_values_sequence(values, 10000);

    std::string file_path = "test_iterator_begin.pbt";
    pbt::Writer writer(file_path, get_writer_config());
    write_key_value_pairs(writer, keys, values);

    pbt::Reader reader = writer.to_reader(get_reader_config());

    std::string key;
    std::string_view value;
    auto itr = reader.begin();
    uint64_t count = 0;
    while (!itr.is_end())
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

        itr.next();
        count++;
    }

    if (count != keys.size())
    {
        std::cout << "count mismatch: " << count << " " << keys.size() << std::endl;
        exit(1);
    }

    std::cout << "test_iterator_begin done" << std::endl;
}

void test_iterator_seek_key()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(keys, 10000);
    generate_values_sequence(values, 10000);

    std::string file_path = "test_iterator_seek_key.pbt";
    pbt::Writer writer(file_path, get_writer_config());
    write_key_value_pairs(writer, keys, values);

    pbt::Reader reader = writer.to_reader(get_reader_config());

    std::string key;
    std::string_view value;
    auto itr = reader.seek_first(keys[5000]);
    uint64_t count = 5000;
    while (!itr.is_end())
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

        itr.next();
        count++;
    }

    if (count != keys.size())
    {
        std::cout << "count mismatch: " << count << " " << keys.size() << std::endl;
        exit(1);
    }

    std::cout << "test_iterator_seek_key done" << std::endl;
}

void test_iterator_seek_index()
{
    std::vector<std::string> keys;
    std::vector<std::string> values;
    generate_keys_sequence(keys, 10000);
    generate_values_sequence(values, 10000);

    std::string file_path = "test_iterator_seek_index.pbt";
    pbt::Writer writer(file_path, get_writer_config());
    write_key_value_pairs(writer, keys, values);

    pbt::Reader reader = writer.to_reader(get_reader_config());

    std::string key;
    std::string_view value;
    auto itr = reader.seek(5000);
    uint64_t count = 5000;
    while (!itr.is_end())
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

        itr.next();
        count++;
    }

    if (count != keys.size())
    {
        std::cout << "count mismatch: " << count << " " << keys.size() << std::endl;
        exit(1);
    }

    std::cout << "test_iterator_seek_index done" << std::endl;
}

int main()
{
    test_merge();
    // test_reduce();
    test_duplicate_keys();

    test_get_by_key();
    test_get_by_index();
    test_iterator_begin();
    test_iterator_seek_key();
    test_iterator_seek_index();

    // benchmark_add();
    // benchmark_get_by_key();
    // benchmark_get_by_index();
    // benchmark_iterator();

    return 0;
}
