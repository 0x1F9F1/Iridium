#include "vfsbase.h"

#include "findhandle.h"
#include "stream.h"

namespace Iridium
{
    template <typename Node, typename F>
    static inline Node* MergeLists(Node* pSrc1, Node* pSrc2, F& comparator)
    {
        Node* pDst = nullptr; /* destination head ptr */
        Node** ppDst = &pDst; /* ptr to head or prev->next */

        while (true)
        {
            if (pSrc1 == nullptr)
            {
                *ppDst = pSrc2;
                break;
            }

            if (pSrc2 == nullptr)
            {
                *ppDst = pSrc1;
                break;
            }

            if (comparator(pSrc2, pSrc1))
            { /* if src2 < src1 */
                *ppDst = pSrc2;
                ppDst = &pSrc2->Next;
                pSrc2 = pSrc2->Next;
            }
            else
            { /* src1 <= src2 */
                *ppDst = pSrc1;
                ppDst = &pSrc1->Next;
                pSrc1 = pSrc1->Next;
            }
        }

        return pDst;
    }

    template <typename Node, typename F>
    static inline Node* SortList(Node* pList, F comparator)
    {
        if (pList == nullptr) /* check for empty list */
            return nullptr;

        constexpr usize NumLists = 32;

        Node* aList[NumLists] {}; /* array of lists */

        for (Node *pNode = pList, *pNext = nullptr; pNode; pNode = pNext)
        {
            pNext = pNode->Next;
            pNode->Next = nullptr;

            usize i = 0;

            for (; (i < NumLists) && (aList[i] != nullptr); ++i)
            {
                pNode = MergeLists(aList[i], pNode, comparator);
                aList[i] = nullptr;
            }

            if (i == NumLists)
                --i;

            aList[i] = pNode;
        }

        Node* pNode = nullptr; /* merge array into one list */

        for (usize i = 0; i < NumLists; i++)
            pNode = MergeLists(aList[i], pNode, comparator);

        return pNode;
    }

    bool VirtualFileSystemBase::Exists(StringView path)
    {
        return FindNode(path).first != nullptr;
    }

    void VirtualFileSystemBase::Reserve(usize capacity)
    {
        capacity += (capacity >> 2);

        if (capacity < bucket_count_)
            return;

        ResizeBuckets(capacity);
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
            for (Node* n = buckets_[i]; n; n = n->HashNext)
            {
                StringView const name = names_.GetString(n->Name);

                auto find = handles.find(name);

                if (find == handles.end())
                    find = handles.emplace_hint(find, name, names.AddString(name));

                n->Name = find->second;
            }
        }

