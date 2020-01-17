#include "alloc.h"

namespace Iridium
{
    void* Z_alloc([[maybe_unused]] void* opaque, unsigned int items, unsigned int size)
    {
        return std::malloc(size * items);
    }

    void Z_free([[maybe_unused]] void* opaque, void* address)
    {
        return std::free(address);
    }
} // namespace Iridium
