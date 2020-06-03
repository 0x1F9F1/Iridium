#pragma once

namespace Iridium
{
    class MetaClass;

    template <typename T>
    struct MetaClassStore_
    {
        static MetaClass Instance;
    };

    template <typename T>
    IR_FORCEINLINE const MetaClass* GetMetaClass()
    {
        return &MetaClassStore_<T>::Instance;
    }
} // namespace Iridium

#define META_DECLARE struct MetaData

#define VIRTUAL_META_DECLARE \
    META_DECLARE;            \
    const ::Iridium::MetaClass* GetClass() const override
