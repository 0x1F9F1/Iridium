#pragma once

#include <type_traits>

#define META_DEFINE_CLASS_STORE(NAME, TYPE, PARENT, DECLARE)        \
    template <>                                                     \
    ::Iridium::MetaClass Iridium::MetaClassStore_<TYPE>::Instance { \
        NAME, sizeof(TYPE), alignof(TYPE), PARENT, DECLARE, ::Iridium::MetaAllocate<TYPE>, ::Iridium::MetaFree<TYPE>};

#define META_DEFINE_META_DATA(TYPE)                       \
    struct TYPE::MetaData                                 \
    {                                                     \
        using MetaSelf = TYPE;                            \
        static void DeclareFields(::Iridium::MetaClass&); \
    };

#define META_DEFINE_GET_CLASS(TYPE)                    \
    const ::Iridium::MetaClass* TYPE::GetClass() const \
    {                                                  \
        return ::Iridium::GetMetaClass<TYPE>();        \
    }

#define META_CHECK_IS_INTERCONVIRTIBLE(BASE, DERIVED)                                                       \
    IrCheck((static_cast<DERIVED*>(reinterpret_cast<BASE*>(0x1000)) == reinterpret_cast<DERIVED*>(0x1000)), \
        #DERIVED " is not interconvirtible with " #BASE)

#define META_DECLARE_FIELDS(TYPE) void TYPE::MetaData::DeclareFields([[maybe_unused]] ::Iridium::MetaClass& cls)

#define META_FIELD(NAME, MEMBER) \
    cls.AddField(NAME, offsetof(MetaSelf, MEMBER), GetMetaType<decltype(MetaSelf::MEMBER)>())

#define META_DEFINE(NAME, TYPE)                                                  \
    META_DEFINE_META_DATA(TYPE)                                                  \
    META_DEFINE_CLASS_STORE(NAME, TYPE, nullptr, &TYPE::MetaData::DeclareFields) \
    META_DECLARE_FIELDS(TYPE)

#define VIRTUAL_META_DEFINE(NAME, TYPE)                                          \
    META_DEFINE_META_DATA(TYPE)                                                  \
    META_DEFINE_CLASS_STORE(NAME, TYPE, nullptr, &TYPE::MetaData::DeclareFields) \
    META_DEFINE_GET_CLASS(TYPE)                                                  \
    META_DECLARE_FIELDS(TYPE)

#define VIRTUAL_META_DEFINE_CHILD(NAME, TYPE, PARENT)                                      \
    static_assert(std::is_base_of_v<PARENT, TYPE>, "Invalid Parent");                      \
    META_DEFINE_META_DATA(TYPE)                                                            \
    META_DEFINE_CLASS_STORE(NAME, TYPE,                                                    \
        (META_CHECK_IS_INTERCONVIRTIBLE(PARENT, TYPE), ::Iridium::GetMetaClass<PARENT>()), \
        &TYPE::MetaData::DeclareFields)                                                    \
    META_DEFINE_GET_CLASS(TYPE)                                                            \
    META_DECLARE_FIELDS(TYPE)
