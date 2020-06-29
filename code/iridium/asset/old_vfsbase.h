#pragma once

#include "path.h"

namespace Iridium
{
    class VirtualFileSystemBase
    {
    public:
        bool Exists(StringView path)
        {
            return FindNode(path).first != nullptr;
        }

        bool Delete(StringView path);

        void Reserve(usize capacity)
        {
            capacity += (capacity >> 1);

            if (capacity < bucket_count_)
                return;

            ResizeBuckets(capacity);
        }

        void Clear();

        void Sort();

    private:
        VirtualFileSystemBase();
        VirtualFileSystemBase(const VirtualFileSystemBase&) = delete;
        VirtualFileSystemBase& operator=(const VirtualFileSystemBase&) = delete;

        virtual ~VirtualFileSystemBase() = 0;

        template <typename T>
        friend class VirtualFileSystem;

        struct FolderNode;

        enum class NodeType : u8
        {
            Folder,
            FileT,
        };

        struct Node
        {
            // Hash of full file path
            u32 Hash {};

            u32 NameSize {0};
            u16 NameOffset {0};

            NodeType Type {};

            // Next node in current hash bucket
            Node* HashNext {nullptr};

            // Parent folder
            FolderNode* Parent {nullptr};

            inline Node(u32 hash, NodeType type)
                : Hash(hash)
                , Type(type)
            {}

            inline StringView GetName() const
            {
                return {reinterpret_cast<const char*>(this) + NameOffset, NameSize};
            }
        };

        struct FileNode : Node
        {
            // Next file in current folder
            FileNode* Next {nullptr};

            using Node::Node;
        };

        struct FolderNode : Node
        {
            // Next folder in current folder
            FolderNode* Next {nullptr};

            // TODO: Add stats? (File Count, Folder Count)

            FileNode* Files {nullptr};
            FolderNode* Folders {nullptr};

            inline FolderNode(u32 hash)
                : Node(hash, NodeType::Folder)
            {}

            void AddFile(FileNode* node)
            {
                IrAssert(node->Parent == nullptr, "Node already has a parent");

                node->Parent = this;
                node->Next = std::exchange(Files, node);
            }

            void AddFolder(FolderNode* node)
            {
                IrAssert(node->Parent == nullptr, "Node already has a parent");

                node->Parent = this;
                node->Next = std::exchange(Folders, node);
            }

            inline bool IsEmpty() const
            {
                return (Files == nullptr) && (Folders == nullptr);
            }

            bool Unlink(Node* node);
        };

        FolderNode* root_ {nullptr};

        usize node_count_ {0};
        usize bucket_count_ {0};
        Ptr<Node* []> buckets_ { nullptr };

        usize GetNextCapacity(usize capacity);
        void ResizeBuckets(usize new_capacity);

        FolderNode* GetFolderNode(StringView name, bool create);
        FolderNode* GetFolderNode(StringView name, u32 hash, bool create);

        void LinkNodeHash(Node* node)
        {
            Reserve(node_count_ + 1);
            node->HashNext = std::exchange(buckets_[node->Hash & (bucket_count_ - 1)], node);
            ++node_count_;
        }

        void UnlinkNodeHash(Node* node);

        void RemoveNode(Node* node);

        bool NodePathEqual(const Node* n, StringView path);

        Pair<Node*, u32> FindNode(StringView name)
        {
            u32 hash = HashPath(name);

            return {FindNode(name, hash), hash};
        }

        IR_FORCEINLINE Node* FindNode(StringView name, u32 hash)
        {
            if (bucket_count_ != 0)
            {
                for (Node* n = buckets_[hash & (bucket_count_ - 1)]; n; n = n->HashNext)
                {
                    if (n->Hash == hash && NodePathEqual(n, name))
                        return n;
                }
            }

            return nullptr;
        }

        virtual void DeleteNode(Node* node) = 0;

        template <typename T, typename... Args>
        static T* CreateNode(StringView name, Args&&... args)
        {
            if (name.size() > std::numeric_limits<decltype(T::NameSize)>::max())
                return nullptr;

            static_assert(sizeof(T) <= std::numeric_limits<decltype(T::NameOffset)>::max(), "Node too large");

            void* ptr = operator new(sizeof(T) + name.size() + 1);

            if (ptr == nullptr)
                return nullptr;

            T* result = new (ptr) T(std::forward<Args>(args)...);

            char* name_ptr = static_cast<char*>(ptr) + sizeof(T);
            std::memcpy(name_ptr, name.data(), name.size());
            name_ptr[name.size()] = '\0';

            result->NameOffset = static_cast<decltype(T::NameOffset)>(sizeof(T));
            result->NameSize = static_cast<decltype(T::NameSize)>(name.size());

            return result;
        }

        static const u32 PathHashLookup[256];

        static IR_FORCEINLINE u32 HashPath(StringView str, u32 hash = 0) noexcept
        {
            for (unsigned char v : str)
                hash = _rotl(hash, 1) ^ PathHashLookup[v];

            return hash;
        }

        static Tuple<StringView, StringView, u32, u32> SplitPath(StringView name);
    };

    Pair<StringView, StringView> ParentPath(StringView path);

    inline VirtualFileSystemBase::VirtualFileSystemBase() = default;
    inline VirtualFileSystemBase::~VirtualFileSystemBase() = default;

    inline void VirtualFileSystemBase::DeleteNode(Node* node)
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

    inline Tuple<StringView, StringView, u32, u32> VirtualFileSystemBase::SplitPath(StringView name)
    {
        usize split = name.size();

        while (split > 0 && name[split - 1] != '/')
            --split;

        StringView folder = name.substr(0, split);
        StringView file = name.substr(split);

        u32 folder_hash = VirtualFileSystemBase::HashPath(folder);
        u32 full_hash = VirtualFileSystemBase::HashPath(file, folder_hash);

        return {folder, file, folder_hash, full_hash};
    }
} // namespace Iridium
