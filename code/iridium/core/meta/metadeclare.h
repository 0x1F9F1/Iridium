#pragma once

namespace Iridium
{
    class MetaClass;

    template <typename T>
    extern const MetaClass* GetMetaClass();
} // namespace Iridium

#define META_DECLARE struct MetaData

#define VIRTUAL_META_DECLARE \
    META_DECLARE;            \
    const ::Iridium::MetaClass* GetClass() const override
