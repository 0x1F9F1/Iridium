#pragma once

#include "core/bitfield.h"

#include "asset/findhandle.h"
#include "asset/stream.h"

namespace Iridium
{
    class VFS final
    {
    public:
        struct Node;
        struct FolderNode;

        struct FileNode;

        struct StreamFileNode;

        template <typename T>
        struct FixedFileNode;

        VFS() = default;
        VFS(const VFS&) = delete;
        VFS(VFS&&) = delete;

        ~VFS();

        usize Lock();
        usize Unlock();

        bool Locked();

        void Clear();
        void Reserve(usize capacity);

        bool FileExists(StringView path);

        Rc<Stream> Open(StringView path, bool read_only);

        Rc<Stream> Create(StringView path, bool truncate);

        bool Rename(StringView old_name, StringView new_name, bool create);

        Ptr<FindFileHandle> Find(StringView path);

        template <typename T, typename... Args>
        T* AddFile(StringView name, Args&&... args);

        void* FileContext {nullptr};

    private:
        usize node_count_ {0};

        // Number of nodes which are folders
        // FileCount == NodeCount - FolderCount
        usize folder_count_ {0};

        usize lock_count_ {0};

        Node** node_buckets_ {nullptr};
        usize node_bucket_count_ {0};

        void ExpandBuckets(usize capacity);

        Node* FindNode(StringView name, u32 hash, bool is_folder);
        Node* FindNode(FolderNode* folder, StringView name, u32 hash, bool is_folder);

        FolderNode* GetFolder(StringView path, bool create);
        FolderNode* GetFolder(StringView path, u32 hash, bool create);

        FileNode* GetFile(StringView path);

        void HashLinkNode(Node* node);
        void HashUnlinkNode(Node* node);
        void HashReplaceNode(Node* old_node, Node* new_node);

        Rc<Stream> ConvertToTempNode(FileNode* node, bool truncate);

        template <typename T, typename... Args>
        static T* CreateNode(StringView name, u32 hash, Args&&... args);

        template <typename T, typename... Args>
        static T* MoveNode(Node* node, Args&&... args);

        template <typename T>
        static void FreeNode(T* node);

        static u32 HashPath(StringView str, u32 hash = 0) noexcept;
        static u32 HashReplacePrefix(u32 hash, u32 prefix_xor, u32 suffix_len) noexcept;

        static const u32 HashPathLookup[256];

        static Tuple<StringView, StringView, u32, u32> SplitAndHashPath(StringView name);
    };

    struct VFS::Node
    {
        inline Node(bool is_folder)
        {
            IsFolder = is_folder;
        }

        virtual inline ~Node()
        {
            if (IsNewName && NamePointer)
                delete[] NamePointer;
        }

        u32 NameHash {0};

        union
        {
            u16 Flags {0};

            // Is this node a FolderNode
            bit_field<u16, 0> IsFolder;

            // Is NamePointer dynamically allocated
            bit_field<u16, 1> IsNewName;

            // Folder Only
            // Are this folders entries sorted
            bit_field<u16, 2> IsSorted;

            // File Only
            // Can this file be opened for writing
            bit_field<u16, 3> IsReadOnly;
        };

        u16 NameLength {0};

        char* NamePointer {nullptr};

        Node* HashNext {nullptr};

        // Next node in current folder
        Node* FolderNext {nullptr};

        // Prev node in current folder
        Node* FolderPrev {nullptr};

        FolderNode* ParentFolder {nullptr};

        bool SetName(StringView name, u32 hash);

        inline StringView GetName() const
        {
            return StringView(NamePointer, NameLength);
        }
    };

    struct VFS::FolderNode final : VFS::Node
    {
        inline FolderNode()
            : Node(true)
        {}

        Node* EntriesHead {nullptr};
        Node* EntriesTail {nullptr};

        u32 FileCount {0};
        u32 FolderCount {0};

