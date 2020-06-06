#pragma once

#include "metatype.h"

namespace Iridium
{
    class MetaClass final : public MetaType
    {
    public:
        inline MetaClass(const char* name, usize size, usize align, const MetaClass* parent,
            void (*declare)(MetaClass&), void* (*allocate)(usize), void (*free)(void*, usize))
            : name_(name)
            , size_(size)
            , align_(align)
            , parent_(parent)
            , allocate_(allocate)
            , free_(free)
        {
            declare(*this);
        }

        bool IsA(const MetaClass* target) const
        {
            for (const MetaClass* cls = this; cls; cls = cls->parent_)
            {
                if (cls == target)
                    return true;

                if (cls->size_ < target->size_)
                    break;
            }

            return false;
        }

        void* Allocate(usize len) const override
        {
            return allocate_(len);
        }

        void Free(void* ptr, usize len) const override
        {
            return free_(ptr, len);
        }

        usize SizeOf() const override
        {
            return size_;
        }

        usize AlignOf() const override
        {
            return align_;
        }

        const char* Name() const
        {
            return name_;
        }

        void AddField(const char* name, usize offset, const MetaType* type);

        struct MetaField
        {
            const char* Name {nullptr};
            usize Offset {0};
            const MetaType* Type {nullptr};
        };

        const MetaField* GetField(const char* name) const;

        META_TYPE_REFLECT_DERIVED(Class, MetaClass);

        VIRTUAL_META_DECLARE;

    private:
        const char* name_ {nullptr};

        usize size_ {0};
        usize align_ {0};

        const MetaClass* parent_ {nullptr};

        void* (*allocate_)(usize) {nullptr};
        void (*free_)(void*, usize) {nullptr};

        Vec<MetaField> fields_;
    };
} // namespace Iridium
