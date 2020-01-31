#pragma once

namespace Iridium
{
    class BuzHash
    {
    public:
        BuzHash(usize len) noexcept
            : shift_(len & 0x1F)
        {}

        u32 get() const noexcept
        {
            return hash_;
        }

        void reset() noexcept
        {
            hash_ = 0;
        }

        void eat(u8 input) noexcept
        {
            hash_ = (hash_ << 1 | hash_ >> 31) ^ lookup_[input];
        }

        void update(u8 input, u8 old) noexcept
        {
            u32 const new_value = lookup_[input];
            u32 const old_value = lookup_[old];
            hash_ = (hash_ << 1 | hash_ >> 31) ^ new_value ^ (old_value << shift_ | old_value >> (32 - shift_));
        }

        static u32 Hash(const void* input, usize len) noexcept
        {
            BuzHash hasher(len);

            for (usize i = 0; i < len; ++i)
                hasher.eat(static_cast<const u8*>(input)[i]);

            return hasher.get();
        }

    private:
        u32 hash_ {0};
        u32 const shift_ {0};

        static const u32 lookup_[256];
    };

    class Stream;

    struct BuzEntry
    {
        u32 BHash {0};
        u32 JHash {0};

        inline friend bool operator==(const BuzEntry& lhs, const BuzEntry& rhs) noexcept
        {
            return (lhs.BHash == rhs.BHash) && (lhs.JHash == rhs.JHash);
        }

        inline friend bool operator<(const BuzEntry& lhs, u32 value) noexcept
        {
            return lhs.BHash < value;
        }

        inline friend bool operator<(u32 value, const BuzEntry& lhs) noexcept
        {
            return value < lhs.BHash;
        }
    };

    BuzEntry HashBytes(const void* input, usize len);

    Option<Vec<u8>> FindBytes(Stream& input, BuzEntry entry, usize len);
    HashMap<BuzEntry, Vec<u8>> FindBytes(Stream& input, Vec<BuzEntry> entries, usize len);

    class BuzFinder
    {
    public:
        void Add(BuzEntry hash, void* output, usize length);
        bool Find(Rc<Stream> input);

    private:
        HashMap<usize, Vec<Pair<BuzEntry, u8*>>> sections_;
    };
} // namespace Iridium

// custom specialization of std::hash can be injected in namespace std
namespace std
{
    template <>
    struct hash<Iridium::BuzEntry>
    {
        std::size_t operator()(const Iridium::BuzEntry& s) const noexcept
        {
            return std::hash<Iridium::u32> {}(s.BHash) ^ std::hash<Iridium::u32> {}(s.JHash);
        }
    };
} // namespace std
