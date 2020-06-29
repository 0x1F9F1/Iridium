#pragma once

#include "asset/device/virtual.h"

namespace Iridium::Angel
{
    class DaveArchive final : public VirtualFileDevice
    {
    public:
        DaveArchive(Rc<Stream> stream);

        bool RefreshFileList();

        static void Save(
            Rc<FileDevice> device, Vec<String> files, Rc<Stream> output, bool packed_names, u32 data_alignment = 0x800);

        const Rc<Stream>& GetInput() const
        {
            return input_;
        }

    private:
        Rc<Stream> input_;
    };
} // namespace Iridium::Angel
