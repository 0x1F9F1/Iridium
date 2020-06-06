#pragma once

namespace Iridium
{
    class MetaClass;

    template <typename T>
    struct MetaClassStore
    {
        static IR_FORCEINLINE constexpr const MetaClass* Get()
        {
            return &Instance;
        }

    private:
        static MetaClass Instance;
    };
} // namespace Iridium

#define META_DECLARE struct MetaData

#define VIRTUAL_META_DECLARE \
    META_DECLARE;            \
    const ::Iridium::MetaClass* GetClass() const override
