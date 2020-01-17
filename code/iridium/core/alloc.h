#pragma once

#include <cstddef>

namespace Iridium
{
    void* Z_alloc(void* opaque, unsigned int items, unsigned int size);
    void Z_free(void* opaque, void* address);
} // namespace Iridium
