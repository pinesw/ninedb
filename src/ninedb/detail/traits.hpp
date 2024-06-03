#pragma once

#include <type_traits>
#include <string>

namespace ninedb::detail
{
    /**
     * Returns true if T1 is dereferenceable to T2.
     */
    template <typename T1, typename T2>
    constexpr bool is_dereferenceable_to_v = std::is_same<std::remove_reference_t<decltype(*std::declval<T1>())>, T2>::value;
}
