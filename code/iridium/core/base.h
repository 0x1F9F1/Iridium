#pragma once

#include "meta/metadeclare.h"

namespace Iridium
{
    class Base
    {
    public:
        inline virtual ~Base() = default;

        virtual const MetaClass* GetClass() const;

        bool IsA(const MetaClass* target) const;

        template <typename T>
        bool IsA() const
        {
            const MetaClass* target = GetMetaClass<T>();

            if constexpr (std::is_final_v<T>)
            {
                return GetClass() == target;
            }
            else
            {
                return IsA(target);
            }
        }

        META_DECLARE;
    };
} // namespace Iridium
