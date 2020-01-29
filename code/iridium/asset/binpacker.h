#pragma once

namespace Iridium
{
    class BinPacker
    {
    public:
        struct Item
        {
            // Size of the item to pack
            u64 Size {0};

            // Preffered offset of the item to pack
            u64 PrefferedOffset {UINT64_MAX};

            u64 ActualOffset {UINT64_MAX};

            u64 Alignment {0};

            usize Index {0};

            bool Fixed {false};
        };

        usize Add(u64 size, u64 offset = UINT64_MAX, u64 align = 1, bool fixed = false);
        bool Finalize();

        usize Size() const;
        const Item& operator[](usize index) const;

    private:
        Vec<Item> items_;
    };

    inline usize BinPacker::Size() const
    {
        return items_.size();
    }

    inline const BinPacker::Item& BinPacker::operator[](usize index) const
    {
        return items_[index];
    }
} // namespace Iridium