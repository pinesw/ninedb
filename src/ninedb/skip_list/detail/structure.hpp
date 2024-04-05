#pragma once

#include <list>
#include <memory>
#include <string>
#include <variant>

namespace ninedb::skip_list::detail
{
    struct Entry
    {
        std::shared_ptr<std::string> key;
        std::variant<std::shared_ptr<std::string>, std::list<Entry>::iterator> value;
    };
}
