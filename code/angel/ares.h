#pragma once

#include "asset/device/virtual.h"

namespace Iridium::Angel
{
    struct VirtualFileINode;

    class AresArchive final : public VirtualFileDevice
    {
    public:
        AresArchive(Rc<Stream> stream);

        static void Save(Rc<FileDevice> device, Vec<String> files, Rc<Stream> output);

        const Rc<Stream>& GetInput() const
        {
            return input_;
        }

    private:
        bool RefreshFileList();

        void AddFileNode(const Vec<VirtualFileINode>& nodes, const Vec<char>& names, u32 index, String& path);

        Rc<Stream> input_;
    };
} // namespace Iridium::Angel
