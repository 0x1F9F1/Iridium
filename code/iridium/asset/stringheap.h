#pragma once

namespace Iridium
{
    class StringHeap
    {
    public:
        struct Handle
        {
        public:
            inline constexpr Handle() noexcept = default;

            inline usize GetSize() const noexcept
            {
                return Size;
            }

        private:
            Handle(usize offset, usize size) noexcept;

            u32 Offset {0};
            u32 Size {0};

            friend class StringHeap;
        };

        Handle AddString(StringView value);
        Pair<char*, Handle> NewString(usize length);
        void FreeString(Handle handle);

        void Clear();

        StringView GetString(Handle handle) const;
        const char* GetCString(Handle handle) const;

        usize GetSize() const;

    private:
        Vec<char> data_;
    };

    inline StringView StringHeap::GetString(StringHeap::Handle handle) const
    {
        return StringView(&data_.at(handle.Offset), handle.Size);
    }

    inline const char* StringHeap::GetCString(Handle handle) const
    {
        return &data_.at(handle.Offset);
    }

    inline usize StringHeap::GetSize() const
    {
        return data_.size();
    }
} // namespace Iridium
