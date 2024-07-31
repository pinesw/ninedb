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
        buffer.resize(detail::Footer::size());
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        file.open(file_path, std::ios::binary);
        file.seekg(-detail::Footer::size(), std::ios::end);
        file.read(buffer.data(), detail::Footer::size());
        file.close();

        uint8_t *buffer_ptr = (uint8_t *)buffer.data();
        detail::Footer footer;
        detail::Footer::read(buffer_ptr, footer);
        footer.validate();

        return footer;
    }
}
