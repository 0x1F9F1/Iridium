#pragma once

#include "findhandle.h"
#include "stream.h"
#include "vfsbase.h"

namespace Iridium
{
    template <typename T>
    class VirtualFileSystem final : public VirtualFileSystemBase
    {
    public:
        ~VirtualFileSystem() override;

        bool AddFile(StringView name, T entry);

        template <typename F>
        Rc<Stream> Open(StringView path, bool read_only, F callback);

        template <typename F>
        Rc<Stream> Create(StringView path, bool truncate, F callback);

        template <typename F>
        Ptr<FindFileHandle> Find(StringView path, F callback);

    private:
        struct FileNodeT : FileNode
        {
            inline FileNodeT(StringHash hash, StringHeap::Handle name, T entry)
                : FileNode(hash, name, NodeType::FileT)
                , Entry(std::move(entry))
            {}

            T Entry;
        };

        template <typename F>
        class VirtualFindFileHandle;

        void DeleteNode(Node* node) override;

        Rc<Stream> ConvertToVirtualNode(StringView path, FileNodeT* node, Rc<Stream> input);
    };

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
        GetFolderNode(dir_name, true)->AddFile(new_node);
        LinkNodeHash(new_node);

        return true;
    }

    template <typename T>
    template <typename F>
    inline Rc<Stream> VirtualFileSystem<T>::Open(StringView path, bool read_only, F callback)
    {
        auto [node, hash] = FindNode(path);

        if (node == nullptr)
            return nullptr;

        switch (node->Type)
        {
            case NodeType::Virtual:
            {
                Rc<Stream> result = static_cast<VirtualFileNode*>(node)->File;
                result->Seek(0, SeekWhence::Set);
                return result;
            }

            case NodeType::FileT:
            {
                Rc<Stream> result = callback(path, static_cast<FileNodeT*>(node)->Entry);

                if (!read_only && result != nullptr)
                {
                    result = ConvertToVirtualNode(path, static_cast<FileNodeT*>(node), std::move(result));
                }

                return result;
            }

            default: return nullptr;
        }
    }

    template <typename T>
    template <typename F>
    inline Rc<Stream> VirtualFileSystem<T>::Create(StringView path, bool truncate, F callback)
    {
        auto [node, hash] = FindNode(path);

        if (node == nullptr)
        {
            return ConvertToVirtualNode(path, nullptr, nullptr);
        }

        switch (node->Type)
        {
            case NodeType::Virtual:
            {
                Rc<Stream> result = static_cast<VirtualFileNode*>(node)->File;

                if (truncate)
                {
                    result->SetSize(0);
                }

                result->Seek(0, SeekWhence::Set);

                return result;
            }

            case NodeType::FileT:
            {
                return ConvertToVirtualNode(path, static_cast<FileNodeT*>(node),
                    !truncate ? callback(path, static_cast<FileNodeT*>(node)->Entry) : nullptr);
            }

            default: return nullptr;
        }
    }

    template <typename T>
    template <typename F>
    Ptr<FindFileHandle> VirtualFileSystem<T>::Find(StringView path, F callback)
    {
        auto [node, hash] = FindNode(path);

        if ((node == nullptr) || (node->Type != NodeType::Folder))
            return nullptr;

        return MakeUnique<VirtualFindFileHandle<F>>(static_cast<FolderNode*>(node), &names_, std::move(callback));
    }

    template <typename T>
    template <typename F>
    class VirtualFileSystem<T>::VirtualFindFileHandle final : public FindFileHandle
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

                switch (files_->Type)
                {
                    case NodeType::FileT:
                    {
                        callback_(static_cast<FileNodeT*>(files_)->Entry, entry);
                        break;
                    }

                    case NodeType::Virtual:
                    {
                        entry.Size = static_cast<VirtualFileNode*>(files_)->File->Size().get(0);
                        break;
                    }
                }

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
    void VirtualFileSystem<T>::DeleteNode(Node* node)
    {
        if (node->Type == NodeType::FileT)
            delete static_cast<FileNodeT*>(node);
        else
            VirtualFileSystemBase::DeleteNode(node);
    }

    template <typename T>
    inline Rc<Stream> VirtualFileSystem<T>::ConvertToVirtualNode(StringView path, FileNodeT* node, Rc<Stream> input)
    {
        Rc<Stream> result = Stream::Temp();

        if (input != nullptr)
        {
            input->CopyTo(*result);
            input = nullptr;
            result->Seek(0, SeekWhence::Set);
        }

        if (node != nullptr)
        {
            RemoveNode(node);
        }

        AddVirtualFile(path, result);

        return result;
    }
} // namespace Iridium