        names_ = std::move(names);
    }

    void VirtualFileSystemBase::Clear()
    {
        usize total = 0;

        for (usize i = 0; i < bucket_count_; ++i)
        {
            for (Node *n = buckets_[i], *next = nullptr; n; n = next, ++total)
            {
                next = n->HashNext;
                DeleteNode(n);
            }
        }

        IrDebugAssert(total == node_count_, "Node count mismatch");

        buckets_.reset();
        bucket_count_ = 0;

        node_count_ = 0;
        names_.Clear();
    }

    void VirtualFileSystemBase::Sort()
    {
        FolderNode* dnode = root_;
        usize depth = 0;

        const auto comparator = [this](Node* lhs, Node* rhs) {
            return PathCompareLess(names_.GetString(lhs->Name), names_.GetString(rhs->Name));
        };

        while (dnode)
        {
            dnode->Files = SortList(dnode->Files, comparator);
            dnode->Folders = SortList(dnode->Folders, comparator);

            if (dnode->Folders)
            {
                dnode = dnode->Folders;
                ++depth;
                continue;
            }

            while (dnode && depth)
            {
                if (dnode->Next)
                {
                    dnode = dnode->Next;

                    break;
                }

                dnode = dnode->Parent;
                --depth;
            }

            if (depth == 0)
                break;
        };
    }

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

    bool VirtualFileSystemBase::FolderNode::Unlink(Node* node)
    {
        if (node->Type == NodeType::Folder)
        {
            for (FolderNode** here = &Folders; *here; here = &(*here)->Next)
            {
                if (*here == node)
                {
                    *here = std::exchange((*here)->Next, nullptr);
                    return true;
                }
            }
        }
        else
        {
            for (FileNode** here = &Files; *here; here = &(*here)->Next)
            {
                if (*here == node)
                {
                    *here = std::exchange((*here)->Next, nullptr);
                    return true;
                }
            }
        }

        return false;
    }

    VirtualFileSystemBase::VirtualFileSystemBase() = default;
    VirtualFileSystemBase::~VirtualFileSystemBase() = default;

    usize VirtualFileSystemBase::GetNextCapacity(usize capacity)
    {
        usize result = bucket_count_ ? bucket_count_ : 32;

        while (result < capacity)
            result <<= 1;

        return result;
    }

    void VirtualFileSystemBase::ResizeBuckets(usize new_capacity)
    {
        new_capacity = GetNextCapacity(new_capacity);

        Ptr<Node*[]> old_buckets = std::exchange(buckets_, MakeUnique<Node*[]>(new_capacity));
        usize old_capacity = std::exchange(bucket_count_, new_capacity);

        usize total = 0;

        for (usize i = 0; i < old_capacity; ++i)
        {
            for (Node *n = old_buckets[i], *next = nullptr; n; n = next)
            {
                next = n->HashNext;
                n->HashNext = std::exchange(buckets_[n->Hash.Value & (bucket_count_ - 1)], n);
                ++total;
            }
        }

        IrDebugAssert(total == node_count_, "Node count mismatch");
    }

    VirtualFileSystemBase::FolderNode* VirtualFileSystemBase::GetFolderNode(StringView name, bool create)
    {
        auto [node, hash] = FindNode(name);

        if (node != nullptr)
        {
            IrAssert(node->Type == NodeType::Folder, "Found file node, expected folder");

            return static_cast<FolderNode*>(node);
        }

        if (!create)
            return false;

        auto [parent_path, folder_name] = ParentPath(name);

        FolderNode* new_node = new FolderNode {hash, names_.AddString(name)};

        if (name.empty())
        {
            IrDebugAssert(root_ == nullptr, "Root node already exists");

            root_ = new_node;
        }
        else
        {
            GetFolderNode(parent_path, true)->AddFolder(new_node);
        }

        LinkNodeHash(new_node);

        return new_node;
    }

    void VirtualFileSystemBase::LinkNodeHash(Node* node)
    {
        Reserve(node_count_ + 1);
        node->HashNext = std::exchange(buckets_[node->Hash.Value & (bucket_count_ - 1)], node);
        ++node_count_;
    }

    void VirtualFileSystemBase::UnlinkNodeHash(Node* node)
    {
        if (bucket_count_ == 0)
            return;

        for (Node** n = &buckets_[node->Hash.Value & (bucket_count_ - 1)]; *n; n = &(*n)->HashNext)
        {
            if (*n == node)
            {
                *n = std::exchange(node->HashNext, nullptr);
                --node_count_;
                return;
            }
        }
    }

    void VirtualFileSystemBase::RemoveNode(Node* node)
    {
        if (node == nullptr)
            return;

        if (node->Type == NodeType::Folder)
        {
            IrAssert(!static_cast<FolderNode*>(node)->IsEmpty(), "Cannot remove non-empty folder node");
        }

        if (node->Parent != nullptr)
        {
            node->Parent->Unlink(node);
            node->Parent = nullptr;
        }
        else
        {
            IrAssert(node == root_, "Invalid Root Node");
            root_ = nullptr;
        }

        UnlinkNodeHash(node);
        DeleteNode(node);
    }

    bool VirtualFileSystemBase::NodePathEqual(const Node* n, StringView path)
    {
        if ((n->Type == NodeType::Folder) != (path.empty() || path.back() == '/'))
            return false;

        if (n->Type != NodeType::Folder)
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

    Pair<VirtualFileSystemBase::Node*, StringHash> VirtualFileSystemBase::FindNode(StringView name)
    {
        StringHash hash = StringHash::HashIdent(name);

        if (bucket_count_ != 0)
        {
            Node** bucket = &buckets_[hash.Value & (bucket_count_ - 1)];

            for (Node *n = *bucket, *prev = nullptr; n; prev = n, n = n->HashNext)
            {
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
        }

        return {nullptr, hash};
    }

    void VirtualFileSystemBase::DeleteNode(Node* node)
    {
        if (node->Type == NodeType::Folder)
        {
            delete static_cast<FolderNode*>(node);
        }
        else
        {
            IrDebugAssert(false, "Invalid Node Type");
        }
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
