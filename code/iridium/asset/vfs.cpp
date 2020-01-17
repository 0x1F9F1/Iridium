#include "vfs.h"

namespace Iridium
{
    Pair<StringView, StringView> SplitPath(StringView name)
    {
        usize split = name.rfind('/');

        if (split == StringView::npos)
            split = 0;
        else
            split += 1;

        return {name.substr(0, split), name.substr(split)};
    }
} // namespace Iridium
