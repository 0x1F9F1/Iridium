#pragma once

namespace Iridium
{
    u32 StringLiteralHash(StringView string, u32 seed);
    u32 IdentLiteralHash(StringView string, u32 seed);

    struct StringHash
    {
        u32 Value {0};

        constexpr StringHash();
        constexpr StringHash(u32 value);

        static StringHash Hash(StringView string);
        static StringHash HashIdent(StringView string);

        constexpr bool operator==(const StringHash& other) const;
        constexpr bool operator!=(const StringHash& other) const;

        constexpr bool operator<(const StringHash& other) const;
    };

    struct StringHasher
    {
    public:
        constexpr StringHasher();
        constexpr StringHasher(u32 seed);

        void UpdateString(StringView string);
        void UpdateIdent(StringView string);

        [[nodiscard]] StringHash Finalize() const;

    private:
        u32 value_ {0};
    };

    inline constexpr StringHash::StringHash() = default;

    inline constexpr StringHash::StringHash(u32 value)
        : Value(value)
    { }

    inline constexpr bool StringHash::operator==(const StringHash& other) const
    {
        return Value == other.Value;
    }

    inline constexpr bool StringHash::operator!=(const StringHash& other) const
    {
        return Value != other.Value;
    }

    inline constexpr bool StringHash::operator<(const StringHash& other) const
    {
        return Value < other.Value;
    }

    inline StringHash StringHash::Hash(StringView string)
    {
        return StringHash(StringLiteralHash(string, 0));
    }

    inline StringHash StringHash::HashIdent(StringView string)
    {
        return StringHash(IdentLiteralHash(string, 0));
    }

    inline constexpr StringHasher::StringHasher() = default;

    inline constexpr StringHasher::StringHasher(u32 seed)
        : value_(seed)
    { }

    inline StringHash StringHasher::Finalize() const
    {
        u32 hash = value_;

        hash += hash << 3;
        hash ^= hash >> 11;
        hash += hash << 15;

        return StringHash(hash);
    }

    inline namespace Literals
    {
        inline StringHash operator""_sh(const char* string, usize length)
        {
            return StringHash(StringLiteralHash({string, length}, 0));
        }
    } // namespace Literals
} // namespace Iridium

namespace std
{
    template <>
    struct hash<Iridium::StringHash>
    {
        inline constexpr std::size_t operator()(const Iridium::StringHash& s) const noexcept
        {
            return static_cast<std::size_t>(s.Value);
        }
    };
} // namespace std
