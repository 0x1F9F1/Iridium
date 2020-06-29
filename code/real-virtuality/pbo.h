#pragma once

#include "asset/device/virtual.h"

namespace Iridium
{
    class PboArchive final : public VirtualFileDevice
    {
    public:
        PboArchive(Rc<Stream> stream);

        bool RefreshFileList();

        const Rc<Stream>& GetInput() const
        {
            return input_;
        }

        u32 GetBaseOffset() const
        {
            return base_offset_;
        }

    private:
        Rc<Stream> input_;
        u32 base_offset_ {0};
    };
} // namespace Iridium
