#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include <boost/endian/conversion.hpp>

#include "./detail/profiling.hpp"

#include "./kvdb.hpp"
#include "./detail/hilbert.hpp"
#include "./config.hpp"

namespace ninedb
{
    struct HrDb
    {
        static HrDb open(const std::string &path, const Config &config)
        {
            ZoneDb;

            if (config.writer.reduce != nullptr)
            {
                throw std::runtime_error("reduce function is not allowed for HrDb");
            }

            Config new_config = config;
            new_config.writer.reduce = HrDb::reduce;
            return HrDb(KvDb::open(path, new_config));
        }

        /**
         * Add a value to the spatial db.
         * The value will be associated with the given x and y coordinates.
         */
        void add(uint32_t x, uint32_t y, std::string_view value)
        {
            ZoneDb;

            uint32_t h = detail::hilbert_xy_to_index(16, x, y);
            uint32_t h_be = boost::endian::endian_reverse(h);
            std::string_view key((char *)&h_be, sizeof(h_be));
            kvdb.add(key, pad_value_with_bbox(x, y, x, y, value));
        }

        /**
         * Add a value to the spatial db.
         * The value will be associated with the given bounding box.
         */
        void add(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, std::string_view value)
        {
            ZoneDb;

            uint32_t x = average(x0, x1);
            uint32_t y = average(y0, y1);
            uint32_t h = detail::hilbert_xy_to_index(16, x, y);
            uint32_t h_be = boost::endian::endian_reverse(h);
            std::string_view key((char *)&h_be, sizeof(h_be));
            kvdb.add(key, pad_value_with_bbox(x0, y0, x1, y1, value));
        }

        /**
         * Search for values in the spatial db.
         * The search will be performed using the given bounding box.
         */
        std::vector<std::string_view> search(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1) const
        {
            ZoneDb;

            std::vector<std::string_view> values;
            kvdb.traverse(
                [x0, y0, x1, y1](std::string_view value)
                {
                    uint32_t *ptr = (uint32_t *)value.data();
                    uint32_t bx0 = ptr[0];
                    uint32_t by0 = ptr[1];
                    uint32_t bx1 = ptr[2];
                    uint32_t by1 = ptr[3];
                    return intersects(x0, y0, x1, y1, bx0, by0, bx1, by1);
                },
                values);
            for (std::string_view &value : values)
            {
                value.remove_prefix(4 * sizeof(uint32_t));
            }
            return values;
        }

        /**
         * Compact the db.
         * This will merge all the files in the db into a single file.
         */
        void compact()
        {
            ZoneDb;

            kvdb.compact();
        }

        /**
         * Flush the buffer to disk and perform any necessary merges.
         */
        void flush()
        {
            ZoneDb;

            kvdb.flush();
        }

    private:
        KvDb kvdb;

        HrDb(KvDb &&kvdb)
            : kvdb(std::move(kvdb)) {}

        static std::string pad_value_with_bbox(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, std::string_view value)
        {
            ZoneDb;

            std::string new_value;
            new_value.resize(4 * sizeof(uint32_t) + value.size());
            uint32_t *ptr = (uint32_t *)new_value.data();
            ptr[0] = x0;
            ptr[1] = y0;
            ptr[2] = x1;
            ptr[3] = y1;
            std::memcpy(new_value.data() + 4 * sizeof(uint32_t), value.data(), value.size());
            return new_value;
        }

        static uint32_t average(uint32_t a, uint32_t b)
        {
            ZoneDb;

            return (a / 2) + (b / 2) + (a & b & 1);
        }

        static bool intersects(uint32_t ax0, uint32_t ay0, uint32_t ax1, uint32_t ay1, uint32_t bx0, uint32_t by0, uint32_t bx1, uint32_t by1)
        {
            ZoneDb;

            return bx0 <= ax1 && bx1 >= ax0 && by0 <= ay1 && by1 >= ay0;
        }

        static void reduce(const std::vector<std::string> &values, std::string &reduced_value)
        {
            ZoneDb;

            uint32_t x0 = std::numeric_limits<uint32_t>::max();
            uint32_t y0 = std::numeric_limits<uint32_t>::max();
            uint32_t x1 = std::numeric_limits<uint32_t>::min();
            uint32_t y1 = std::numeric_limits<uint32_t>::min();
            for (const auto &value : values)
            {
                uint32_t *ptr = (uint32_t *)value.data();
                x0 = std::min(x0, ptr[0]);
                y0 = std::min(y0, ptr[1]);
                x1 = std::max(x1, ptr[2]);
                y1 = std::max(y1, ptr[3]);
            }
            reduced_value.resize(4 * sizeof(uint32_t));
            uint32_t *ptr = (uint32_t *)reduced_value.data();
            ptr[0] = x0;
            ptr[1] = y0;
            ptr[2] = x1;
            ptr[3] = y1;
        }
    };
}
