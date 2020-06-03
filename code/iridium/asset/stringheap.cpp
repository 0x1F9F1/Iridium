#include "stringheap.h"

namespace Iridium
{
    StringHeap::Handle::Handle(usize offset, usize size) noexcept
        : Offset(static_cast<u32>(offset))
        , Size(static_cast<u32>(size))
    {}

    // TODO: Normalize slashes (\ -> /)

    StringHeap::Handle StringHeap::AddString(StringView value)
    {
        Handle result {data_.size(), value.size()};

        data_.insert(data_.end(), value.begin(), value.end());
        data_.push_back('\0');

        return result;
    }

    Pair<char*, StringHeap::Handle> StringHeap::NewString(usize length)
    {
        usize const heap_size = data_.size();

        data_.resize(heap_size + length + 1);

        return {&data_[heap_size], {heap_size, length}};
    }

    void StringHeap::FreeString(Handle handle)
    {
        if (data_.size() == handle.Offset + handle.Size + 1)
        {
            data_.resize(handle.Offset);
        }
    }

    void StringHeap::Clear()
    {
        data_.clear();
    }
} // namespace Iridium
