#include "findhandle.h"

namespace Iridium
{
    Vec<String> FindFileHandle::GetFileNames()
    {
        Vec<String> result;

        if (this)
        {
            for (FolderEntry entry; Next(entry);)
            {
                if (!entry.IsFolder)
                {
                    result.emplace_back(std::move(entry.Name));
                }
            }
        }

        return result;
    }
} // namespace Iridium
