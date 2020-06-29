#pragma once

#include "asset/filedevice.h"
#include "asset/vfs.h"

namespace Iridium
{
    class VirtualFileDevice : public FileDevice
    {
    public:
        Rc<Stream> Open(StringView path, bool read_only) override;
        Rc<Stream> Create(StringView path, bool write_only, bool truncate) override;

        bool Exists(StringView path) override;

        Ptr<FindFileHandle> Find(StringView path) override;

    protected:
        VFS vfs_;
    };
} // namespace Iridium
