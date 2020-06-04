#include "metabuiltin.h"

#include "metadefine.h"

namespace Iridium
{
    VIRTUAL_META_DEFINE_CHILD("ScalarMetaType", ScalarMetaType, MetaType)
    {}

#define META_DEFINE_SCALAR_TYPE(TYPE, ID)              \
    struct ID##MetaType : ScalarMetaType               \
    {                                                  \
        usize SizeOf() const override                  \
        {                                              \
            return sizeof(TYPE);                       \
        }                                              \
                                                       \
        usize AlignOf() const override                 \
        {                                              \
            return alignof(TYPE);                      \
        }                                              \
                                                       \
        void* Allocate(usize len) const override       \
        {                                              \
            return MetaAllocate<TYPE>(len);            \
        }                                              \
                                                       \
        void Free(void* ptr, usize len) const override \
        {                                              \
            MetaFree<TYPE>(ptr, len);                  \
        }                                              \
                                                       \
        META_TYPE_REFLECT_DERIVED(ID, ID##MetaType);   \
    };                                                 \
                                                       \
    static const ID##MetaType ID##MetaType##Inst;      \
                                                       \
    const MetaType* MetaTypeFactory<TYPE>::Create()    \
    {                                                  \
        return &ID##MetaType##Inst;                    \
    }

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
