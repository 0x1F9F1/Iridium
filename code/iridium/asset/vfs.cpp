#include "vfs.h"

namespace Iridium
{
    bool VFS::Node::SetName(StringView name, u32 hash)
    {
        usize const name_len = name.size();

        if (name_len > UINT16_MAX)
            return false;

        if (name.size() > NameLength)
        {
            if (IsNewName)
                delete[] NamePointer;

            NamePointer = new char[name_len];
            IsNewName = true;
        }

        std::memcpy(NamePointer, name.data(), name_len);

        NameLength = static_cast<u16>(name_len);
        NameHash = hash;

        return true;
    }

    VFS::~VFS()
    {
        IrAssert(!Locked(), "VFS destroyed while locked");

        Clear();
    }

    void VFS::Clear()
    {
        if (Locked())
            return;

        if (node_bucket_count_ == 0)
            return;

        for (usize i = 0; i < node_bucket_count_; ++i)
        {
            for (Node *n = node_buckets_[i], *next = nullptr; n; n = next)
            {
                next = n->HashNext;
                FreeNode(n);
            }
        }

        delete[] node_buckets_;

        node_count_ = 0;

        node_buckets_ = nullptr;
        node_bucket_count_ = 0;
    }

    IR_NOINLINE void VFS::ExpandBuckets(usize capacity)
    {
        capacity += capacity >> 1; // 66% load factor

        usize old_bucket_count = node_bucket_count_;
        usize new_bucket_count = old_bucket_count ? old_bucket_count : 64;

        while (new_bucket_count < capacity)
            new_bucket_count <<= 1;

        Node** old_buckets = node_buckets_;
        Node** new_buckets = new Node*[new_bucket_count]();

        usize const bucket_mask = new_bucket_count - 1;

        for (usize i = 0; i < old_bucket_count; ++i)
        {
            for (Node *n = old_buckets[i], *next = nullptr; n; n = next)
            {
                next = n->HashNext;
                n->HashNext = std::exchange(new_buckets[n->NameHash & bucket_mask], n);
            }
        }

        node_buckets_ = new_buckets;
        node_bucket_count_ = new_bucket_count;

        delete[] old_buckets;
    }

    static inline bool NodePathEqual(const VFS::Node* n, StringView path)
    {
        while (n)
        {
            StringView const name = n->GetName();

            usize const name_size = name.size();
            usize const path_size = path.size();

            if (name_size > path_size)
                return false;

            StringView base_name = path;
            base_name.remove_prefix(path_size - name_size);

            if (!StringCompareIEqual(base_name, name))
                return false;

            path.remove_suffix(name_size);

            n = n->ParentFolder;
        }

        return path.empty();
    }

    VFS::Node* VFS::FindNode(StringView path, u32 hash, bool is_folder)
    {
        Node* node = nullptr;

        if (node_bucket_count_ != 0)
        {
            node = node_buckets_[hash & (node_bucket_count_ - 1)];

            for (; node; node = node->HashNext)
            {
                if (node->NameHash == hash && node->IsFolder == is_folder && NodePathEqual(node, path))
                    break;
            }
        }

        return node;
    }

    VFS::Node* VFS::FindNode(FolderNode* folder, StringView name, u32 hash, bool is_folder)
    {
        Node* node = nullptr;

        if (node_bucket_count_ != 0)
        {
            node = node_buckets_[hash & (node_bucket_count_ - 1)];

            for (; node; node = node->HashNext)
            {
                if (node->NameHash == hash && node->IsFolder == is_folder && node->ParentFolder == folder &&
                    StringCompareIEqual(name, node->GetName()))
                    break;
            }
        }

        return node;
    }

    static Pair<StringView, StringView> SplitFolderPath(StringView path)
    {
        usize split = path.size();

        if (split == 0)
            return {path, path};

        do
        {
            --split;
        } while (split > 0 && path[split - 1] != '/');

        StringView base_name = path;
        base_name.remove_prefix(split);

        return {path.substr(0, split), base_name};
    }

    VFS::FolderNode* VFS::GetFolder(StringView path, u32 hash, bool create)
    {
        FolderNode* node = static_cast<FolderNode*>(FindNode(path, hash, true));

        if (node == nullptr && create && !Locked())
        {
            auto [parent_path, name] = SplitFolderPath(path);

            node = CreateNode<FolderNode>(name, hash);

            HashLinkNode(node);

            if (!path.empty())
            {
                GetFolder(parent_path, true)->AddNode(node);
            }
        }

        return node;
    }

