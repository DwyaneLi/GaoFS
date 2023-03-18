//
// Created by DELL on 2023/3/13.
//

#ifndef GAOFS_MAKE_ARRAY_HPP
#define GAOFS_MAKE_ARRAY_HPP

#include <array>

namespace gaofs::utils {

template <typename... T>
constexpr auto
make_array(T&&... values) -> std::array<
    typename std::decay<typename std::common_type<T...>::type>::type,
    sizeof...(T)> {
    return std::array<
        typename std::decay<typename std::common_type<T...>::type>::type,
        sizeof...(T)>{std::forward<T>(values)...};
}

} // namespace gaofs::utils

#endif //GAOFS_MAKE_ARRAY_HPP
