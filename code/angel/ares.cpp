#include "ares.h"

#include "core/bits.h"

#include "asset/stream.h"
#include "asset/stream/partial.h"

#include <charconv>

namespace Iridium::Angel
{
    struct AresHeader
    {
        u32 Magic;
        u32 FileCount;
        u32 RootCount;
        u32 NamesSize;
    };

    static_assert(sizeof(AresHeader) == 0x10);

    struct VirtualFileINode
    {
        u32 dword0;
        u32 dword4;
        u32 dword8;

        u32 GetOffset() const
        {
            return dword0;
        }

        u32 GetSize() const
        {
            return dword4 & 0x7FFFFF;
        }

        u32 GetEntryIndex() const
        {
            return dword0;
        }

        u32 GetEntryCount() const
        {
            return dword4 & 0x7FFFFF;
        }

        bool IsDirectory() const
        {
            return (dword8 & 1) != 0;
        }

        u32 GetNameOffset() const
        {
            return (dword8 >> 14) & 0x3FFFF;
        }

        u32 GetExtOffset() const
        {
            return (dword4 >> 23) & 0x1FF;
        }

        u32 GetNameInteger() const
        {
            return (dword8 >> 1) & 0x1FFF;
        }

        void SetOffset(u32 offset)
        {
            dword0 = offset;
        }

        void SetSize(u32 size)
        {
            dword4 = (dword4 & 0xFF800000) | (size);
        }

        void SetEntryIndex(u32 index)
        {
            dword0 = index;
        }

        void SetEntryCount(u32 size)
        {
            dword4 = (dword4 & 0xFF800000) | (size);
        }

        void SetIsDirectory(bool is_dir)
        {
            dword8 = (dword8 & 0xFFFFFFFE) | u32(is_dir);
        }

        void SetNameOffset(u32 offset)
        {
            dword8 = (dword8 & 0x00003FFF) | (offset << 14);
        }

        void SetExtOffset(u32 offset)
        {
            dword4 = (dword4 & 0x007FFFFF) | (offset << 23);
        }

        void SetNameInteger(u32 value)
        {
            dword8 = (dword8 & 0xFFFFC001) | (value << 1);
        }
    };

    static_assert(sizeof(VirtualFileINode) == 0xC);

    AresArchive::AresArchive(Rc<Stream> input)
        : input_(std::move(input))
    {
        IrAssert(RefreshFileList(), "Invalid Archive");
    }

    bool AresArchive::RefreshFileList()
    {
        if (!input_->TrySeek(0))
            return false;

        AresHeader header {};

        if (!input_->TryRead(&header, sizeof(header)))
            return false;

        if (header.Magic != 0x53455241)
            return false;

        Vec<VirtualFileINode> nodes(header.FileCount);

        if (!input_->TryRead(nodes.data(), nodes.size() * sizeof(VirtualFileINode)))
            return false;

        Vec<char> names(header.NamesSize);

        if (!input_->TryRead(names.data(), names.size()))
            return false;

        if (names.empty() || names.back() != '\0')
            names.emplace_back('\0');

        vfs_.Reserve(header.FileCount);
        vfs_.FileContext = this;

        String path;
        path.reserve(128);

        for (u32 i = 0; i < header.RootCount; ++i)
            AddFileNode(nodes, names, i, path);

        return true;
    }

    struct AresFileNode final : VFS::FixedFileNode<VirtualFileINode>
    {
        using FixedFileNode::FixedFileNode;

        Rc<Stream> Open(void* ctx, bool /*read_only*/) override
        {
            const Rc<Stream>& input = static_cast<AresArchive*>(ctx)->GetInput();

            return MakeRc<PartialStream>(Entry.GetOffset(), Entry.GetSize(), input);
        }

