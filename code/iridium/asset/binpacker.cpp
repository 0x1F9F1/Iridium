#include "binpacker.h"

#include "core/bits.h"

#include <map>

namespace Iridium
{
    usize BinPacker::Add(u64 size, u64 offset, u64 align, bool fixed)
    {
        if (fixed)
        {
            IrAssert((offset & (align - 1)) == 0, "Invalid Item");
        }

        Item item;

        item.Size = size;
        item.PrefferedOffset = offset;
        item.Fixed = fixed;
        item.Alignment = align;

        usize result = items_.size();

        item.Index = result;

        items_.emplace_back(item);

        return result;
    }

    bool BinPacker::Finalize()
    {
        std::multimap<u64, usize> result;
        std::multimap<u64, usize> pending;

        for (usize i = 0; i < items_.size(); ++i)
        {
            const auto& item = items_[i];

            if (item.PrefferedOffset != UINT64_MAX)
            {
                if (item.Fixed && (item.PrefferedOffset & (item.Alignment - 1)))
                {
                    return false;
                }

                result.emplace(item.PrefferedOffset, i);
            }
            else
            {
                pending.emplace(item.Size, i);
            }
        }

        u64 here = 0;

        // Process items with preferred offsets, moving any non-fixed overlapping items to pending
        for (auto iter = result.begin(); iter != result.end();)
        {
            const auto& item = items_[iter->second];

            if (iter->first < here)
            {
                // Current item starts before current offset
                if (item.Fixed)
                {
                    // Item has a fixed offset, make space by moving previous non-fixed items to pending
                    while (true)
                    {
                        auto prev = std::prev(iter);

                        const auto& prev_item = items_[prev->second];

                        if (prev->first + prev_item.Size <= iter->first)
                            break; // We no longer overlap

                        if (prev_item.Fixed)
                            return false; // Item cannot be moved, error.

                        pending.emplace(prev_item.Size, prev->second);
                        result.erase(prev);
                    }
                }
                else
                {
                    // Item offset can change, so move it to pending
                    pending.emplace(item.Size, iter->second);
                    iter = result.erase(iter);

                    continue;
                }
            }

            here = iter->first + item.Size;

            ++iter;
        }

        here = 0;

        // Iterate over the fixed items, filling in any gaps where possible.
        for (auto iter = result.begin(); iter != result.end(); ++iter)
        {
            const auto& item = items_[iter->second];

            // Attempt to fill in any avaiable space
            while (here < iter->first)
            {
                if (pending.empty())
                    break;

                u64 avail = iter->first - here;

                auto find = pending.lower_bound(avail);

                if (find == pending.end())
                    --find;

                while (true)
                {
                    if (find->first <= avail)
                    {
                        // Item fits in available gap
                        result.emplace_hint(iter, here, find->second);
                        here += find->first;

                        pending.erase(find);

                        break;
                    }
                    else if (find != pending.begin())
                    {
                        // Item is too large, go to the next smaller item
                        --find;
                    }
                    else
                    {
                        // No item fits in the available gap, so just move on
                        here = iter->first;

                        break;
                    }
                }
            }

            // Move to end of current item
            here = iter->first + item.Size;
        }

        // Add the remaining items to the end
        for (auto iter = pending.begin(); iter != pending.end(); iter = pending.erase(iter))
        {
            const auto& item = items_[iter->second];

            here = bits::align(here, item.Alignment);

            result.emplace(here, iter->second);

            here += iter->first;
        }

        for (const auto& item : result)
        {
            items_[item.second].ActualOffset = item.first;
        }

        return true;
    }
}; // namespace Iridium
