#include "metastore.h"

#include "metadefine.h"

#include "metaalloc.h"

namespace Iridium
{
    VIRTUAL_META_DEFINE_CHILD("ScalarMetaType", ScalarMetaType, MetaType)
    {}

#define META_DEFINE_SCALAR_TYPE(TYPE, ID)                                               \
    usize ID##MetaType::SizeOf() const                                                  \
    {                                                                                   \
        return sizeof(TYPE);                                                            \
    }                                                                                   \
                                                                                        \
    usize ID##MetaType::AlignOf() const                                                 \
    {                                                                                   \
        return alignof(TYPE);                                                           \
    }                                                                                   \
                                                                                        \
    void* ID##MetaType::Allocate(usize len) const                                       \
    {                                                                                   \
        return MetaAllocate<TYPE>(len);                                                 \
    }                                                                                   \
                                                                                        \
    void ID##MetaType::Free(void* ptr, usize len) const                                 \
    {                                                                                   \
        MetaFree<TYPE>(ptr, len);                                                       \
    }                                                                                   \
                                                                                        \
    VIRTUAL_META_DEFINE_CHILD(IR_STRINGIFY(ID##MetaType), ID##MetaType, ScalarMetaType) \
    {}                                                                                  \
                                                                                        \
    const ID##MetaType ID##MetaType##Inst;

    META_DEFINE_SCALAR_TYPE(bool, Bool);
    META_DEFINE_SCALAR_TYPE(char, Char);

    META_DEFINE_SCALAR_TYPE(signed char, SChar);
    META_DEFINE_SCALAR_TYPE(unsigned char, UChar);

    META_DEFINE_SCALAR_TYPE(signed short, SShort);
    META_DEFINE_SCALAR_TYPE(unsigned short, UShort);

    META_DEFINE_SCALAR_TYPE(signed int, SInt);
    META_DEFINE_SCALAR_TYPE(unsigned int, UInt);

    META_DEFINE_SCALAR_TYPE(signed long, SLong);
    META_DEFINE_SCALAR_TYPE(unsigned long, ULong);

    META_DEFINE_SCALAR_TYPE(signed long long, SLongLong);
    META_DEFINE_SCALAR_TYPE(unsigned long long, ULongLong);

    META_DEFINE_SCALAR_TYPE(float, Float);
    META_DEFINE_SCALAR_TYPE(double, Double);
    META_DEFINE_SCALAR_TYPE(long double, LongDouble);

#undef META_DEFINE_SCALAR_TYPE

    usize PointerMetaType::SizeOf() const
    {
        return sizeof(void*);
    }

    usize PointerMetaType::AlignOf() const
    {
        return alignof(void*);
    }

    void* PointerMetaType::Allocate(usize) const
    {
        return nullptr;
    }

    void PointerMetaType::Free(void*, usize) const
    {}

    VIRTUAL_META_DEFINE_CHILD("PointerMetaType", PointerMetaType, ScalarMetaType)
    {
        META_FIELD("TargetType", TargetType);
    }
} // namespace Iridium
