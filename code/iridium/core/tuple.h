#pragma once

#include <tuple>

namespace Iridium
{
    template <typename T1, typename T2>
    using Pair = std::pair<T1, T2>;

    template <typename... Types>
    using Tuple = std::tuple<Types...>;
} // namespace Iridium
