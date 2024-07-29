#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "./format.hpp"
#include "./storage.hpp"

namespace ninedb::pbt::detail
{
    constexpr uint32_t VERSION_MAJOR = 0;
    constexpr uint32_t VERSION_MINOR = 1;

#pragma pack(push, 1)
    struct Footer
    {
        static const uint32_t MAGIC = 0x1EAF1111;

        // Root node handle.
        uint64_t root_offset;
        uint64_t root_size;

        // Tree structure values.
        uint64_t level_0_end; // TODO: remove me. Reader can go `tree_height` nodes to the right to find the last leaf.
        uint16_t tree_height;

        // Metadata values.
        uint64_t global_start;
        uint64_t global_end;

        // Versioning values.
        uint16_t version_major;
        uint16_t version_minor;

        // Magic value.
        uint32_t magic;

        Footer()
        {
            ZonePbtStructures;

            this->version_major = VERSION_MAJOR;
            this->version_minor = VERSION_MINOR;
            this->magic = MAGIC;
        }

        uint64_t write(uint64_t offset, Storage &storage) const
        {
            ZonePbtStructures;

            std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(Footer::size_of());
            uint8_t *address = buffer.get();

            address += Format::write_uint64(address, this->root_offset);
            address += Format::write_uint64(address, this->root_size);
            address += Format::write_uint64(address, this->level_0_end);
            address += Format::write_uint16(address, this->tree_height);
            address += Format::write_uint64(address, this->global_start);
            address += Format::write_uint64(address, this->global_end);
            address += Format::write_uint16(address, this->version_major);
            address += Format::write_uint16(address, this->version_minor);
            address += Format::write_uint32(address, this->magic);

            storage.write(offset, buffer.get(), Footer::size_of());

            return Footer::size_of();
        }

        static uint64_t read(uint64_t offset, Storage &storage, Footer &footer)
        {
            ZonePbtFormat;

            std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(Footer::size_of());
            uint8_t *address = buffer.get();

            storage.read(offset, buffer.get(), Footer::size_of());

            return Footer::read(address, footer);
        }

        static uint64_t read(uint8_t *address, Footer &footer)
        {
            ZonePbtFormat;

            address += Format::read_uint64(address, footer.root_offset);
            address += Format::read_uint64(address, footer.root_size);
            address += Format::read_uint64(address, footer.level_0_end);
            address += Format::read_uint16(address, footer.tree_height);
            address += Format::read_uint64(address, footer.global_start);
            address += Format::read_uint64(address, footer.global_end);
            address += Format::read_uint16(address, footer.version_major);
            address += Format::read_uint16(address, footer.version_minor);
            address += Format::read_uint32(address, footer.magic);

            return Footer::size_of();
        }

        static constexpr uint64_t size_of()
        {
            return sizeof(Footer);
        }

        void validate()
        {
            ZonePbtStructures;

            if (this->magic != MAGIC)
            {
                throw std::runtime_error("Invalid magic number");
            }
            if (this->version_major != VERSION_MAJOR)
            {
                throw std::runtime_error("Invalid major version");
            }
            if (this->version_minor != VERSION_MINOR)
            {
                throw std::runtime_error("Invalid minor version");
            }
        }
    };
#pragma pack(pop)

    /**
     * Write-only structure for leaf nodes.
     */
    struct NodeLeafWrite
    {
        uint16_t num_children;
        std::vector<uint64_t> offsets;
        std::vector<uint64_t> key_sizes;
        std::vector<uint64_t> value_sizes;
        std::string data;

        NodeLeafWrite() : num_children(0) {}

        void add_key_value(std::string_view key, std::string_view value)
        {
            ZonePbtStructures;

            num_children++;
            offsets.push_back(data.size());
            key_sizes.push_back(key.size());
            value_sizes.push_back(value.size());
            data.append(key);
            data.append(value);
        }

