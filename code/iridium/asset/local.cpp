#include "local.h"

#include "findhandle.h"
#include "platform_io.h"
#include "stream.h"

namespace Iridium
{
    Rc<Stream> LocalFileDevice::Open(StringView path, bool read_only)
    {
        return PlatformOpenFile(path, read_only);
    }

    Rc<Stream> LocalFileDevice::Create(StringView path, bool write_only, bool truncate)
    {
        return PlatformCreateFile(path, write_only, truncate);
    }

    bool LocalFileDevice::Exists(StringView path)
    {
        return PlatformFileExists(path);
    }

    Ptr<FindFileHandle> LocalFileDevice::Find(StringView path)
    {
        return PlatformFindFiles(path);
    }

    bool LocalFileDevice::Delete(StringView path)
    {
        return PlatformDeleteFile(path);
    }

    StaticRc<LocalFileDevice> LocalFileDevice::s_LocalFiles;
} // namespace Iridium
