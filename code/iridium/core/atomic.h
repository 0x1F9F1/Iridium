#pragma once

#include <atomic>

namespace Iridium
{
    template <typename T>
    using Atomic = std::atomic<T>;
}
