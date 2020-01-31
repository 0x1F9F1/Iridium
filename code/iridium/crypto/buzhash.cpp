#include "buzhash.h"

#include "asset/stream.h"
#include "asset/stream/bulk.h"

#include "core/joaat.h"
#include "core/parallel.h"

#include <bitset>

namespace Iridium
{
    constexpr const u32 BuzHash::lookup_[256] {
        // Fractional Part of Pi
        // clang-format off
        0x243F6A88, 0x85A308D3, 0x13198A2E, 0x03707344, 0xA4093822, 0x299F31D0, 0x082EFA98, 0xEC4E6C89,
        0x452821E6, 0x38D01377, 0xBE5466CF, 0x34E90C6C, 0xC0AC29B7, 0xC97C50DD, 0x3F84D5B5, 0xB5470917,
        0x9216D5D9, 0x8979FB1B, 0xD1310BA6, 0x98DFB5AC, 0x2FFD72DB, 0xD01ADFB7, 0xB8E1AFED, 0x6A267E96,
        0xBA7C9045, 0xF12C7F99, 0x24A19947, 0xB3916CF7, 0x0801F2E2, 0x858EFC16, 0x636920D8, 0x71574E69,
        0xA458FEA3, 0xF4933D7E, 0x0D95748F, 0x728EB658, 0x718BCD58, 0x82154AEE, 0x7B54A41D, 0xC25A59B5,
        0x9C30D539, 0x2AF26013, 0xC5D1B023, 0x286085F0, 0xCA417918, 0xB8DB38EF, 0x8E79DCB0, 0x603A180E,
        0x6C9E0E8B, 0xB01E8A3E, 0xD71577C1, 0xBD314B27, 0x78AF2FDA, 0x55605C60, 0xE65525F3, 0xAA55AB94,
        0x57489862, 0x63E81440, 0x55CA396A, 0x2AAB10B6, 0xB4CC5C34, 0x1141E8CE, 0xA15486AF, 0x7C72E993,
        0xB3EE1411, 0x636FBC2A, 0x2BA9C55D, 0x741831F6, 0xCE5C3E16, 0x9B87931E, 0xAFD6BA33, 0x6C24CF5C,
        0x7A325381, 0x28958677, 0x3B8F4898, 0x6B4BB9AF, 0xC4BFE81B, 0x66282193, 0x61D809CC, 0xFB21A991,
        0x487CAC60, 0x5DEC8032, 0xEF845D5D, 0xE98575B1, 0xDC262302, 0xEB651B88, 0x23893E81, 0xD396ACC5,
        0x0F6D6FF3, 0x83F44239, 0x2E0B4482, 0xA4842004, 0x69C8F04A, 0x9E1F9B5E, 0x21C66842, 0xF6E96C9A,
        0x670C9C61, 0xABD388F0, 0x6A51A0D2, 0xD8542F68, 0x960FA728, 0xAB5133A3, 0x6EEF0B6C, 0x137A3BE4,
        0xBA3BF050, 0x7EFB2A98, 0xA1F1651D, 0x39AF0176, 0x66CA593E, 0x82430E88, 0x8CEE8619, 0x456F9FB4,
        0x7D84A5C3, 0x3B8B5EBE, 0xE06F75D8, 0x85C12073, 0x401A449F, 0x56C16AA6, 0x4ED3AA62, 0x363F7706,
        0x1BFEDF72, 0x429B023D, 0x37D0D724, 0xD00A1248, 0xDB0FEAD3, 0x49F1C09B, 0x075372C9, 0x80991B7B,
        0x25D479D8, 0xF6E8DEF7, 0xE3FE501A, 0xB6794C3B, 0x976CE0BD, 0x04C006BA, 0xC1A94FB6, 0x409F60C4,
        0x5E5C9EC2, 0x196A2463, 0x68FB6FAF, 0x3E6C53B5, 0x1339B2EB, 0x3B52EC6F, 0x6DFC511F, 0x9B30952C,
        0xCC814544, 0xAF5EBD09, 0xBEE3D004, 0xDE334AFD, 0x660F2807, 0x192E4BB3, 0xC0CBA857, 0x45C8740F,
        0xD20B5F39, 0xB9D3FBDB, 0x5579C0BD, 0x1A60320A, 0xD6A100C6, 0x402C7279, 0x679F25FE, 0xFB1FA3CC,
        0x8EA5E9F8, 0xDB3222F8, 0x3C7516DF, 0xFD616B15, 0x2F501EC8, 0xAD0552AB, 0x323DB5FA, 0xFD238760,
        0x53317B48, 0x3E00DF82, 0x9E5C57BB, 0xCA6F8CA0, 0x1A87562E, 0xDF1769DB, 0xD542A8F6, 0x287EFFC3,
        0xAC6732C6, 0x8C4F5573, 0x695B27B0, 0xBBCA58C8, 0xE1FFA35D, 0xB8F011A0, 0x10FA3D98, 0xFD2183B8,
        0x4AFCB56C, 0x2DD1D35B, 0x9A53E479, 0xB6F84565, 0xD28E49BC, 0x4BFB9790, 0xE1DDF2DA, 0xA4CB7E33,
        0x62FB1341, 0xCEE4C6E8, 0xEF20CADA, 0x36774C01, 0xD07E9EFE, 0x2BF11FB4, 0x95DBDA4D, 0xAE909198,
        0xEAAD8E71, 0x6B93D5A0, 0xD08ED1D0, 0xAFC725E0, 0x8E3C5B2F, 0x8E7594B7, 0x8FF6E2FB, 0xF2122B64,
        0x8888B812, 0x900DF01C, 0x4FAD5EA0, 0x688FC31C, 0xD1CFF191, 0xB3A8C1AD, 0x2F2F2218, 0xBE0E1777,
        0xEA752DFE, 0x8B021FA1, 0xE5A0CC0F, 0xB56F74E8, 0x18ACF3D6, 0xCE89E299, 0xB4A84FE0, 0xFD13E0B7,
        0x7CC43B81, 0xD2ADA8D9, 0x165FA266, 0x80957705, 0x93CC7314, 0x211A1477, 0xE6AD2065, 0x77B5FA86,
        0xC75442F5, 0xFB9D35CF, 0xEBCDAF0C, 0x7B3E89A0, 0xD6411BD3, 0xAE1E7E49, 0x00250E2D, 0x2071B35E,
        0x226800BB, 0x57B8E0AF, 0x2464369B, 0xF009B91E, 0x5563911D, 0x59DFA6AA, 0x78C14389, 0xD95A537F,
        0x207D5BA2, 0x02E5B9C5, 0x83260376, 0x6295CFA9, 0x11C81968, 0x4E734A41, 0xB3472DCA, 0x7B14A94A,
        // clang-format on
    };

