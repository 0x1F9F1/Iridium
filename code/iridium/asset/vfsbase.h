#pragma once

#include "stringheap.h"

namespace Iridium
{
    class VirtualFileSystemBase
    {
    public:
        void CompactNames();
        void Reserve(usize capacity);
        bool Exists(StringView path);

        StringView GetString(StringHeap::Handle handle)
        {
            return names_.GetString(handle);
        }

    private:
        VirtualFileSystemBase();
        ~VirtualFileSystemBase();

        VirtualFileSystemBase(const VirtualFileSystemBase&) = delete;
        VirtualFileSystemBase& operator=(const VirtualFileSystemBase&) = delete;

        template <typename T>
        friend class VirtualFileSystem;

        // TODO: Optimize Node layouts for size/speed

        struct FolderNode;

        struct Node
        {
            // Hash of full file path
            StringHash Hash {};

            // Is this node a folder
            bool IsFolder {false};

            // Next node in current hash bucket
            Node* HashNext {nullptr};

            // Name
            StringHeap::Handle Name {};

            // Parent folder
            FolderNode* Parent {nullptr};

            inline Node(StringHash hash, StringHeap::Handle name, bool is_folder)
                : Hash(hash)
                , IsFolder(is_folder)
                , Name(name)
            {}
        };

        struct FileNode : Node
        {
            // Next file in current folder
            FileNode* Next {nullptr};

            inline FileNode(StringHash hash, StringHeap::Handle name)
                : Node(hash, name, false)
            {}
        };

        struct FolderNode : VirtualFileSystemBase::Node
        {
            // Next folder in current folder
            FolderNode* Next {nullptr};

            // TODO: Add stats? (File Count, Folder Count)

            FileNode* Files {nullptr};
            FolderNode* Folders {nullptr};

            inline FolderNode(StringHash hash, StringHeap::Handle name)
                : Node(hash, name, true)
            {}

            void AddFile(FileNode* node);
            void AddFolder(FolderNode* node);
        };

        FolderNode* root_ {nullptr};

        usize node_count_ {0};
        usize bucket_count_ {0};
        Ptr<Node* []> buckets_ { nullptr };

        StringHeap names_;

        Pair<Node*, StringHash> FindNode(StringView name);

        FolderNode* CreateFolderNode(StringView name);

        usize GetNextCapacity(usize capacity);

        void AddNode(Node* node);
        void LinkNodeInternal(Node* node);

        bool NodePathEqual(const Node* n, StringView path);
    };

    Pair<StringView, StringView> ParentPath(StringView path);
} // namespace Iridium
