#include "base.h"

#include "meta/metadefine.h"

namespace Iridium
{
    bool Base::IsA(const MetaClass* target) const
    {
        return GetClass()->IsA(target);
    }

    VIRTUAL_META_DEFINE("Base", Base)
    {}
} // namespace Iridium
