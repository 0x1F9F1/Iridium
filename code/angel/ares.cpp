#include "ares.h"

#include "core/bits.h"

#include "asset/stream.h"
#include "asset/stream/partial.h"

#include <charconv>

namespace Iridium::Angel
{
    AresArchive::AresArchive(Rc<Stream> input)
        : input_(std::move(input))
    {
        IrAssert(RefreshFileList(), "Invalid Archive");
    }

    Rc<Stream> AresArchive::Open(StringView path, bool read_only)
    {
        return vfs_.Open(
            path, read_only, [this](StringView path, VirtualFileINode& entry) { return OpenEntry(path, entry); });
    }

    Rc<Stream> AresArchive::Create(StringView path, bool /*write_only*/, bool truncate)
    {
        return vfs_.Create(
            path, truncate, [this](StringView path, VirtualFileINode& entry) { return OpenEntry(path, entry); });
    }

    bool AresArchive::Exists(StringView path)
    {
        return vfs_.Exists(path);
    }

    Ptr<FindFileHandle> AresArchive::Find(StringView path)
    {
        return vfs_.Find(
            path, [](const VirtualFileINode& entry, FolderEntry& output) { output.Size = entry.GetSize(); });
    }

    struct PathVisitor
    {
        void Visit(Vec<String>& files)
        {
            StringView here;

            for (StringView file : files)
            {
                while ((here.size() > file.size()) || (here != file.substr(0, here.size())))
                {
                    usize i = here.substr(0, here.size() - 1).rfind('/');

                    if (i == StringView::npos)
                        i = 0;
                    else
                        i += 1;

                    EndFolder(here, here.substr(i, here.size() - i - 1));

                    here = here.substr(0, i);
                }

                while (true)
                {
                    usize i = file.find('/', here.size());

                    if (i == StringView::npos)
                        break;

                    StringView name = file.substr(here.size(), i - here.size());

                    here = file.substr(0, i + 1);

                    BeginFolder(here, name);
                }

                VisitFile(file, here, file.substr(here.size()));
            }

            while (!here.empty())
            {
                usize i = here.substr(0, here.size() - 1).rfind('/');

                if (i == StringView::npos)
                    i = 0;
                else
                    i += 1;

                EndFolder(here, here.substr(i, here.size() - i - 1));

                here = here.substr(0, i);
            }
        }

        virtual void BeginFolder(StringView path, StringView name) = 0;
        virtual void EndFolder(StringView path, StringView name) = 0;
        virtual void VisitFile(StringView path, StringView dir, StringView name) = 0;
    };

    struct AresNodeBuilder : PathVisitor
    {
        AresHeader header;
        Vec<VirtualFileINode> nodes;
        Vec<char> name_heap;

        Vec<String> file_names;
        Vec<Vec<VirtualFileINode>> pending;

        void Init()
        {
            pending.emplace_back();
        }

        void Finalize()
        {
            u32 roots = static_cast<u32>(pending.back().size());

            nodes.insert(nodes.begin(), pending.back().begin(), pending.back().end());
            pending.pop_back();

            for (usize i = 0; i < nodes.size(); ++i)
            {
                if (nodes[i].dword8 & 0x1)
                {
                    nodes[i].dword0 += roots;
                }
            }

            header.Magic = 0x53455241;
            header.FileCount = static_cast<u32>(nodes.size());
            header.RootCount = roots;
            header.NamesSize = static_cast<u32>(name_heap.size());
        }

        u32 AddString(StringView name)
        {
            // TODO: Handle duplicates

            usize offset = name_heap.size();
            IrAssert(offset < 0x3FFFF, "Too many names");

            name_heap.insert(name_heap.end(), name.begin(), name.end());
            name_heap.emplace_back('\0');

            return static_cast<u32>(offset);
        }

        void BeginFolder(StringView /*path*/, StringView /*name*/) override
        {
            pending.emplace_back();
        }

        void EndFolder(StringView /*path*/, StringView name) override
        {
            u32 entry_index = static_cast<u32>(nodes.size());
            u32 entry_count = static_cast<u32>(pending.back().size());

            nodes.insert(nodes.end(), pending.back().begin(), pending.back().end());

            pending.pop_back();

            VirtualFileINode node {};
            node.dword0 = entry_index;
            node.dword4 = entry_count;
            node.dword8 = (AddString(name) << 14) | 0x1;

            pending.back().emplace_back(node);
        }

        void VisitFile(StringView path, StringView /*dir*/, StringView name) override
        {
            VirtualFileINode node {};
            node.dword0 = static_cast<u32>(file_names.size());
            node.dword4 = 0;
            node.dword8 = (AddString(name) << 14);

            file_names.emplace_back(path);
            pending.back().emplace_back(node);
        }
    };

