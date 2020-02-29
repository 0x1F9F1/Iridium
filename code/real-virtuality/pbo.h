#pragma once

#include "asset/filedevice.h"
#include "asset/vfs.h"

namespace Iridium
{
    class PboArchive final : public FileDevice
    {
    public:
        PboArchive(Rc<Stream> input);

        Rc<Stream> Open(StringView path, bool read_only) override;

        bool Exists(StringView path) override;

        Ptr<FindFileHandle> Find(StringView path) override;

    private:
        struct PboEntry
        {
            u32 PackingMethod {0};
            u32 Size {0};
            u32 RawSize {0};
            u32 Offset {0};

            u32 TimeStamp {0};

            u32 GetSize() const
            {
                return (PackingMethod != 0) ? Size : RawSize;
            }
        };

        VirtualFileSystem<PboEntry> vfs_;
        Rc<Stream> input_;

        bool RefreshFileList();

        Rc<Stream> OpenEntry(StringView path, PboEntry& entry);
    };

    extern template class VirtualFileSystem<PboArchive::PboEntry>;
} // namespace Iridium
