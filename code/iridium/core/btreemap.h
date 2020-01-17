#pragma once

#include <map>

namespace Iridium
{
    template <typename Key, typename Value>
    using BTreeMap = std::map<Key, Value, std::less<>>;
} // namespace Iridium
