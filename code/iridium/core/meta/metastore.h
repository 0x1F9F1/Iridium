#pragma once

#include "metatype.h"

#include "metaclass.h"

namespace Iridium
{
    template <typename T, typename = void>
    struct MetaTypeFactory;

    template <typename T>
    IR_FORCEINLINE constexpr const MetaType* GetMetaType()
    {
        return MetaTypeFactory<std::remove_cv_t<T>>::Create();
    }

    struct ScalarMetaType : MetaType
    {
        META_TYPE_REFLECT_DERIVED(Scalar, ScalarMetaType);

        VIRTUAL_META_DECLARE;
    };

#define META_DECLARE_SCALAR_TYPE(TYPE, ID)                       \
    struct ID##MetaType : ScalarMetaType                         \
    {                                                            \
        usize SizeOf() const override;                           \
        usize AlignOf() const override;                          \
        void* Allocate(usize len) const override;                \
        void Free(void* ptr, usize len) const override;          \
                                                                 \
        META_TYPE_REFLECT_DERIVED(ID, ID##MetaType);             \
    };                                                           \
                                                                 \
    extern const ID##MetaType ID##MetaType##Inst;                \
                                                                 \
    template <>                                                  \
    struct MetaTypeFactory<TYPE>                                 \
    {                                                            \
        static IR_FORCEINLINE constexpr const MetaType* Create() \
        {                                                        \
            return &ID##MetaType##Inst;                          \
        }                                                        \
    };

    META_DECLARE_SCALAR_TYPE(bool, Bool);
    META_DECLARE_SCALAR_TYPE(char, Char);

    META_DECLARE_SCALAR_TYPE(signed char, SChar);
    META_DECLARE_SCALAR_TYPE(unsigned char, UChar);

    META_DECLARE_SCALAR_TYPE(signed short, SShort);
    META_DECLARE_SCALAR_TYPE(unsigned short, UShort);

    META_DECLARE_SCALAR_TYPE(signed int, SInt);
    META_DECLARE_SCALAR_TYPE(unsigned int, UInt);

    META_DECLARE_SCALAR_TYPE(signed long, SLong);
    META_DECLARE_SCALAR_TYPE(unsigned long, ULong);

    META_DECLARE_SCALAR_TYPE(signed long long, SLongLong);
    META_DECLARE_SCALAR_TYPE(unsigned long long, ULongLong);

    META_DECLARE_SCALAR_TYPE(float, Float);
    META_DECLARE_SCALAR_TYPE(double, Double);
    META_DECLARE_SCALAR_TYPE(long double, LongDouble);

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
        static inline const PointerMetaType Instance {GetMetaType<T>()};

        static IR_FORCEINLINE constexpr const MetaType* Create()
        {
            return &Instance;
        }
    };

    template <typename T>
    struct MetaTypeFactory<T&> : MetaTypeFactory<T*>
    {};

    template <typename T>
    struct MetaTypeFactory<T, std::enable_if_t<std::is_class_v<T>>>
    {
        static IR_FORCEINLINE constexpr const MetaType* Create()
        {
            return MetaClassStore<T>::Get();
        }
    };
} // namespace Iridium