        void clear()
        {
            ZonePbtStructures;

            num_children = 0;
            offsets.clear();
            key_sizes.clear();
            value_sizes.clear();
            data.clear();
        }

        uint64_t write(uint64_t offset, Storage &storage) const
        {
            ZonePbtStructures;

            uint64_t size = size_of();
            std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(size);
            uint8_t *address = buffer.get();

            uint64_t data_offset = sizeof(uint16_t) + 3 * sizeof(uint64_t) * this->num_children;

            address += Format::write_uint16(address, this->num_children);
            for (uint64_t i = 0; i < this->num_children; i++)
            {
                address += Format::write_uint64(address, this->offsets[i] + data_offset);
                address += Format::write_uint64(address, this->key_sizes[i]);
                address += Format::write_uint64(address, this->value_sizes[i]);
            }
            address += Format::write_string_data_only(address, this->data);

            storage.write(offset, buffer.get(), size);

            return size;
        }

        uint64_t size_of() const
        {
            ZonePbtStructures;

            uint64_t size = sizeof(uint16_t);
            size += 3 * sizeof(uint64_t) * this->num_children;
            size += this->data.size();

            return size;
        }
    };

    /**
     * Read-only structure for leaf nodes.
     */
    struct NodeLeafRead
    {
        std::unique_ptr<uint8_t[]> buffer;

        NodeLeafRead(uint8_t *buffer) : buffer(buffer) {}

        NodeLeafRead(uint64_t offset, const Storage &storage)
        {
            ZonePbtStructures;

            set_offset(offset, storage);
        }

        NodeLeafRead(uint64_t offset, uint64_t size, const Storage &storage)
        {
            ZonePbtStructures;

            set_offset_and_size(offset, size, storage);
        }

        void set_offset(uint64_t offset, const Storage &storage)
        {
            ZonePbtStructures;

            uint16_t num_children;
            storage.read(offset, (void *)&num_children, sizeof(uint16_t));

            std::array<uint64_t, 3> last_child_buffer;
            storage.read(offset + sizeof(uint16_t) + 3 * sizeof(uint64_t) * (num_children - 1), last_child_buffer.data(), sizeof(last_child_buffer));

            uint64_t size = last_child_buffer[0] + last_child_buffer[1] + last_child_buffer[2];
            this->buffer = std::make_unique<uint8_t[]>(size);
            storage.read(offset, this->buffer.get(), size);
        }

        void set_offset_and_size(uint64_t offset, uint64_t size, const Storage &storage)
        {
            ZonePbtStructures;

            buffer = std::make_unique<uint8_t[]>(size);
            storage.read(offset, buffer.get(), size);
        }

        uint64_t size_of() const
        {
            ZonePbtStructures;

            uint8_t *address = this->buffer.get();

            uint16_t num_children;
            address += Format::read_uint16(address, num_children);

            address += 3 * sizeof(uint64_t) * (num_children - 1);

            uint64_t offset;
            uint64_t key_size;
            uint64_t value_size;
            address += Format::read_uint64(address, offset);
            address += Format::read_uint64(address, key_size);
            address += Format::read_uint64(address, value_size);

            return offset + key_size + value_size;
        }

        uint16_t num_children() const
        {
            ZonePbtStructures;

            uint16_t num_children;
            Format::read_uint16(buffer.get(), num_children);
            return num_children;
        }

        std::string_view key(uint16_t i) const
        {
            ZonePbtStructures;

            uint8_t *address = this->buffer.get() + sizeof(uint16_t) + 3 * sizeof(uint64_t) * i;
            uint64_t offset;
            uint64_t key_size;
            address += Format::read_uint64(address, offset);
            address += Format::read_uint64(address, key_size);

            return std::string_view((char *)this->buffer.get() + offset, key_size);
        }

