#pragma once

#include <unordered_set>

namespace Iridium
{
    template <typename Key>
    using HashSet = std::unordered_set<Key>;
} // namespace Iridium
