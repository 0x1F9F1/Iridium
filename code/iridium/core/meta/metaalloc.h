#pragma once

#include <type_traits>

namespace Iridium
{
    template <typename T>
    void* MetaAllocate([[maybe_unused]] usize len)
    {
        if constexpr (std::is_default_constructible_v<T>)
        {
            if (len)
                return new T[len];
            else
                return new T;
        }
        else
        {
            return nullptr;
        }
    }

    template <typename T>
    void MetaFree(void* ptr, usize len)
    {
        if (ptr)
        {
            if (len)
            {
                if constexpr (std::is_abstract_v<T>)
                {
                    IrAssert(false, "Cannot delete array of abstract class");
                }
                else
                {
                    delete[] static_cast<T*>(ptr);
                }
            }
            else
            {
                delete static_cast<T*>(ptr);
            }
        }
    }
} // namespace Iridium
