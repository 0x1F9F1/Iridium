#pragma once

#include <tuple>

namespace Iridium
{
    template <typename T1, typename T2>
    using Pair = std::pair<T1, T2>;

    template <typename... Types>
    using Tuple = std::tuple<Types...>;

    template <typename... T>
    struct HashTuple
    {
        static inline void hash_combine(std::size_t& seed, std::size_t hash)
        {
            seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        template <std::size_t... Is>
        static inline std::size_t hash_unpack(Tuple<T...> const& tt, std::index_sequence<Is...>)
        {
            std::size_t seed = 0;

            using unpack = char[];

            (void) unpack {(hash_combine(seed, std::hash<T> {}(std::get<Is>(tt))), 0)..., 0};

            return seed;
        }

        inline std::size_t operator()(Tuple<T...> const& tt) const
        {
            return hash_unpack(tt, std::index_sequence_for<T...> {});
        }
    };
} // namespace Iridium
