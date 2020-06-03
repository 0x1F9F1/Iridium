#include "refcount.h"

#include "meta/metadefine.h"

namespace Iridium
{
    VIRTUAL_META_DEFINE_CHILD("RefCounted", RefCounted, Base)
    {}

    VIRTUAL_META_DEFINE_CHILD("AtomicRefCounted", AtomicRefCounted, Base)
    {}
} // namespace Iridium
