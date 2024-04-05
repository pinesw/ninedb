#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <limits>
#include <random>
#include <string>
#include <string_view>
#include <vector>

// #define NINEDB_PROFILING

#include "../src/ninedb/hrdb.hpp"

using namespace ninedb;

Config get_test_config()
{
    Config config;
    config.delete_if_exists = true;
    config.max_buffer_size = 1 << 16;
    config.max_level_count = 2;
    config.writer.enable_compression = true;
    config.writer.enable_prefix_encoding = true;
    return config;
}

Config get_benchmark_config()
{
    Config config;
    config.delete_if_exists = true;
    config.reader.leaf_node_cache_size = 0;
    // config.writer.enable_compression = true;
    // config.writer.enable_prefix_encoding = true;
    return config;
}

std::string pad_left(const std::string &value, uint32_t width, char pad)
{
    if (value.size() >= width)
    {
        return value;
    }
    return std::string(width - value.size(), pad) + std::string(value);
}

std::vector<std::string_view> brute_force_search(const std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> &bboxes, const std::vector<std::string> &values, uint32_t x, uint32_t y)
{
    std::vector<std::string_view> result;
    for (uint64_t i = 0; i < bboxes.size(); i++)
    {
        std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> bbox = bboxes[i];
        auto [x0, y0, x1, y1] = bbox;
        if (x0 <= x && x <= x1 && y0 <= y && y <= y1)
        {
            result.push_back(values[i]);
        }
    }
    return result;
}

std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> generate(uint32_t n, uint32_t size)
{
    std::mt19937 rng(0);
    std::uniform_real_distribution<double> dist(0, 1);
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> bboxes;
    for (uint32_t i = 0; i < n; i++)
    {
        uint32_t x = (uint32_t)(dist(rng) * (100u - size));
        uint32_t y = (uint32_t)(dist(rng) * (100u - size));
        uint32_t w = (uint32_t)(dist(rng) * size);
        uint32_t h = (uint32_t)(dist(rng) * size);
        bboxes.push_back({x, y, x + w, y + h});
    }
    return bboxes;
}

void test_empty_db()
{
    HrDb db = HrDb::open("test_empty_db", get_test_config());
    db.flush();

    std::vector<std::string_view> result = db.search(0, 0, std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max());
    assert(result.size() == 0);

    std::cout << "test_empty_db done" << std::endl;
}

void test_small_db()
{
    HrDb db = HrDb::open("test_small_db", get_test_config());
    db.add(0, 0, "value_0");
    db.add(1, 0, "value_1");
    db.add(0, 1, "value_2");
    db.add(1, 1, "value_3");
    db.flush();

    {
        std::vector<std::string_view> result = db.search(0, 0, 0, 0);
        assert(result.size() == 1);
        assert(result[0] == "value_0");
    }

    {
        std::vector<std::string_view> result = db.search(1, 0, 1, 0);
        assert(result.size() == 1);
        assert(result[0] == "value_1");
    }

    {
        std::vector<std::string_view> result = db.search(0, 1, 0, 1);
        assert(result.size() == 1);
        assert(result[0] == "value_2");
    }

    {
        std::vector<std::string_view> result = db.search(1, 1, 1, 1);
        assert(result.size() == 1);
        assert(result[0] == "value_3");
    }

    {
        std::vector<std::string_view> result = db.search(0, 0, 1, 1);
        assert(result.size() == 4);
        assert(result[0] == "value_0");
        assert(result[1] == "value_1");
        assert(result[2] == "value_3");
        assert(result[3] == "value_2");
    }

    std::cout << "test_small_db done" << std::endl;
}