        void AddNode(Node* node)
        {
            node->ParentFolder = this;

            node->FolderNext = nullptr;
            node->FolderPrev = EntriesTail;

            if (node->FolderPrev)
                node->FolderPrev->FolderNext = node;
            else
                EntriesHead = node;

            EntriesTail = node;

            IsSorted = false;

            if (node->IsFolder)
                ++FolderCount;
            else
                ++FileCount;
        }

        void RemoveNode(Node* node)
        {
            Node* next = node->FolderNext;
            Node* prev = node->FolderPrev;

            if (next)
                next->FolderPrev = prev;
            else
                EntriesTail = prev;

            if (prev)
                prev->FolderNext = next;
            else
                EntriesHead = next;

            node->FolderNext = nullptr;
            node->FolderPrev = nullptr;

            node->ParentFolder = nullptr;

            if (node->IsFolder)
                --FolderCount;
            else
                --FileCount;
        }

        void ReplaceNode(Node* old_node, Node* new_node)
        {
            Node* next = old_node->FolderNext;
            Node* prev = old_node->FolderPrev;

            if (next)
                next->FolderPrev = new_node;
            else
                EntriesTail = new_node;

            if (prev)
                prev->FolderNext = new_node;
            else
                EntriesHead = new_node;

            old_node->FolderNext = nullptr;
            old_node->FolderPrev = nullptr;
            old_node->ParentFolder = nullptr;

            new_node->FolderNext = next;
            new_node->FolderPrev = prev;
            new_node->ParentFolder = this;
        }

        void Sort();
    };

    struct VFS::FileNode : VFS::Node
    {
        inline FileNode()
            : Node(false)
        {}

        // TODO: Is read_only necessary?
        virtual Rc<Stream> Open(void* ctx, bool read_only) = 0;

        // TODO: Is truncate necessary?
        virtual Rc<Stream> Create(void* ctx, bool truncate);

        virtual bool Stat(void* ctx, FolderEntry& entry) = 0;
    };

    struct VFS::StreamFileNode final : VFS::FileNode
    {
        inline StreamFileNode(Rc<Stream> data)
            : Data(std::move(data))
        {}

        Rc<Stream> Data;

        Rc<Stream> Open(void* ctx, bool read_only) override;
        Rc<Stream> Create(void* ctx, bool truncate) override;
        bool Stat(void* ctx, FolderEntry& entry) override;
    };

    template <typename T>
    struct VFS::FixedFileNode : VFS::FileNode
    {
        inline FixedFileNode(T entry)
            : Entry(std::move(entry))
        {
            IsReadOnly = true;
        }

        T Entry;
    };

    inline usize VFS::Lock()
    {
        return ++lock_count_;
    }

    inline usize VFS::Unlock()
    {
        return --lock_count_;
    }

    inline bool VFS::Locked()
    {
        return lock_count_ != 0;
    }

    inline void VFS::Reserve(usize capacity)
    {
        if (capacity + (capacity >> 2) <= node_bucket_count_)
            return;

        ExpandBuckets(capacity);
    }

    inline VFS::FolderNode* VFS::GetFolder(StringView path, bool create)
    {
        return GetFolder(path, HashPath(path), create);
    }

    inline VFS::FileNode* VFS::GetFile(StringView path)
    {
        return static_cast<FileNode*>(FindNode(path, HashPath(path), false));
    }

    inline bool VFS::FileExists(StringView path)
    {
        return GetFile(path) != nullptr;
    }

    template <typename T, typename... Args>
    inline T* VFS::AddFile(StringView name, Args&&... args)
    {
        if (Locked())
            return nullptr;

        if (name.empty() || name.back() == '/')
            return nullptr;

        const auto [dir_name, file_name, dir_hash, full_hash] = SplitAndHashPath(name);

        FolderNode* folder = GetFolder(dir_name, dir_hash, true);

        if (FindNode(folder, file_name, full_hash, false) != nullptr)
            return nullptr;

        T* node = CreateNode<T>(file_name, full_hash, std::forward<Args>(args)...);

        if (node != nullptr)
        {
            folder->AddNode(node);

            HashLinkNode(node);
        }

        return node;
    }

