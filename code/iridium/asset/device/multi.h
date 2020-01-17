#pragma once

#include "asset/filedevice.h"

namespace Iridium
{
    class MultiFileDevice final : public FileDevice
    {
    public:
        ~MultiFileDevice() override;

        Rc<Stream> Open(StringView path, bool read_only) override;
        Rc<Stream> Create(StringView path, bool write_only, bool truncate) override;

        bool Exists(StringView path) override;

        void AddMount(StringView prefix, bool read_only, Rc<FileDevice> device);

    private:
        struct SubMount
        {
            bool ReadOnly {true};
            Rc<FileDevice> Device;
        };

        struct Mount
        {
            String Prefix;
            Vec<SubMount> Devices;
        };

        template <typename T>
        bool VisitMountedDevices(StringView path, bool read_only, T callback);

        Vec<Mount> mounts_;
    };
} // namespace Iridium