    void AresArchive::Save(Rc<FileDevice> device, Vec<String> files, Rc<Stream> output)
    {
        for (String& file : files)
        {
            for (char& c : file)
            {
                if (c >= 'a' && c <= 'z')
                    c -= 0x20;
                else if (c == '\\')
                    c = '/';
            }
        }

        std::sort(files.begin(), files.end(), [](StringView lhs, StringView rhs) {
            for (usize i = 0, len = std::min(lhs.size(), rhs.size()); i < len; ++i)
            {
                const char c1 = lhs[i];
                const char c2 = rhs[i];

                if (c1 == c2)
                    continue;

                if (c1 == '/')
                    return true;

                if (c2 == '/')
                    return false;

                return (c1 < c2);
            }

            return lhs.size() < rhs.size();
        });

        AresNodeBuilder visitor;

        visitor.Init();
        visitor.Visit(files);
        visitor.Finalize();

        AresHeader& header = visitor.header;
        Vec<VirtualFileINode>& nodes = visitor.nodes;
        Vec<char>& name_heap = visitor.name_heap;
        Vec<String> file_names = visitor.file_names;

        constexpr u32 data_alignment = 0x10;

        u32 offset = static_cast<u32>(sizeof(header) + (nodes.size() * sizeof(VirtualFileINode)) + name_heap.size());

        for (VirtualFileINode& node : nodes)
        {
            if (node.dword8 & 0x1)
                continue;

            StringView file_name = file_names[node.dword0];

            node.dword0 = 0;

            Rc<Stream> input = device->Open(file_name, true);

            if (input == nullptr)
                continue;

            offset = bits::align(offset, data_alignment);

            output->Seek(offset, SeekWhence::Set);

            u64 length = input->CopyTo(*output);

            IrAssert(length <= 0x7FFFFF, "File Too Large");

            node.dword0 = offset;
            node.dword4 |= length;

            offset += static_cast<u32>(length);
        }

        output->Seek(0, SeekWhence::Set);

        output->Write(&header, sizeof(header));
        output->Write(nodes.data(), nodes.size() * sizeof(VirtualFileINode));
        output->Write(name_heap.data(), name_heap.size());
    } // namespace Iridium::Angel

    bool AresArchive::RefreshFileList()
    {
        input_->Seek(0, SeekWhence::Set);

        AresHeader header {};

        if (input_->Read(&header, sizeof(header)) != sizeof(header))
            return false;

        if (header.Magic != 0x53455241)
            return false;

        Vec<VirtualFileINode> nodes(header.FileCount);

        if (input_->Read(nodes.data(), nodes.size() * sizeof(VirtualFileINode)) !=
            nodes.size() * sizeof(VirtualFileINode))
            return false;

        Vec<char> names(header.NamesSize);

        if (input_->Read(names.data(), names.size()) != names.size())
            return false;

        if (names.back() != '\0')
            names.emplace_back('\0');

        vfs_.Reserve(header.FileCount);

        String path;
        path.reserve(128);

        for (u32 i = 0; i < header.RootCount; ++i)
        {
            AddFileNode(nodes, names, i, path);
        }

        return true;
    }

    void AresArchive::AddFileNode(const Vec<VirtualFileINode>& nodes, const Vec<char>& names, u32 index, String& path)
    {
        usize const old_size = path.size();

        const VirtualFileINode& node = nodes.at(index);

        for (const char* v = &names.at(node.GetNameOffset()); *v; ++v)
        {
            if (*v == '\1')
            {
                char name_integer[16];

                auto [p, ec] = std::to_chars(name_integer, name_integer + 16, node.GetNameInteger());

                IrDebugAssert(ec == std::errc(), "Failed to stringify integer");

                path.append(name_integer, p - name_integer);
            }
            else
            {
                path.push_back(*v);
            }
        }

        if (u32 ext_offset = node.GetExtOffset(); ext_offset != 0)
        {
            path.push_back('.');
            path += &names.at(ext_offset);
        }

        if (node.IsDirectory())
        {
            path.push_back('/');

            for (u32 i = node.GetEntryIndex(), end = i + node.GetEntryCount(); i < end; ++i)
            {
                AddFileNode(nodes, names, i, path);
            }
        }
        else
        {
            vfs_.AddFile(path, node);
        }

        path.resize(old_size);
    }

    Rc<Stream> AresArchive::OpenEntry(StringView /*path*/, VirtualFileINode& entry)
    {
        return MakeRc<PartialStream>(entry.GetOffset(), entry.GetSize(), input_);
    }
} // namespace Iridium::Angel