    template <typename T, typename... Args>
    inline T* VFS::CreateNode(StringView name, u32 hash, Args&&... args)
    {
        usize const name_len = name.size();

        if (name_len > UINT16_MAX)
            return nullptr;

        void* ptr = operator new(sizeof(T) + name_len);

        if (ptr == nullptr)
            return nullptr;

        T* result = new (ptr) T(std::forward<Args>(args)...);

        result->NameHash = hash;

        char* const name_ptr = static_cast<char*>(ptr) + sizeof(T);

        result->NameLength = static_cast<u16>(name_len);
        result->NamePointer = name_ptr;

        std::memcpy(name_ptr, name.data(), name_len);

        return result;
    }

    template <typename T, typename... Args>
    inline T* VFS::MoveNode(Node* node, Args&&... args)
    {
        u16 const name_len = node->NameLength;
        bool const new_name = node->IsNewName;

        void* ptr = operator new(sizeof(T) + (new_name ? 0 : name_len));

        if (ptr == nullptr)
            return nullptr;

        T* result = new (ptr) T(std::forward<Args>(args)...);

        result->NameHash = node->NameHash;
        result->NameLength = name_len;

        if (new_name)
        {
            result->NamePointer = node->NamePointer;
            result->IsNewName = true;

            node->NamePointer = nullptr;
            node->NameLength = 0;

            node->IsNewName = false;
        }
        else
        {
            char* const name_ptr = static_cast<char*>(ptr) + sizeof(T);
            result->NamePointer = name_ptr;
            std::memcpy(name_ptr, node->NamePointer, name_len);
        }

        return result;
    }

    template <typename T>
    inline void VFS::FreeNode(T* node)
    {
        node->~T();

        operator delete(node);
    }

    inline void VFS::HashLinkNode(Node* node)
    {
        Reserve(node_count_ + 1);

        Node** bucket = &node_buckets_[node->NameHash & (node_bucket_count_ - 1)];

        node->HashNext = *bucket;
        *bucket = node;

        ++node_count_;

        folder_count_ += node->IsFolder;
    }

    inline void VFS::HashUnlinkNode(Node* node)
    {
        if (node_bucket_count_ == 0)
            return;

        for (Node** n = &node_buckets_[node->NameHash & (node_bucket_count_ - 1)]; *n; n = &(*n)->HashNext)
        {
            if (*n == node)
            {
                *n = node->HashNext;
                node->HashNext = nullptr;

                --node_count_;
                folder_count_ -= node->IsFolder;

                return;
            }
        }
    }

    inline void VFS::HashReplaceNode(Node* old_node, Node* new_node)
    {
        if (node_bucket_count_ == 0)
            return;

        for (Node** n = &node_buckets_[old_node->NameHash & (node_bucket_count_ - 1)]; *n; n = &(*n)->HashNext)
        {
            if (*n == old_node)
            {
                *n = new_node;

                new_node->HashNext = old_node->HashNext;
                old_node->HashNext = nullptr;

                return;
            }
        }
    }

    inline u32 VFS::HashPath(StringView str, u32 hash) noexcept
    {
        for (u8 v : str)
            hash = _rotl(hash, 1) ^ HashPathLookup[v];

        return hash;
    }

    inline u32 VFS::HashReplacePrefix(u32 hash, u32 prefix_xor, u32 suffix_len) noexcept
    {
        // HashReplacePrefix(HashPath("foobar"), HashPath("foo") ^ HashPath("frob"), 3) == HashPath("frobbar")

        return hash ^ _rotl(prefix_xor, suffix_len & 0x1F);
    }

    inline Tuple<StringView, StringView, u32, u32> VFS::SplitAndHashPath(StringView name)
    {
        const char* name_str = name.data();

        usize split = name.size();

        while (split > 0 && name_str[split - 1] != '/')
            --split;

        StringView folder {name_str, split};
        u32 folder_hash = HashPath(folder);

        StringView file {name_str + split, name.size() - split};
        u32 full_hash = HashPath(file, folder_hash);

        return {folder, file, folder_hash, full_hash};
    }
} // namespace Iridium