        bool Stat(void* /*ctx*/, FolderEntry& entry) override
        {
            entry.Size = Entry.GetSize();

            return true;
        }
    };

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
            vfs_.AddFile<AresFileNode>(path, node);
        }

        path.resize(old_size);
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
        BTreeMap<String, u32> name_lookup;

        Vec<String> file_names;
        Vec<Vec<VirtualFileINode>> pending;

        Vec<char> ext_heap;
        BTreeMap<String, u32> ext_lookup;

        void Init()
        {
            ext_heap.emplace_back('\0');
            pending.emplace_back();
        }

        void Finalize()
        {
            u32 roots = static_cast<u32>(pending.back().size());

            nodes.insert(nodes.begin(), pending.back().begin(), pending.back().end());
            pending.clear();

            u32 exts_size = static_cast<u32>(ext_heap.size());
            name_heap.insert(name_heap.begin(), ext_heap.begin(), ext_heap.end());
            ext_heap.clear();

            for (VirtualFileINode& node : nodes)
            {
                if (node.IsDirectory())
                {
                    node.SetEntryIndex(node.GetEntryIndex() + roots);
                }

                node.SetNameOffset(node.GetNameOffset() + exts_size);
            }

            header.Magic = 0x53455241;
            header.FileCount = static_cast<u32>(nodes.size());
            header.RootCount = roots;
            header.NamesSize = static_cast<u32>(name_heap.size());
        }

        VirtualFileINode CreateNode(StringView name)
        {
            VirtualFileINode node {};

            if (usize ext_idx = name.rfind('.'); ext_idx != StringView::npos)
            {
                StringView ext = name.substr(ext_idx + 1);

                if (ext.size() > 0 && ext.size() < 24)
                {
                    auto find = ext_lookup.find(ext);

                    if (find == ext_lookup.end())
                    {
                        if (ext_heap.size() < 0x1FF)
                        {
                            u32 ext_offset = static_cast<u32>(ext_heap.size());

                            ext_heap.insert(ext_heap.end(), ext.begin(), ext.end());
                            ext_heap.emplace_back('\0');

                            find = ext_lookup.emplace(ext, ext_offset).first;
                        }
                    }

                    if (find != ext_lookup.end())
                    {
                        node.SetExtOffset(find->second);

                        name = name.substr(0, ext_idx);
                    }
                }
            }

            String modified_name;

            for (usize i = 0; i < name.size(); ++i)
            {
                if (char c = name[i]; c < '1' || c > '9')
                {
                    if (c != '0')
                        continue;

                    if ((i + 1) < name.size())
                    {
                        c = name[i + 1];

                        if ((c >= '0') && (c <= '9'))
                            continue;
                    }
                }

                usize j = i + 1;

                for (; j < name.size(); ++j)
                {
                    char c = name[j];

                    if ((c < '0') || (c > '9'))
                        break;
                }

                u32 value = 0;

                auto [_, ec] = std::from_chars(name.data() + i, name.data() + j, value);

                if (ec != std::errc())
                    continue;

                if (value > 0x1FFF)
                    continue;

                node.SetNameInteger(value);

                modified_name = name.substr(0, i);
                modified_name += '\1';
                modified_name += name.substr(j);

                name = modified_name;

                break;
            }

            auto find = name_lookup.find(name);

            if (find == name_lookup.end())
            {
                IrAssert((name_heap.size() + ext_heap.size()) < 0x3FFFF, "Too Many Names");

                u32 name_offset = static_cast<u32>(name_heap.size());

                name_heap.insert(name_heap.end(), name.begin(), name.end());
                name_heap.emplace_back('\0');

                find = name_lookup.emplace(name, name_offset).first;
            }

            node.SetNameOffset(find->second);

            return node;
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

            VirtualFileINode node = CreateNode(name);

            node.SetIsDirectory(true);
            node.SetEntryIndex(entry_index);
            node.SetEntryCount(entry_count);

            pending.back().emplace_back(node);
        }

        void VisitFile(StringView path, StringView /*dir*/, StringView name) override
        {
            VirtualFileINode node = CreateNode(name);

            node.SetOffset(static_cast<u32>(file_names.size()));

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

        AresHeader header = std::move(visitor.header);
        Vec<VirtualFileINode> nodes = std::move(visitor.nodes);
        Vec<char> name_heap = std::move(visitor.name_heap);
        Vec<String> file_names = std::move(visitor.file_names);

        constexpr u32 data_alignment = 0x10;

        u32 offset = static_cast<u32>(sizeof(header) + (nodes.size() * sizeof(VirtualFileINode)) + name_heap.size());

        for (VirtualFileINode& node : nodes)
        {
            if (node.IsDirectory())
                continue;

            StringView file_name = file_names[node.dword0];

            node.SetOffset(0);
            node.SetSize(0);

            Rc<Stream> input = device->Open(file_name, true);

            if (input == nullptr)
                continue;

            offset = bits::align(offset, data_alignment);

            output->Seek(offset, SeekWhence::Set);

            u64 length = input->CopyTo(*output);

            IrAssert(length <= 0x7FFFFF, "File Too Large");

            node.SetOffset(offset);
            node.SetSize(static_cast<u32>(length));

            offset += static_cast<u32>(length);
        }

        output->Seek(0, SeekWhence::Set);

        output->Write(&header, sizeof(header));
        output->Write(nodes.data(), nodes.size() * sizeof(VirtualFileINode));
        output->Write(name_heap.data(), name_heap.size());
    } // namespace Iridium::Angel
} // namespace Iridium::Angel
