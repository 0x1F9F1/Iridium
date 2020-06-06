#pragma once

namespace Iridium
{
    class MetaType : public Base
    {
    public:
        enum class MetaId : u8
        { // clang-format off
            Scalar,
                Bool,
                Char,

                UChar,
                UShort,
                UInt,
                ULong,
                ULongLong,

                SChar,
                SShort,
                SInt,
                SLong,
                SLongLong,

                Float,
                Double,
                LongDouble,

                Pointer,

                Enum,
                    Bitset,

            Class,

            Invalid = UINT8_MAX,
        }; // clang-format on

        using MetaThis = MetaType;

        virtual MetaId GetMetaId() const noexcept = 0;

        virtual inline bool IsA(MetaId) const noexcept
        {
            return false;
        }

        virtual usize SizeOf() const = 0;
        virtual usize AlignOf() const = 0;

        virtual void* Allocate(usize len) const = 0;
        virtual void Free(void* ptr, usize len) const = 0;

        VIRTUAL_META_DECLARE;
    };

    template <typename T>
    inline const T* MetaTypeCast(const MetaType* info) noexcept
    {
        return (info && info->IsA(T::s_MetaId)) ? static_cast<const T*>(info) : nullptr;
    }
} // namespace Iridium

#define META_TYPE_REFLECT_DERIVED(ID, TYPE)                                   \
    using MetaBase = MetaThis;                                                \
    using MetaThis = TYPE;                                                    \
    static constexpr const ::Iridium::MetaType::MetaId s_MetaId {MetaId::ID}; \
    inline ::Iridium::MetaType::MetaId GetMetaId() const noexcept override    \
    {                                                                         \
        return s_MetaId;                                                      \
    }                                                                         \
    inline bool IsA(::Iridium::MetaType::MetaId id) const noexcept override   \
    {                                                                         \
        return s_MetaId == id || MetaBase::IsA(id);                           \
    }
