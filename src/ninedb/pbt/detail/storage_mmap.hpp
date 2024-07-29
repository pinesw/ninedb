#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "../../detail/profiling.hpp"

namespace ninedb::pbt::detail
{
    struct StorageMmap
    {
        /**
         * Create a new storage at the given path.
         */
        StorageMmap(const std::string &path, bool read_only)
            : path(path), read_only(read_only)
        {
            if (!std::filesystem::exists(path))
            {
                throw std::runtime_error("File does not exist");
            }

            load_mmap();
        }

        ~StorageMmap()
        {
            if (region != nullptr)
            {
                flush();
                unload_mmap();
            }
        }

        StorageMmap(const StorageMmap &) = delete;
        StorageMmap &operator=(const StorageMmap &) = delete;

        /**
         * Get the length of the storage in bytes.
         */
        std::size_t get_size() const
        {
            ZonePbtStorage;

            return region->get_size();
        }

        /**
         * Set the length of the storage in bytes.
         */
        void set_size(std::size_t size)
        {
            ZonePbtStorage;

            if (region != nullptr)
            {
                if (region->get_size() == size)
                {
                    return;
                }
                region->flush();
                unload_mmap();
            }
            std::filesystem::resize_file(path, size);
            load_mmap();
        }

        /**
         * Set all bytes in the storage to zero.
         */
        void clear()
        {
            ZonePbtStorage;

            memset(address, 0, region->get_size());
        }

        /**
         * Flush the storage to disk.
         */
        void flush() const
        {
            ZonePbtStorage;

            if (region != nullptr)
            {
                region->flush();
            }
        }

        // /**
        //  * Get the memory address of the beginning of the storage.
        //  */
        // void *get_address() const
        // {
        //     ZonePbtStorage;

        //     return address;
        // }

        /**
         * Read data from the storage.
         */
        size_t read(std::size_t offset, std::size_t size, uint8_t *result)
        {
            ZonePbtStorage;

            if (offset + size > region->get_size())
            {
                throw std::runtime_error("Read out of bounds");
            }
            memcpy(result, (uint8_t *)address + offset, size);
            return size;
        }

    private:
        std::string path;
        bool read_only;
        boost::interprocess::file_mapping *mapping = nullptr;
        boost::interprocess::mapped_region *region = nullptr;
        void *address = nullptr;

        void load_mmap()
        {
            ZonePbtStorage;

            {
                size_t size = std::filesystem::file_size(path);
                mapping = new boost::interprocess::file_mapping(path.c_str(), boost::interprocess::read_write);
                region = new boost::interprocess::mapped_region(*mapping, boost::interprocess::read_write, 0, size);
                address = region->get_address();
            }
        }

        void unload_mmap()
        {
            ZonePbtStorage;

            if (region != nullptr)
            {
                delete region;
                delete mapping;
            }
            region = nullptr;
            mapping = nullptr;
        }
    };
}
