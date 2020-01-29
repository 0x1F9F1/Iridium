#include "relative.h"

#include "asset/findhandle.h"
#include "asset/stream.h"

namespace Iridium
{
    RelativeFileDevice::RelativeFileDevice(String prefix, Rc<FileDevice> device)
        : prefix_(std::move(prefix))
        , device_(std::move(device))
    {}

    Rc<Stream> RelativeFileDevice::Open(StringView path, bool read_only)
    {
        return device_->Open(ToAbsolute(path), read_only);
    }

    Rc<Stream> RelativeFileDevice::Create(StringView path, bool write_only, bool truncate)
    {
        return device_->Create(ToAbsolute(path), write_only, truncate);
    }

    bool RelativeFileDevice::Exists(StringView path)
    {
        return device_->Exists(ToAbsolute(path));
    }

    Ptr<FindFileHandle> RelativeFileDevice::Find(StringView path)
    {
        return device_->Find(ToAbsolute(path));
    }

    bool RelativeFileDevice::Extension(StringView path, FileDeviceExtension& data)
    {
        return device_->Extension(ToAbsolute(path), data);
    }

    String RelativeFileDevice::ToAbsolute(StringView path)
    {
        return Concat(prefix_, path);
    }
} // namespace Iridium
