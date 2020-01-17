#pragma once

#include "findhandle.h"
#include "vfsbase.h"

namespace Iridium
{
    template <typename T>
    class VirtualFileSystem : public VirtualFileSystemBase
    {
    public:
        ~VirtualFileSystem();

        bool AddFile(StringView name, T entry);

        T* FindFile(StringView name);

        static_assert(std::is_trivially_destructible_v<T>, "File entries must be trivially destructible");

        template <typename F>
        Ptr<FindFileHandle> Find(StringView path, F callback);

    protected:
        struct FileNodeT : FileNode
        {
            inline FileNodeT(StringHash hash, StringHeap::Handle name, T entry)
                : FileNode(hash, name)
                , Entry(std::move(entry))
            {}

            T Entry;
        };

        template <typename F>
        class VirtualFindFileHandle;

    private:
        void Clear();
    };

    Pair<StringView, StringView> SplitPath(StringView name);

    template <typename T>
    inline VirtualFileSystem<T>::~VirtualFileSystem()
    {
        Clear();
    }

    template <typename T>
    inline bool VirtualFileSystem<T>::AddFile(StringView name, T entry)
    {
        if (name.empty() || name.back() == '/')
            return false;

        auto [node, hash] = FindNode(name);

        if (node != nullptr)
            return false;

        auto [dir_name, file_name] = SplitPath(name);

        FileNodeT* new_node = new FileNodeT {hash, names_.AddString(file_name), std::move(entry)};

        CreateFolderNode(dir_name)->AddFile(new_node);

        AddNode(new_node);

        return true;
    }

    template <typename T>
    inline T* VirtualFileSystem<T>::FindFile(StringView name)
    {
        auto [node, hash] = FindNode(name);

        if (node == nullptr || node->IsFolder)
            return nullptr;

        return &static_cast<FileNodeT*>(node)->Entry;
    }

    template <typename T>
    template <typename F>
    Ptr<FindFileHandle> VirtualFileSystem<T>::Find(StringView path, F callback)
    {
        auto [node, hash] = FindNode(path);

        if (node == nullptr || !node->IsFolder)
            return nullptr;

        return MakeUnique<VirtualFindFileHandle<F>>(static_cast<FolderNode*>(node), &names_, std::move(callback));
    }

    template <typename T>
    template <typename F>
    class VirtualFileSystem<T>::VirtualFindFileHandle : public FindFileHandle
    {
    public:
        VirtualFindFileHandle(FolderNode* folder, StringHeap* names, F callback)
            : files_(folder->Files)
            , folders_(folder->Folders)
            , names_(names)
            , prefix_(folder->Name.GetSize())
            , callback_(std::move(callback))
        {}

        bool Next(FolderEntry& entry) override
        {
            entry.Reset();

            if (files_)
            {
                entry.Name = names_->GetString(files_->Name);

                callback_(static_cast<FileNodeT*>(files_)->Entry, entry);

                files_ = files_->Next;

                return true;
            }

            if (folders_)
            {
                StringView folder_path = names_->GetString(folders_->Name);

                entry.Name = folder_path.substr(prefix_, folder_path.size() - prefix_ - 1);
                entry.IsFolder = true;

                folders_ = folders_->Next;

                return true;
            }

            return false;
        }

    private:
        FileNode* files_ {nullptr};
        FolderNode* folders_ {nullptr};
        StringHeap* names_ {nullptr};
        usize prefix_ {0};
        F callback_;
    };

    template <typename T>
    void VirtualFileSystem<T>::Clear()
    {
        usize total = 0;

        for (usize i = 0; i < bucket_count_; ++i)
            for (Node *n = buckets_[i], *next = nullptr; n; n = next, ++total)
            {
                next = n->HashNext;

                if (n->IsFolder)
                    delete static_cast<FolderNode*>(n);
                else
                    delete static_cast<FileNodeT*>(n);
            }

        IrDebugAssert(total == node_count_, "Node count mismatch");

        buckets_.reset();
        bucket_count_ = 0;

        node_count_ = 0;
        names_.Clear();
    }
} // namespace Iridium
