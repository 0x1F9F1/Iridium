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

        bool AddFile(StringView name, T entry, Rc<Stream> data = nullptr, bool override = false);

        template <typename F>
        Rc<Stream> Open(StringView path, bool read_only, F callback);

        template <typename F>
        Rc<Stream> Create(StringView path, bool truncate, F callback);

        template <typename F>
        Ptr<FindFileHandle> Find(StringView path, F callback);

    private:
        struct FileNodeT : FileNode
        {
            inline FileNodeT(StringHash hash, StringHeap::Handle name, T entry, Rc<Stream> data)
                : FileNode(hash, name, NodeType::FileT, std::move(data))
                , Entry(std::move(entry))
            {}

            T Entry;
        };

        template <typename F>
        class VirtualFindFileHandle;

        void DeleteNode(Node* node) override;
        FileNodeT* AddFileNode(StringView name, T entry, Rc<Stream> data, bool override);
        Rc<Stream> ConvertToVirtualNode(StringView path, FileNodeT* node, Rc<Stream> input);
    };

    template <typename T>
    inline VirtualFileSystem<T>::~VirtualFileSystem()
    {
        Clear();
    }

    template <typename T>
    inline bool VirtualFileSystem<T>::AddFile(StringView name, T entry, Rc<Stream> data, bool override)
    {
        return AddFileNode(name, std::move(entry), std::move(data), override) != nullptr;
    }

    template <typename T>
    template <typename F>
    inline Rc<Stream> VirtualFileSystem<T>::Open(StringView path, bool read_only, F callback)
    {
        auto [node, hash] = FindNode(path);

        if (node == nullptr || node->Type != NodeType::FileT)
            return nullptr;

        FileNodeT* fnode = static_cast<FileNodeT*>(node);

        Rc<Stream> result = fnode->Data;

        if (result != nullptr)
        {
            result->Seek(0, SeekWhence::Set);
        }
        else
        {
            result = callback(path, fnode->Entry);

            if (!read_only && result != nullptr)
            {
                result = ConvertToVirtualNode(path, fnode, std::move(result));
            }
        }

        return result;
    }

    template <typename T>
    template <typename F>
    inline Rc<Stream> VirtualFileSystem<T>::Create(StringView path, bool truncate, F callback)
    {
        auto [node, hash] = FindNode(path);

        if (node == nullptr)
            return ConvertToVirtualNode(path, nullptr, nullptr);

        if (node->Type != NodeType::FileT)
            return nullptr;

        FileNodeT* fnode = static_cast<FileNodeT*>(node);

        return ConvertToVirtualNode(path, fnode,
            !truncate ? callback(path, fnode->Entry) : nullptr);
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

                if (files_->Type == NodeType::FileT)
                {
                    FileNodeT* fnode = static_cast<FileNodeT*>(files_);

                    if (fnode->Data != nullptr)
                    {
                        entry.Size = fnode->Data->Size().get(0);
                    }
                    else
                    {
                        callback_(fnode->Entry, entry);
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
    inline typename VirtualFileSystem<T>::FileNodeT* VirtualFileSystem<T>::AddFileNode(
        StringView name, T entry, Rc<Stream> data, bool override)
    {
        if (name.empty() || name.back() == '/')
            return nullptr;

        const auto [node, hash] = FindNode(name);

        if (node != nullptr)
        {
            if (override && node->Type == NodeType::FileT)
            {
                FileNodeT* fnode = static_cast<FileNodeT*>(node);

                fnode->Entry = std::move(entry);
                fnode->Data = std::move(data);

                return fnode;
            }

            return nullptr;
        }

        const auto [dir_name, file_name] = SplitPath(name);

        FileNodeT* fnode = new FileNodeT {hash, names_.AddString(file_name), std::move(entry), std::move(data)};
        GetFolderNode(dir_name, true)->AddFile(fnode);
        LinkNodeHash(fnode);

        return fnode;
    }

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
        if (node == nullptr)
        {
            node = AddFileNode(path, T {}, nullptr, true);
        }

        if (node->Data == nullptr)
        {
            node->Data = Stream::Temp();
        }
        else
        {
            node->Data->SetSize(0);
            node->Data->Seek(0, SeekWhence::Set);
        }

        Rc<Stream> result = node->Data;

        if (input != nullptr)
        {
            input->CopyTo(*result);
            input = nullptr;
            result->Seek(0, SeekWhence::Set);
        }

        return result;
    }
} // namespace Iridium