    Rc<Stream> VFS::Open(StringView path, bool read_only)
    {
        FileNode* node = GetFile(path);

        if (node == nullptr)
            return nullptr;

        if (!read_only && node->IsReadOnly)
            return ConvertToTempNode(node, false);

        return node->Open(FileContext, read_only);
    }

    Rc<Stream> VFS::Create(StringView path, bool truncate)
    {
        FileNode* node = GetFile(path);

        if (node == nullptr)
        {
            Rc<Stream> data = Stream::Temp();

            if (data != nullptr)
                AddFile<StreamFileNode>(path, data);

            return data;
        }

        if (node->IsReadOnly)
            return ConvertToTempNode(node, truncate);

        return node->Create(FileContext, truncate);
    }

    Rc<Stream> VFS::ConvertToTempNode(FileNode* node, bool truncate)
    {
        if (Locked())
            return nullptr;

        Rc<Stream> data = Stream::Temp();

        if (data == nullptr)
            return nullptr;

        if (!truncate)
        {
            Rc<Stream> input = node->Open(FileContext, false);

            if (input == nullptr)
                return nullptr;

            input->CopyTo(*data);

            data->Seek(0, SeekWhence::Set);
        }

        StreamFileNode* new_node = MoveNode<StreamFileNode>(node, data);

        node->ParentFolder->ReplaceNode(node, new_node);
        HashReplaceNode(node, new_node);

        FreeNode(node);

        return data;
    }

    bool VFS::Rename(StringView old_name, StringView new_name, bool create)
    {
        if (Locked())
            return false;

        FileNode* node = GetFile(old_name);

        if (node == nullptr)
            return false;

        const auto [new_folder, new_file_name, new_folder_hash, new_hash] = SplitAndHashPath(new_name);

        FolderNode* folder = GetFolder(new_folder, new_folder_hash, create);

        if (folder == nullptr)
            return false;

        node->ParentFolder->RemoveNode(node);
        HashUnlinkNode(node);

        bool success = node->SetName(new_file_name, new_hash);

        folder->AddNode(node);
        HashLinkNode(node);

        return success;
    }