    BuzEntry HashBytes(const void* input, usize len)
    {
        return {BuzHash::Hash(input, len), joaat(input, len)};
    }

    Option<Vec<u8>> FindBytes(Stream& input, BuzEntry entry, usize len)
    {
        constexpr usize BufferSize = 0x10000;

        BuzHash hasher(len);

        Vec<u8> buffer(len + BufferSize);

        usize n = input.Read(buffer.data(), buffer.size());

        if (n < len)
            return None;

        for (usize i = 0; i < len; ++i)
            hasher.eat(buffer[i]);

        if (hasher.get() == entry.BHash && joaat(&buffer[0], len) == entry.JHash)
            return Vec<u8> {&buffer[0], &buffer[len]};

        for (n -= len; n; n = input.Read(&buffer[len], BufferSize))
        {
            const u8* here = buffer.data();
            const u8* const end = here + n;

            while (here < end)
            {
                hasher.update(here[len], *here);

                ++here;

                if (hasher.get() == entry.BHash && joaat(here, len) == entry.JHash)
                    return Vec<u8> {here, here + len};
            }

            std::memcpy(&buffer[0], end, len);
        }

        return None;
    }

    HashMap<BuzEntry, Vec<u8>> FindBytes(Stream& input, Vec<BuzEntry> entries, usize len)
    {
        std::sort(entries.begin(), entries.end(), [](const BuzEntry& lhs, const BuzEntry& rhs) {
            if (lhs.BHash != rhs.BHash)
                return lhs.BHash < rhs.BHash;

            return lhs.JHash < rhs.JHash;
        });

        entries.erase(std::unique(entries.begin(), entries.end()), entries.end());

        HashMap<BuzEntry, Vec<u8>> result;

        if (entries.size() == 1)
        {
            Option<Vec<u8>> find = FindBytes(input, entries[0], len);

            if (find)
                result.try_emplace(entries[0], std::move(*find));

            return result;
        }

        constexpr usize BufferSize = 0x10000;
        constexpr usize FilterSize = 0x10000;
        constexpr usize FilterMask = FilterSize - 1;

        std::bitset<FilterSize> filter;

        for (const BuzEntry& entry : entries)
            filter.set(entry.BHash & FilterMask);

        BuzHash hasher(len);

        Vec<u8> buffer(len + BufferSize);

        usize n = input.Read(buffer.data(), buffer.size());

        const BuzEntry* const entries_beg = entries.data();
        const BuzEntry* const entries_end = entries.data() + entries.size();

        if (n < len)
            return result;

        for (usize i = 0; i < len; ++i)
            hasher.eat(buffer[i]);

        if (filter.test(hasher.get() & FilterMask))
        {
            if (const auto find = std::equal_range(entries_beg, entries_end, hasher.get()); find.first != find.second)
            {
                u32 const jhash = joaat(&buffer[0], len);

                for (auto i = find.first; i != find.second; ++i)
                {
                    if (jhash == i->JHash)
                    {
                        result.try_emplace(*i, &buffer[0], &buffer[len]);

                        break;
                    }
                }
            }
        }

        for (n -= len; n; n = input.Read(&buffer[len], BufferSize))
        {
            const u8* here = buffer.data();
            const u8* const end = here + n;

            while (here < end)
            {
                hasher.update(here[len], *here);

                ++here;

                if (filter.test(hasher.get() & FilterMask))
                {
                    if (const auto find = std::equal_range(entries_beg, entries_end, hasher.get());
                        find.first != find.second)
                    {
                        u32 const jhash = joaat(here, len);

                        for (auto i = find.first; i != find.second; ++i)
                        {
                            if (jhash == i->JHash)
                            {
                                if (result.try_emplace(*i, here, here + len).second &&
                                    (result.size() == entries.size()))
                                    return result;

                                break;
                            }
                        }
                    }
                }
            }

            std::memcpy(&buffer[0], end, len);
        }

        return result;
    }

    void BuzFinder::Add(BuzEntry hash, void* output, usize length)
    {
        sections_[length].push_back({hash, (u8*) output});
    }

    bool BuzFinder::Find(Rc<Stream> input)
    {
        Rc<Stream> sync_input = Stream::BulkSync(std::move(input));

        Atomic<bool> found = true;

        parallel_for_each(sections_.begin(), sections_.end(), [&found, sync_input](const auto& section) {
            const auto& [length, values] = section;

            Vec<BuzEntry> entries(values.size());

            for (usize i = 0; i < values.size(); ++i)
                entries[i] = values[i].first;

            BulkStream input(sync_input);

            HashMap<BuzEntry, Vec<u8>> results = FindBytes(input, std::move(entries), length);

            for (const auto& entry : values)
            {
                auto find = results.find(entry.first);

                if (find == results.end())
                {
                    found = false;

                    return false;
                }

                std::memcpy(entry.second, find->second.data(), length);
            }

            return true;
        });

        return found;
    }

} // namespace Iridium
