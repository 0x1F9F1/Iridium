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

        struct FileEntry
        {
            T Entry;
            Rc<Stream> Data;

            FileEntry(T entry = T {}, Rc<Stream> data = nullptr);
        };

        struct Visitor
        {
            virtual ~Visitor() = default;

            virtual bool VisitFile(StringView path, FileEntry& entry) = 0;
            virtual bool VisitFolder(StringView path) = 0;
        };

        bool AddFile(StringView name, FileEntry entry, bool override = false);

        template <typename F>
        Rc<Stream> Open(StringView path, bool read_only, F callback);

        template <typename F>
        Rc<Stream> Create(StringView path, bool truncate, F callback);

        template <typename F>
        Ptr<FindFileHandle> Find(StringView path, F callback);

        FileEntry* GetEntry(StringView path);

        bool Visit(StringView path, Visitor& visitor);

    private:
        struct FileNodeT : FileNode
        {
            inline FileNodeT(u32 hash, FileEntry entry)
                : FileNode(hash, NodeType::FileT)
                , Entry(std::move(entry))
            {}

            FileEntry Entry;
        };

        template <typename F>
        class VirtualFindFileHandle;

        void DeleteNode(Node* node) override;
        FileNodeT* AddFileNode(StringView name, FileEntry entry, bool override);
        Rc<Stream> ConvertToVirtualNode(StringView path, FileNodeT* node, Rc<Stream> input);
    };

    template <typename T>
    inline VirtualFileSystem<T>::FileEntry::FileEntry(T entry, Rc<Stream> data)
        : Entry(std::move(entry))
        , Data(std::move(data))
    {}

    template <typename T>
    inline VirtualFileSystem<T>::~VirtualFileSystem()
    {
        Clear();
    }

    template <typename T>
    inline bool VirtualFileSystem<T>::AddFile(StringView name, FileEntry entry, bool override)
    {
        return AddFileNode(name, std::move(entry), override) != nullptr;
    }

    template <typename T>
    inline typename VirtualFileSystem<T>::FileEntry* VirtualFileSystem<T>::GetEntry(StringView path)
    {
        const auto [node, _] = FindNode(path);

        if (node == nullptr || node->Type != NodeType::FileT)
            return nullptr;

        return &static_cast<FileNodeT*>(node)->Entry;
    }

    template <typename T>
    inline bool VirtualFileSystem<T>::Visit(StringView path, Visitor& visitor)
    {
        const auto [node, _] = FindNode(path);

        if ((node == nullptr) || (node->Type != NodeType::Folder))
            return false;

        FolderNode* dnode = static_cast<FolderNode*>(node);
        usize depth = 0;

        while (dnode)
        {
            StringView here = dnode->GetName().substr(path.size());

            if (visitor.VisitFolder(here))
            {
                for (FileNode* fnode = dnode->Files; fnode; fnode = fnode->Next)
                {
                    if (fnode->Type == NodeType::FileT)
                    {
                        StringView name = fnode->GetName();

                        visitor.VisitFile(Concat(here, name), static_cast<FileNodeT*>(fnode)->Entry);
                    }
                }

                if (dnode->Folders)
                {
                    dnode = dnode->Folders;
                    ++depth;
                    continue;
                }
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
        }

        return true;
    }

    template <typename T>
    template <typename F>
    inline Rc<Stream> VirtualFileSystem<T>::Open(StringView path, bool read_only, F callback)
    {
        const auto [node, _] = FindNode(path);

        if (node == nullptr || node->Type != NodeType::FileT)
            return nullptr;

        FileNodeT* fnode = static_cast<FileNodeT*>(node);

        Rc<Stream> result = fnode->Entry.Data;

        if (result != nullptr)
        {
            result->Seek(0, SeekWhence::Set);
        }
        else
        {
            result = callback(path, fnode->Entry.Entry);

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

        return ConvertToVirtualNode(path, fnode, !truncate ? callback(path, fnode->Entry.Entry) : nullptr);
    }

    template <typename T>
    template <typename F>
    Ptr<FindFileHandle> VirtualFileSystem<T>::Find(StringView path, F callback)
    {
        auto [node, hash] = FindNode(path);

        if ((node == nullptr) || (node->Type != NodeType::Folder))
            return nullptr;

        return MakeUnique<VirtualFindFileHandle<F>>(static_cast<FolderNode*>(node), std::move(callback));
    }

    template <typename T>
    template <typename F>
    class VirtualFileSystem<T>::VirtualFindFileHandle final : public FindFileHandle
    {
    public:
        VirtualFindFileHandle(FolderNode* folder, F callback)
            : files_(folder->Files)
            , folders_(folder->Folders)
            , prefix_(folder->GetName().size())
            , callback_(std::move(callback))
        {}

        bool Next(FolderEntry& entry) override
        {
            entry.Reset();

            if (files_)
            {
                entry.Name = files_->GetName();

                if (files_->Type == NodeType::FileT)
                {
                    FileEntry& fentry = static_cast<FileNodeT*>(files_)->Entry;

                    if (fentry.Data != nullptr)
                    {
                        entry.Size = fentry.Data->Size().get(0);
                    }
                    else
                    {
                        callback_(fentry.Entry, entry);
                    }
                }

                files_ = files_->Next;

                return true;
            }

            if (folders_)
            {
                StringView folder_path = folders_->GetName();

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
        usize prefix_ {0};
        F callback_;
    };

    template <typename T>
    inline typename VirtualFileSystem<T>::FileNodeT* VirtualFileSystem<T>::AddFileNode(
        StringView name, FileEntry entry, bool override)
    {
        if (name.empty() || name.back() == '/')
            return nullptr;

        const auto [dir_name, file_name, dir_hash, full_hash] = SplitPath(name);

        Node* node = FindNode(name, full_hash);

        if (node != nullptr)
        {
            if (override && node->Type == NodeType::FileT)
            {
                FileNodeT* fnode = static_cast<FileNodeT*>(node);
                fnode->Entry = std::move(entry);
                return fnode;
            }

            return nullptr;
        }

        FileNodeT* fnode = CreateNode<FileNodeT>(file_name, full_hash, std::move(entry));

        GetFolderNode(dir_name, dir_hash, true)->AddFile(fnode);
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
            node = AddFileNode(path, FileEntry {}, true);
        }

        FileEntry& entry = node->Entry;

        if (entry.Data == nullptr)
        {
            entry.Data = Stream::Temp();
        }
        else
        {
            entry.Data->SetSize(0);
            entry.Data->Seek(0, SeekWhence::Set);
        }

        Rc<Stream> result = entry.Data;

        if (input != nullptr)
        {
            input->CopyTo(*result);
            input = nullptr;
            result->Seek(0, SeekWhence::Set);
        }

        return result;
    }
} // namespace Iridium