    const u32 VFS::HashPathLookup[256] {
        // Fractional part of pi, with normalized case
        // clang-format off
        0x243F6A88, 0x85A308D3, 0x13198A2E, 0x03707344, 0xA4093822, 0x299F31D0, 0x082EFA98, 0xEC4E6C89,
        0x452821E6, 0x38D01377, 0xBE5466CF, 0x34E90C6C, 0xC0AC29B7, 0xC97C50DD, 0x3F84D5B5, 0xB5470917,
        0x9216D5D9, 0x8979FB1B, 0xD1310BA6, 0x98DFB5AC, 0x2FFD72DB, 0xD01ADFB7, 0xB8E1AFED, 0x6A267E96,
        0xBA7C9045, 0xF12C7F99, 0x24A19947, 0xB3916CF7, 0x0801F2E2, 0x858EFC16, 0x636920D8, 0x71574E69,
        0xA458FEA3, 0xF4933D7E, 0x0D95748F, 0x728EB658, 0x718BCD58, 0x82154AEE, 0x7B54A41D, 0xC25A59B5,
        0x9C30D539, 0x2AF26013, 0xC5D1B023, 0x286085F0, 0xCA417918, 0xB8DB38EF, 0x8E79DCB0, 0x603A180E,
        0x6C9E0E8B, 0xB01E8A3E, 0xD71577C1, 0xBD314B27, 0x78AF2FDA, 0x55605C60, 0xE65525F3, 0xAA55AB94,
        0x57489862, 0x63E81440, 0x55CA396A, 0x2AAB10B6, 0xB4CC5C34, 0x1141E8CE, 0xA15486AF, 0x7C72E993,
        0xB3EE1411, 0xABD388F0, 0x6A51A0D2, 0xD8542F68, 0x960FA728, 0xAB5133A3, 0x6EEF0B6C, 0x137A3BE4,
        0xBA3BF050, 0x7EFB2A98, 0xA1F1651D, 0x39AF0176, 0x66CA593E, 0x82430E88, 0x8CEE8619, 0x456F9FB4,
        0x7D84A5C3, 0x3B8B5EBE, 0xE06F75D8, 0x85C12073, 0x401A449F, 0x56C16AA6, 0x4ED3AA62, 0x363F7706,
        0x1BFEDF72, 0x429B023D, 0x37D0D724, 0xA4842004, 0x69C8F04A, 0x9E1F9B5E, 0x21C66842, 0xF6E96C9A,
        0x670C9C61, 0xABD388F0, 0x6A51A0D2, 0xD8542F68, 0x960FA728, 0xAB5133A3, 0x6EEF0B6C, 0x137A3BE4,
        0xBA3BF050, 0x7EFB2A98, 0xA1F1651D, 0x39AF0176, 0x66CA593E, 0x82430E88, 0x8CEE8619, 0x456F9FB4,
        0x7D84A5C3, 0x3B8B5EBE, 0xE06F75D8, 0x85C12073, 0x401A449F, 0x56C16AA6, 0x4ED3AA62, 0x363F7706,
        0x1BFEDF72, 0x429B023D, 0x37D0D724, 0xD00A1248, 0xDB0FEAD3, 0x49F1C09B, 0x075372C9, 0x80991B7B,
        0x25D479D8, 0xF6E8DEF7, 0xE3FE501A, 0xB6794C3B, 0x976CE0BD, 0x04C006BA, 0xC1A94FB6, 0x409F60C4,
        0x5E5C9EC2, 0x196A2463, 0x68FB6FAF, 0x3E6C53B5, 0x1339B2EB, 0x3B52EC6F, 0x6DFC511F, 0x9B30952C,
        0xCC814544, 0xAF5EBD09, 0xBEE3D004, 0xDE334AFD, 0x660F2807, 0x192E4BB3, 0xC0CBA857, 0x45C8740F,
        0xD20B5F39, 0xB9D3FBDB, 0x5579C0BD, 0x1A60320A, 0xD6A100C6, 0x402C7279, 0x679F25FE, 0xFB1FA3CC,
        0x8EA5E9F8, 0xDB3222F8, 0x3C7516DF, 0xFD616B15, 0x2F501EC8, 0xAD0552AB, 0x323DB5FA, 0xFD238760,
        0x53317B48, 0x3E00DF82, 0x9E5C57BB, 0xCA6F8CA0, 0x1A87562E, 0xDF1769DB, 0xD542A8F6, 0x287EFFC3,
        0xAC6732C6, 0x8C4F5573, 0x695B27B0, 0xBBCA58C8, 0xE1FFA35D, 0xB8F011A0, 0x10FA3D98, 0xFD2183B8,
        0x4AFCB56C, 0x2DD1D35B, 0x9A53E479, 0xB6F84565, 0xD28E49BC, 0x4BFB9790, 0xE1DDF2DA, 0xA4CB7E33,
        0x62FB1341, 0xCEE4C6E8, 0xEF20CADA, 0x36774C01, 0xD07E9EFE, 0x2BF11FB4, 0x95DBDA4D, 0xAE909198,
        0xEAAD8E71, 0x6B93D5A0, 0xD08ED1D0, 0xAFC725E0, 0x8E3C5B2F, 0x8E7594B7, 0x8FF6E2FB, 0xF2122B64,
        0x8888B812, 0x900DF01C, 0x4FAD5EA0, 0x688FC31C, 0xD1CFF191, 0xB3A8C1AD, 0x2F2F2218, 0xBE0E1777,
        0xEA752DFE, 0x8B021FA1, 0xE5A0CC0F, 0xB56F74E8, 0x18ACF3D6, 0xCE89E299, 0xB4A84FE0, 0xFD13E0B7,
        0x7CC43B81, 0xD2ADA8D9, 0x165FA266, 0x80957705, 0x93CC7314, 0x211A1477, 0xE6AD2065, 0x77B5FA86,
        0xC75442F5, 0xFB9D35CF, 0xEBCDAF0C, 0x7B3E89A0, 0xD6411BD3, 0xAE1E7E49, 0x00250E2D, 0x2071B35E,
        0x226800BB, 0x57B8E0AF, 0x2464369B, 0xF009B91E, 0x5563911D, 0x59DFA6AA, 0x78C14389, 0xD95A537F,
        0x207D5BA2, 0x02E5B9C5, 0x83260376, 0x6295CFA9, 0x11C81968, 0x4E734A41, 0xB3472DCA, 0x7B14A94A,
        // clang-format on
    };

