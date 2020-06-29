#pragma once

#include "asset/compressorid.h"
#include "asset/device/virtual.h"

namespace Iridium
{
    class ZipArchive final : public VirtualFileDevice
    {
    public:
        ZipArchive(Rc<Stream> input);

        bool RefreshFileList();

        const Rc<Stream>& GetInput() const
        {
            return input_;
        }

    private:
        bool FindEndOfCentralDirectory();
        bool FindEndOfCentralDirectory64();

        bool FindCentralDirectory();
        bool FindCentralDirectory64();

        bool ParseCentralDirectory();

        i64 eocd_offset_ {-1};
        i64 eocd64_offset_ {-1};

        i64 cd_offset_ {-1};
        i64 cd_size_ {0};
        i64 cd_entries_ {0};

        Rc<Stream> input_;
    };
} // namespace Iridium
