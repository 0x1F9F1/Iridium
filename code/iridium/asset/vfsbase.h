#pragma once

#include "stringheap.h"

namespace Iridium
{
    class VirtualFileSystemBase
    {
    public:
        StringView GetString(StringHeap::Handle handle);

        bool Exists(StringView path);
        void Reserve(usize capacity);
        void CompactNames();
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
            StringHash Hash {};

            NodeType Type {};

            // Next node in current hash bucket
            Node* HashNext {nullptr};

            // Name
            StringHeap::Handle Name {};

            // Parent folder
            FolderNode* Parent {nullptr};

            inline Node(StringHash hash, StringHeap::Handle name, NodeType type)
                : Hash(hash)
                , Type(type)
                , Name(name)
            {}
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

            inline FolderNode(StringHash hash, StringHeap::Handle name)
                : Node(hash, name, NodeType::Folder)
            {}

            void AddFile(FileNode* node);
            void AddFolder(FolderNode* node);

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

        StringHeap names_;

        usize GetNextCapacity(usize capacity);
        void ResizeBuckets(usize new_capacity);
        FolderNode* GetFolderNode(StringView name, bool create);

        void LinkNodeHash(Node* node);
        void UnlinkNodeHash(Node* node);

        void RemoveNode(Node* node);

        bool NodePathEqual(const Node* n, StringView path);
        Pair<Node*, StringHash> FindNode(StringView name);

        virtual void DeleteNode(Node* node) = 0;
    };

    Pair<StringView, StringView> ParentPath(StringView path);
    Pair<StringView, StringView> SplitPath(StringView name);

    inline StringView VirtualFileSystemBase::GetString(StringHeap::Handle handle)
    {
        return names_.GetString(handle);
    }
} // namespace Iridium
