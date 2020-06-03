#pragma once

#include "metatype.h"

#include "metaclass.h"

#include "metaalloc.h"

namespace Iridium
{
    struct ScalarMetaType : MetaType
    {
        META_TYPE_REFLECT_DERIVED(Scalar, ScalarMetaType);

        VIRTUAL_META_DECLARE;
    };

#define META_DECLARE_SCALAR_TYPE(TYPE)   \
    template <>                          \
    struct MetaTypeFactory<TYPE>         \
    {                                    \
        static const MetaType* Create(); \
    };

    META_DECLARE_SCALAR_TYPE(bool);
    META_DECLARE_SCALAR_TYPE(char);

    META_DECLARE_SCALAR_TYPE(signed char);
    META_DECLARE_SCALAR_TYPE(unsigned char);

    META_DECLARE_SCALAR_TYPE(signed short);
    META_DECLARE_SCALAR_TYPE(unsigned short);

    META_DECLARE_SCALAR_TYPE(signed int);
    META_DECLARE_SCALAR_TYPE(unsigned int);

    META_DECLARE_SCALAR_TYPE(signed long);
    META_DECLARE_SCALAR_TYPE(unsigned long);

    META_DECLARE_SCALAR_TYPE(signed long long);
    META_DECLARE_SCALAR_TYPE(unsigned long long);

    META_DECLARE_SCALAR_TYPE(float);
    META_DECLARE_SCALAR_TYPE(double);
    META_DECLARE_SCALAR_TYPE(long double);

#undef META_DECLARE_SCALAR_TYPE

    struct PointerMetaType : ScalarMetaType
    {
        PointerMetaType(const MetaType* target)
            : TargetType(target)
        {}

        const MetaType* TargetType {nullptr};

        usize SizeOf() const override;
        usize AlignOf() const override;
        void* Allocate(usize len) const override;
        void Free(void* ptr, usize len) const override;

        META_TYPE_REFLECT_DERIVED(Pointer, PointerMetaType);

        VIRTUAL_META_DECLARE;
    };

    template <typename T>
    struct MetaTypeFactory<T*>
    {
        static inline const MetaType* Create()
        {
            static const PointerMetaType result(GetMetaType<T>());
            return &result;
        }
    };

    template <typename T>
    struct MetaTypeFactory<T&>
    {
        static inline const MetaType* Create()
        {
            static const PointerMetaType result(GetMetaType<T>());
            return &result;
        }
    };

    template <typename T>
    struct MetaTypeFactory<T, std::enable_if_t<std::is_class_v<T>>>
    {
        static inline const MetaType* Create()
        {
            return GetMetaClass<T>();
        }
    };
} // namespace Iridium
