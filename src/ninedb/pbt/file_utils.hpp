#pragma once

#include <cstdint>
#include <filesystem>

#include "../detail/profiling.hpp"

#include "./detail/format.hpp"
#include "./detail/structures.hpp"

namespace ninedb::pbt
{
    static detail::Footer read_footer(std::filesystem::path file_path)
    {
        ZonePbtReader;

        std::string buffer;
        buffer.resize(detail::Format::sizeof_footer());
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        file.open(file_path, std::ios::binary);
        file.seekg(-detail::Format::sizeof_footer(), std::ios::end);
        file.read(buffer.data(), detail::Format::sizeof_footer());
        file.close();

        uint8_t *buffer_ptr = (uint8_t *)buffer.data();
        detail::Footer footer;
        detail::Format::read_footer(buffer_ptr, footer);
        detail::Format::validate_footer(footer);

        return footer;
    }
}
