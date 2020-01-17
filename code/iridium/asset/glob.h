#pragma once

namespace Iridium
{
    class FileDevice;
    class FindFileHandle;

    Ptr<FindFileHandle> Glob(Rc<FileDevice> device, String path, String pattern);
} // namespace Iridium
