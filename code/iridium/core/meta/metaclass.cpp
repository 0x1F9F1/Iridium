#include "metaclass.h"

#include "metadefine.h"

namespace Iridium
{
    VIRTUAL_META_DEFINE_CHILD("MetaClass", MetaClass, MetaType)
    {}

    IR_NOINLINE void MetaClass::AddField(const char* name, usize offset, const MetaType* type)
    {
        fields_.push_back({name, offset, type});
    }

    const MetaClass::MetaField* MetaClass::GetField(const char* name) const
    {
        for (const MetaField& field : fields_)
        {
            if (!std::strcmp(name, field.Name))
                return &field;
        }

        return nullptr;
    }
} // namespace Iridium
