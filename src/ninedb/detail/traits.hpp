#pragma once

#include <type_traits>
#include <string>

namespace ninedb::detail
{
    // /**
    //  * Returns true if T1 is the same as T2 after removing cv and reference qualifiers.
    //  */
    // template <typename T1, typename T2>
    // constexpr bool is_of_kind_v = std::is_same<std::remove_cv<std::remove_reference<T1>::type>::type, T2>::value;

    // /**
    //  * Returns true if T is a (const) string, or a reference to a (const) string, or an rvalue reference to a (const) string.
    //  */
    // template <typename T>
    // constexpr bool is_string_kind_v = is_of_kind_v<T, std::string> || std::is_convertible_v<T, std::string> || std::is_convertible_v<T, std::string_view>;

    template<typename T>
    constexpr bool is_string_kind_v = std::is_convertible_v<T, std::string> || std::is_convertible_v<T, std::string_view>;

    /**
     * Returns true if T1 is dereferenceable to T2.
     */
    template <typename T1, typename T2>
    constexpr bool is_dereferenceable_to_v = std::is_same<std::remove_reference_t<decltype(*std::declval<T1>())>, T2>::value;
}
