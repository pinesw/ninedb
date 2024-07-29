#pragma once

#include <cstdio>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "../../detail/profiling.hpp"
#include "../../detail/rlru_cache.hpp"

namespace ninedb::pbt::detail
{
    struct StorageFile
    {
        /**
         * Create a new storage at the given path.
         */
        StorageFile(const std::string &path, bool read_only)
            : path(path), read_only(read_only), cache(256)
        {
            if (!std::filesystem::exists(path))
            {
                throw std::runtime_error("File does not exist");
            }

            open_file();
        }

        ~StorageFile()
        {
            if (file != nullptr)
            {
                flush();
                close_file();
            }
        }

        StorageFile(const StorageFile &) = delete;
        StorageFile &operator=(const StorageFile &) = delete;

        /**
         * Get the length of the storage in bytes.
         */
        std::size_t get_size() const
        {
            ZonePbtStorage;

            return size;
        }

        /**
         * Set the length of the storage in bytes.
         */
        void set_size(std::size_t size)
        {
            ZonePbtStorage;

            if (file != nullptr)
            {
                if (this->size == size)
                {
                    return;
                }
                flush();
                close_file();
            }
            std::filesystem::resize_file(path, size);
            open_file();
        }

        /**
         * Set all bytes in the storage to zero.
         */
        void clear()
        {
            ZonePbtStorage;

            if (file != nullptr)
            {
                std::fseek(file, 0, SEEK_SET);
                const std::vector<char> zeroes(4096, 0);
                std::size_t size = this->size;
                while (size > 0)
                {
                    std::size_t write_size = std::min(zeroes.size(), size);
                    std::fwrite(zeroes.data(), 1, write_size, file);
                    size -= write_size;
                }
                std::fflush(file);
            }
        }

        /**
         * Flush the storage to disk.
         */
        void flush() const
        {
            ZonePbtStorage;

            if (file != nullptr)
            {
                std::fflush(file);
            }
        }

        /**
         * Read data from the storage.
         */
        void read(std::size_t offset, std::size_t size, uint8_t *result)
        {
            ZonePbtStorage;

            if (file == nullptr)
            {
                throw std::runtime_error("File is not open");
            }
            if (offset + size > this->size)
            {
                throw std::runtime_error("Read out of bounds");
            }

            std::size_t read_size = size;
            while (read_size > 0)
            {
                std::size_t cache_index = offset / 4096;
                std::shared_ptr<uint8_t[4096]> cache_data;
                if (!cache.try_get(cache_index, cache_data))
                {
                    cache_data = std::make_shared<uint8_t[4096]>();
                    std::fseek(file, cache_index * 4096, SEEK_SET);
                    std::fread(cache_data.get(), 1, 4096, file);
                    cache.put(cache_index, cache_data);
                }
                std::size_t cache_offset = offset % 4096;
                std::size_t cache_size = std::min(read_size, 4096 - cache_offset);
                std::memcpy(result, cache_data.get() + cache_offset, cache_size);
                offset += cache_size;
                result += cache_size;
                read_size -= cache_size;
            }
        }

    private:
        std::string path;
        bool read_only;
        ninedb::detail::RLRUCache<size_t, std::shared_ptr<uint8_t[4096]>> cache;
        std::FILE *file = nullptr;
        size_t size = 0;

        void open_file()
        {
            ZonePbtStorage;

            if (read_only)
            {
                file = std::fopen(path.c_str(), "rb");
            }
            else
            {
                file = std::fopen(path.c_str(), "r+b");
            }
            if (file == nullptr)
            {
                throw std::runtime_error("Failed to open file");
            }
            size = std::filesystem::file_size(path);
        }

        void close_file()
        {
            ZonePbtStorage;

            if (file != nullptr)
            {
                std::fclose(file);
                file = nullptr;
                size = 0;
            }
        }
    };
}
