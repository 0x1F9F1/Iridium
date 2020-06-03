#pragma once

#include "meta/metadeclare.h"

namespace Iridium
{
    class Base
    {
    public:
        inline virtual ~Base() = default;

        virtual const MetaClass* GetClass() const;

        bool IsA(const MetaClass* parent) const;

        template <typename T>
        bool IsA() const
        {
            return IsA(GetMetaClass<T>());
        }

        template <typename T>
        T* Cast()
        {
            return IsA<T>() ? static_cast<T*>(this) : nullptr;
        }

        META_DECLARE;
    };
} // namespace Iridium
