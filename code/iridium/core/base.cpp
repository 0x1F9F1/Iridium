#include "base.h"

#include "meta/metadefine.h"

namespace Iridium
{
    bool Base::IsA(const MetaClass* parent) const
    {
        return GetClass()->IsA(parent);
    }

    VIRTUAL_META_DEFINE("Base", Base)
    {}
} // namespace Iridium