void test_random_db()
{
    uint32_t width = 500;
    uint32_t height = 500;
    uint32_t spread_x = 10;
    uint32_t spread_y = 10;
    uint32_t offset_x = 10;
    uint32_t offset_y = 10;
    std::mt19937 rng(0);
    std::uniform_real_distribution<double> dist(0, 1);

    std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> bboxes;
    std::vector<std::string> values;
    std::vector<std::tuple<uint32_t, uint32_t>> probes;
    for (int i = 0; i < 1000; i++)
    {
        uint32_t x = (uint32_t)(dist(rng) * width);
        uint32_t y = (uint32_t)(dist(rng) * height);
        uint32_t x0 = offset_x + x - (spread_x / 2);
        uint32_t y0 = offset_y + y - (spread_y / 2);
        uint32_t x1 = offset_x + x + (spread_x / 2);
        uint32_t y1 = offset_y + y + (spread_y / 2);
        bboxes.push_back({x0, y0, x1, y1});
        values.push_back("value_" + pad_left(std::to_string(i), 3, '0'));
    }
    for (int i = 0; i < 1000; i++)
    {
        uint32_t x = (uint32_t)(dist(rng) * width);
        uint32_t y = (uint32_t)(dist(rng) * height);
        probes.push_back({x, y});
    }

    HrDb db = HrDb::open("test_random_db", get_test_config());
    for (uint64_t i = 0; i < bboxes.size(); i++)
    {
        std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> bbox = bboxes[i];
        auto [x0, y0, x1, y1] = bbox;
        db.add(x0, y0, x1, y1, values[i]);
    }
    db.flush();

    for (uint64_t i = 0; i < probes.size(); i++)
    {
        auto [x, y] = probes[i];
        std::vector<std::string_view> expected = brute_force_search(bboxes, values, x, y);
        std::vector<std::string_view> actual = db.search(x, y);
        std::sort(actual.begin(), actual.end());
        assert(actual.size() == expected.size());
        for (uint64_t j = 0; j < actual.size(); j++)
        {
            assert(actual[j] == expected[j]);
        }
    }

    std::cout << "test_random_db done" << std::endl;
}

void benchmark_add()
{
    auto data = generate(10000, 1);

    HrDb db = HrDb::open("benchmark_add", get_benchmark_config());
    auto t1 = std::chrono::high_resolution_clock::now();
    for (uint64_t i = 0; i < data.size(); i++)
    {
        auto [x0, y0, x1, y1] = data[i];
        db.add(x0, y0, x1, y1, "value_" + std::to_string(i));
    }
    db.compact();
    auto t2 = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::cout << "benchmark_add: " << duration.count() << " μs" << std::endl;
}

std::chrono::microseconds benchmark_search_sub(const HrDb &db, const std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> &probes)
{
    auto t1 = std::chrono::high_resolution_clock::now();
    for (uint64_t i = 0; i < probes.size(); i++)
    {
        auto [x0, y0, x1, y1] = probes[i];
        db.search(x0, y0, x1, y1);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
}

void benchmark_search()
{
    auto data = generate(10000, 1);
    auto bboxes100 = generate(1000, (uint32_t)(100 * sqrt(0.1)));
    auto bboxes10 = generate(1000, 10);
    auto bboxes1 = generate(1000, 1);

    HrDb db = HrDb::open("benchmark_search", get_benchmark_config());
    for (uint64_t i = 0; i < data.size(); i++)
    {
        auto [x0, y0, x1, y1] = data[i];
        db.add(x0, y0, x1, y1, "value_" + std::to_string(i));
    }
    db.compact();

    auto duration_100 = benchmark_search_sub(db, bboxes100);
    std::cout << "benchmark_search (1000 x 10%): " << duration_100.count() << " μs" << std::endl;

    auto duration_10 = benchmark_search_sub(db, bboxes10);
    std::cout << "benchmark_search (1000 x 1%): " << duration_10.count() << " μs" << std::endl;

    auto duration_1 = benchmark_search_sub(db, bboxes1);
    std::cout << "benchmark_search (1000 x 0.01%): " << duration_1.count() << " μs" << std::endl;
}

int main()
{
    test_empty_db();
    test_small_db();
    test_random_db();

    // benchmark_add();
    // benchmark_search();

    return 0;
}
