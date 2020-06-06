#pragma once

#include "filewatcher.h"

namespace Iridium
{
    void FileSystemListener::OnAdded(StringView)
    {}

    void FileSystemListener::OnRemoved(StringView)
    {}

    void FileSystemListener::OnModified(StringView)
    {}

    void FileSystemListener::OnRenamed(StringView, StringView)
    {}
} // namespace Iridium