        std::string_view value(uint16_t i) const
        {
            ZonePbtStructures;

            uint8_t *address = this->buffer.get() + sizeof(uint16_t) + 3 * sizeof(uint64_t) * i;
            uint64_t offset;
            uint64_t key_size;
            uint64_t value_size;
            address += Format::read_uint64(address, offset);
            address += Format::read_uint64(address, key_size);
            address += Format::read_uint64(address, value_size);

            return std::string_view((char *)this->buffer.get() + offset + key_size, value_size);
        }
    };

    /**
     * Write-only structure for internal nodes.
     */
    struct NodeInternalWrite
    {
        uint16_t num_children;
        std::vector<uint64_t> offsets;
        std::vector<uint64_t> key_sizes;
        std::vector<uint64_t> reduced_value_sizes;
        std::vector<uint64_t> child_entry_counts;
        std::vector<uint64_t> child_offsets;
        std::vector<uint64_t> child_sizes;
        std::string data;

        NodeInternalWrite() : num_children(0) {}

        void add_first_child(std::string_view left_key, std::string_view right_key, std::string_view reduced_value, uint64_t child_entry_count, uint64_t child_offset, uint64_t child_size)
        {
            ZonePbtStructures;

            offsets.push_back(data.size());
            key_sizes.push_back(left_key.size());
            data.append(left_key);
            add_child(right_key, reduced_value, child_entry_count, child_offset, child_size);
        }

        void add_child(std::string_view right_key, std::string_view reduced_value, uint64_t child_entry_count, uint64_t child_offset, uint64_t child_size)
        {
            ZonePbtStructures;

            num_children++;
            offsets.push_back(data.size());
            key_sizes.push_back(right_key.size());
            reduced_value_sizes.push_back(reduced_value.size());
            child_entry_counts.push_back(child_entry_count);
            child_offsets.push_back(child_offset);
            child_sizes.push_back(child_size);
            data.append(right_key);
            data.append(reduced_value);
        }

        void clear()
        {
            ZonePbtStructures;

            num_children = 0;
            offsets.clear();
            key_sizes.clear();
            reduced_value_sizes.clear();
            child_entry_counts.clear();
            child_offsets.clear();
            child_sizes.clear();
            data.clear();
        }

        uint64_t write(uint64_t offset, Storage &storage) const
        {
            ZonePbtStructures;

            uint64_t size = size_of();
            std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(size);
            uint8_t *address = buffer.get();

            uint64_t data_offset = sizeof(uint16_t) + 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * this->num_children;

            address += Format::write_uint16(address, this->num_children);

            address += Format::write_uint64(address, this->offsets[0] + data_offset);
            address += Format::write_uint64(address, this->key_sizes[0]);
            for (uint64_t i = 0; i < this->num_children; i++)
            {
                address += Format::write_uint64(address, this->offsets[i + 1] + data_offset);
                address += Format::write_uint64(address, this->key_sizes[i + 1]);
                address += Format::write_uint64(address, this->reduced_value_sizes[i]);
                address += Format::write_uint64(address, this->child_entry_counts[i]);
                address += Format::write_uint64(address, this->child_offsets[i]);
                address += Format::write_uint64(address, this->child_sizes[i]);
            }

            address += Format::write_string_data_only(address, this->data);

            storage.write(offset, buffer.get(), size);

            return size;
        }

        uint64_t size_of() const
        {
            ZonePbtStructures;

            uint64_t size = sizeof(uint16_t);
            size += 2 * sizeof(uint64_t);
            size += 6 * sizeof(uint64_t) * this->num_children;
            size += this->data.size();

            return size;
        }
    };

    /**
     * Read-only structure for internal nodes.
     */
    struct NodeInternalRead
    {
        std::unique_ptr<uint8_t[]> buffer;

        NodeInternalRead(uint8_t *buffer) : buffer(buffer) {}

