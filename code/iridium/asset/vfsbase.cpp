#include "vfsbase.h"

#include "findhandle.h"

namespace Iridium
{
    void VirtualFileSystemBase::FolderNode::AddFile(FileNode* node)
    {
        IrAssert(node->Parent == nullptr, "Node already has a parent");

        node->Parent = this;
        node->Next = std::exchange(Files, node);
    }

    void VirtualFileSystemBase::FolderNode::AddFolder(FolderNode* node)
    {
        IrAssert(node->Parent == nullptr, "Node already has a parent");

        node->Parent = this;
        node->Next = std::exchange(Folders, node);
    }

    VirtualFileSystemBase::VirtualFileSystemBase() = default;
    VirtualFileSystemBase::~VirtualFileSystemBase() = default;

    inline usize VirtualFileSystemBase::GetNextCapacity(usize capacity)
    {
        usize result = bucket_count_ ? bucket_count_ : 32;

        while (result < capacity)
            result <<= 1;

        return result;
    }

    void VirtualFileSystemBase::AddNode(Node* node)
    {
        Reserve(node_count_ + 1);
        LinkNodeInternal(node);
        ++node_count_;
    }

    void VirtualFileSystemBase::LinkNodeInternal(Node* node)
    {
        IrDebugAssert(bucket_count_ != 0, "Cannot link node to map with no buckets");

        node->HashNext = std::exchange(buckets_[node->Hash.Value & (bucket_count_ - 1)], node);
    }

    bool VirtualFileSystemBase::NodePathEqual(const Node* n, StringView path)
    {
        if (n->IsFolder != (path.empty() || path.back() == '/'))
            return false;

        if (!n->IsFolder)
        {
            StringView file_name = names_.GetString(n->Name);

            usize j = path.size();

            if (file_name.size() > j)
                return false;

            j -= file_name.size();

            if (!PathCompareEqual(path.substr(j, file_name.size()), file_name))
                return false;

            path = path.substr(0, j);

            n = n->Parent;
        }

        return PathCompareEqual(path, names_.GetString(n->Name));
    }

    void VirtualFileSystemBase::CompactNames()
    {
        if (bucket_count_ == 0)
            return;

        StringHeap names;
        HashMap<StringView, StringHeap::Handle> handles;

        handles.reserve(node_count_);

        for (usize i = 0; i < bucket_count_; ++i)
        {
            for (Node* j = buckets_[i]; j; j = j->HashNext)
            {
                StringView const name = names_.GetString(j->Name);

                auto find = handles.find(name);

                if (find == handles.end())
                    find = handles.emplace_hint(find, name, names.AddString(name));

                j->Name = find->second;
            }
        }

        names_ = std::move(names);
    }

    Pair<VirtualFileSystemBase::Node*, StringHash> VirtualFileSystemBase::FindNode(StringView name)
    {
        StringHash hash = StringHash::HashIdent(name);

        if (bucket_count_ != 0)
        {
            Node** bucket = &buckets_[hash.Value & (bucket_count_ - 1)];

            for (Node *n = *bucket, *prev = nullptr; n; prev = n, n = n->HashNext)
                if (n->Hash == hash && NodePathEqual(n, name))
                {
                    if (prev)
                    {
                        prev->HashNext = n->HashNext;
                        n->HashNext = *bucket;
                        *bucket = n;
                    }

                    return {n, hash};
                }
        }

        return {nullptr, hash};
    }

    Pair<StringView, StringView> ParentPath(StringView path)
    {
        if (path.empty())
            return {path, path};

        usize length = path.size() - 1;
        usize split = path.substr(0, length).rfind('/');

        if (split == StringView::npos)
            split = 0;
        else
            split += 1;

        return {path.substr(0, split), path.substr(split, length - split)};
    }

    VirtualFileSystemBase::FolderNode* VirtualFileSystemBase::CreateFolderNode(StringView name)
    {
        auto [node, hash] = FindNode(name);

        if (node != nullptr)
        {
            IrAssert(node->IsFolder, "Found file node, expected folder");

            return static_cast<FolderNode*>(node);
        }

        auto [parent_path, folder_name] = ParentPath(name);

        FolderNode* new_node = new FolderNode {hash, names_.AddString(name)};

        if (name.empty())
        {
            IrDebugAssert(root_ == nullptr, "Root node already exists");

            root_ = new_node;
        }
        else
        {
            CreateFolderNode(parent_path)->AddFolder(new_node);
        }

        AddNode(new_node);

        return new_node;
    }

    void VirtualFileSystemBase::Reserve(usize capacity)
    {
        capacity += (capacity >> 2);

        if (capacity < bucket_count_)
            return;

        capacity = GetNextCapacity(capacity);

        Ptr<Node*[]> buckets = MakeUnique<Node*[]>(capacity);

        capacity = std::exchange(bucket_count_, capacity);

        buckets_.swap(buckets);

        usize total = 0;

        for (usize i = 0; i < capacity; ++i)
            for (Node* n = buckets[i]; n; LinkNodeInternal(std::exchange(n, n->HashNext)))
                ++total;

        IrDebugAssert(total == node_count_, "Node count mismatch");
    }

    bool VirtualFileSystemBase::Exists(StringView path)
    {
        auto [node, hash] = FindNode(path);

        return node != nullptr;
    }
} // namespace Iridium