    static inline VFS::Node* MergeLists(VFS::Node* pSrc1, VFS::Node* pSrc2)
    {
        VFS::Node* pDst = nullptr; /* destination head ptr */
        VFS::Node** ppDst = &pDst; /* ptr to head or prev->next */

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

            if (StringCompareILess(pSrc2->GetName(), pSrc1->GetName()))
            { /* if src2 < src1 */
                *ppDst = pSrc2;
                ppDst = &(pSrc2->FolderNext);
                pSrc2 = *ppDst;
            }
            else
            { /* src1 <= src2 */
                *ppDst = pSrc1;
                ppDst = &(pSrc1->FolderNext);
                pSrc1 = *ppDst;
            }
        }

        return pDst;
    }

    static inline VFS::Node* SortList(VFS::Node* pList)
    {
        if (pList == nullptr) /* check for empty list */
            return nullptr;

        constexpr usize NumLists = 32;

        VFS::Node* aList[NumLists] {}; /* array of lists */

        for (VFS::Node *pNode = pList, *pNext = nullptr; pNode; pNode = pNext)
        {
            pNext = pNode->FolderNext;
            pNode->FolderNext = nullptr;

            usize i = 0;

            for (; (i < NumLists) && (aList[i] != nullptr); ++i)
            {
                pNode = MergeLists(aList[i], pNode);
                aList[i] = nullptr;
            }

            if (i == NumLists)
                --i;

            aList[i] = pNode;
        }

        VFS::Node* pNode = nullptr; /* merge array into one list */

        for (usize i = 0; i < NumLists; i++)
            pNode = MergeLists(aList[i], pNode);

        return pNode;
    }

    void VFS::FolderNode::Sort()
    {
        if (IsSorted)
            return;

        if (EntriesHead)
        {
            EntriesHead = SortList(EntriesHead);

            Node* node = EntriesHead;
            Node* prev = nullptr;

            while (node)
            {
                node->FolderPrev = prev;
                prev = node;
                node = node->FolderNext;
            }

            EntriesTail = prev;
        }

        IsSorted = true;
    }

    Rc<Stream> VFS::FileNode::Create(void* /*ctx*/, bool /*truncate*/)
    {
        return nullptr;
    }

    Rc<Stream> VFS::StreamFileNode::Open(void* /*ctx*/, bool /*read_only*/)
    {
        return Data;
    }

    Rc<Stream> VFS::StreamFileNode::Create(void* /*ctx*/, bool truncate)
    {
        Rc<Stream> result = Data;

        if (truncate)
            result->SetSize(0);

        result->Seek(0, SeekWhence::Set);

        return result;
    }

    bool VFS::StreamFileNode::Stat(void* /*ctx*/, FolderEntry& entry)
    {
        entry.Size = Data->Size().get(0);

        return true;
    }

    class VFSFindFileHandle : public FindFileHandle
    {
    public:
        VFSFindFileHandle(VFS* vfs, VFS::FolderNode* folder, bool sort)
            : vfs_(vfs)
        {
            vfs->Lock();

            if (sort)
                folder->Sort();

            current_ = folder->EntriesHead;
        }

        ~VFSFindFileHandle() override
        {
            vfs_->Unlock();
        }

        bool Next(FolderEntry& entry) override
        {
            entry.Reset();

            if (current_ == nullptr)
                return false;

            if (current_->IsFolder)
            {
                StringView name = current_->GetName();
                name.remove_suffix(1);

                entry.Name = name;
                entry.IsFolder = true;
            }
            else
            {
                entry.Name = current_->GetName();

                if (!static_cast<VFS::FileNode*>(current_)->Stat(vfs_->FileContext, entry))
                    entry.Size = 0;
            }

            current_ = current_->FolderNext;

            return true;
        }

    private:
        VFS* vfs_ {nullptr};
        VFS::Node* current_ {nullptr};
    };

    Ptr<FindFileHandle> VFS::Find(StringView path)
    {
        FolderNode* node = GetFolder(path, false);

        if (node == nullptr)
            return nullptr;

        return MakeUnique<VFSFindFileHandle>(this, node, true);
    }
} // namespace Iridium
