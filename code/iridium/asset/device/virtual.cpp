#include "virtual.h"

namespace Iridium
{
    Rc<Stream> VirtualFileDevice::Open(StringView path, bool read_only)
    {
        return vfs_.Open(path, read_only);
    }

    Rc<Stream> VirtualFileDevice::Create(StringView path, bool /*write_only*/, bool truncate)
    {
        return vfs_.Create(path, truncate);
    }

    bool VirtualFileDevice::Exists(StringView path)
    {
        return vfs_.FileExists(path);
    }

    Ptr<FindFileHandle> VirtualFileDevice::Find(StringView path)
    {
        return vfs_.Find(path);
    }
} // namespace Iridium
