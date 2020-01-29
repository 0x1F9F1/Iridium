#include "ares.h"

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
        usize old_size = path.size();

        const VirtualFileINode& node = nodes.at(index);

        u32 offset = node.GetOffset();
        u32 size = node.GetSize();

        u32 name_offset = node.GetNameOffset();

        for (const char* v = &names.at(name_offset); *v; ++v)
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

            for (u32 i = 0; i < size; ++i)
            {
                AddFileNode(nodes, names, offset + i, path);
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
