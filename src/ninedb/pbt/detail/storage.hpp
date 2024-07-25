#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "../../detail/profiling.hpp"

namespace ninedb::pbt::detail
{
    struct Storage
    {
        /**
         * Create a new storage at the given path.
         */
        Storage(const std::string &path, bool read_only)
            : path(path), read_only(read_only)
        {
            if (!std::filesystem::exists(path))
            {
                throw std::runtime_error("File does not exist");
            }

            load_mmap();
        }

        ~Storage()
        {
            if (region != nullptr)
            {
                flush();
                unload_mmap();
            }
        }

        Storage(const Storage &) = delete;
        Storage &operator=(const Storage &) = delete;

        /**
         * Get the memory address of the beginning of the storage.
         */
        void *get_address() const
        {
            ZonePbtStorage;

            return address;
        }

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
