#include "filedevice.h"

#include "findhandle.h"
#include "stream.h"

namespace Iridium
{
    Rc<Stream> FileDevice::Open(StringView, bool)
    {
        return nullptr;
    }

    Rc<Stream> FileDevice::Create(StringView, bool, bool)
    {
        return nullptr;
    }

    bool FileDevice::Exists(StringView path)
    {
        return Open(path, true) != nullptr;
    }

    Ptr<FindFileHandle> FileDevice::Find(StringView)
    {
        return nullptr;
    }

    bool FileDevice::Extension(StringView, FileDeviceExtension&)
    {
        return false;
    }
} // namespace Iridium
