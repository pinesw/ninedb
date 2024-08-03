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
        buffer.resize(detail::Footer::size_of());
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        file.open(file_path, std::ios::binary);
        file.seekg(-detail::Footer::size_of(), std::ios::end);
        file.read(buffer.data(), detail::Footer::size_of());
        file.close();

        char *buffer_ptr = buffer.data();
        detail::Footer footer;
        detail::Footer::read(buffer_ptr, footer);
        footer.validate();

        return footer;
    }
}
