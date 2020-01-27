#pragma once

#include "asset/device/archive.h"

namespace Iridium
{
    class DaveArchive final : public FileArchive
    {
    public:
        DaveArchive(Rc<Stream> stream);

        void Save(Rc<Stream> output);

    private:
        bool RefreshFileList();
    };
} // namespace Iridium