        NodeInternalRead(uint64_t offset, const Storage &storage)
        {
            ZonePbtStructures;

            uint16_t num_children;
            storage.read(offset, (void *)&num_children, sizeof(uint16_t));

            std::array<uint64_t, 3> last_child_buffer;
            storage.read(offset + sizeof(uint16_t) + 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * (num_children - 1), last_child_buffer.data(), sizeof(last_child_buffer));

            uint64_t size = last_child_buffer[0] + last_child_buffer[1] + last_child_buffer[2];
            this->buffer = std::make_unique<uint8_t[]>(size);
            storage.read(offset, this->buffer.get(), size);
        }

        NodeInternalRead(uint64_t offset, uint64_t size, const Storage &storage) : buffer(std::make_unique<uint8_t[]>(size))
        {
            ZonePbtStructures;

            storage.read(offset, buffer.get(), size);
        }

        void set_offset_and_size(uint64_t offset, uint64_t size, const Storage &storage)
        {
            ZonePbtStructures;

            buffer = std::make_unique<uint8_t[]>(size);
            storage.read(offset, buffer.get(), size);
        }

        uint64_t size_of() const
        {
            ZonePbtStructures;

            uint8_t *address = this->buffer.get();

            uint16_t num_children;
            address += Format::read_uint16(address, num_children);

            address += 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * (num_children - 1);

            uint64_t offset;
            uint64_t key_size;
            uint64_t reduced_value_size;
            address += Format::read_uint64(address, offset);
            address += Format::read_uint64(address, key_size);
            address += Format::read_uint64(address, reduced_value_size);

            return offset + key_size + reduced_value_size;
        }

        uint16_t num_children() const
        {
            ZonePbtStructures;

            uint16_t num_children;
            Format::read_uint16(buffer.get(), num_children);
            return num_children;
        }

        std::string_view left_key() const
        {
            ZonePbtStructures;

            uint8_t *address = this->buffer.get() + sizeof(uint16_t);
            uint64_t offset;
            uint64_t key_size;
            address += Format::read_uint64(address, offset);
            address += Format::read_uint64(address, key_size);

            return std::string_view((char *)this->buffer.get() + offset, key_size);
        }

        std::string_view right_key(uint16_t i) const
        {
            ZonePbtStructures;

            uint8_t *address = this->buffer.get() + sizeof(uint16_t) + 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * i;
            uint64_t offset;
            uint64_t key_size;
            address += Format::read_uint64(address, offset);
            address += Format::read_uint64(address, key_size);

            return std::string_view((char *)this->buffer.get() + offset, key_size);
        }

        std::string_view reduced_value(uint16_t i) const
        {
            ZonePbtStructures;

            uint8_t *address = this->buffer.get() + sizeof(uint16_t) + 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * i;
            uint64_t offset;
            uint64_t key_size;
            uint64_t reduced_value_size;
            address += Format::read_uint64(address, offset);
            address += Format::read_uint64(address, key_size);
            address += Format::read_uint64(address, reduced_value_size);

            return std::string_view((char *)this->buffer.get() + offset + key_size, reduced_value_size);
        }

        uint64_t child_entry_count(uint16_t i) const
        {
            ZonePbtStructures;

            uint8_t *address = this->buffer.get() + sizeof(uint16_t) + 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * i;
            uint64_t child_entry_count;
            address += Format::skip_uint64(3);
            address += Format::read_uint64(address, child_entry_count);

            return child_entry_count;
        }

        uint64_t child_offset(uint16_t i) const
        {
            ZonePbtStructures;

            uint8_t *address = this->buffer.get() + sizeof(uint16_t) + 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * i;
            uint64_t child_offset;
            address += Format::skip_uint64(4);
            address += Format::read_uint64(address, child_offset);

            return child_offset;
        }

        uint64_t child_size(uint16_t i) const
        {
            ZonePbtStructures;

            uint8_t *address = this->buffer.get() + sizeof(uint16_t) + 2 * sizeof(uint64_t) + 6 * sizeof(uint64_t) * i;
            uint64_t child_size;
            address += Format::skip_uint64(5);
            address += Format::read_uint64(address, child_size);

            return child_size;
        }
    };
}
