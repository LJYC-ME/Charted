#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <concepts>
#include <memory_resource>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

namespace charted
{
    enum class RouteTokenType : std::uint8_t
    {
        Key,
        Index
    };

    struct RouteToken
    {
        const char*      Ptr   { nullptr };
        std::uint16_t    Length{ 0 };
        RouteTokenType   Type  { RouteTokenType::Key };
        std::uint32_t    Index { 0 };

        [[nodiscard]] std::string_view GetString() const noexcept
        {
            return (Ptr != nullptr && Length > 0)
                ? std::string_view(Ptr, static_cast<std::size_t>(Length))
                : std::string_view{};
        }
    };

    static_assert(sizeof(RouteToken) == 16, "RouteToken expected to be 16 bytes.");

    template <std::size_t N>
    struct StringLiteral
    {
        char Data[N]{};

        constexpr StringLiteral(const char (&value)[N]) noexcept
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                Data[i] = value[i];
            }
        }

        [[nodiscard]] static consteval std::size_t Size() noexcept { return N; }
    };

    template <std::size_t N>
    StringLiteral(const char (&)[N]) -> StringLiteral<N>;

    namespace detail
    {
        template <std::size_t MaxTokens>
        struct ParsedStaticRoute
        {
            std::array<RouteToken, MaxTokens> Tokens{};
            std::size_t                       Count{ 0 };
            bool                              Valid{ true };
        };

        template <StringLiteral Path, std::size_t MaxTokens>
        consteval ParsedStaticRoute<MaxTokens> ParseStaticRoute() noexcept
        {
            ParsedStaticRoute<MaxTokens> parsed{};
            constexpr std::size_t pathSize = Path.Size();
            constexpr std::size_t length   = (pathSize > 0) ? (pathSize - 1) : 0;
            constexpr std::size_t maxTokenLength = 65535;

            std::size_t cursor = 0;
            while (cursor < length)
            {
                if (Path.Data[cursor] == '.')
                {
                    if ((cursor + 1) < length && Path.Data[cursor + 1] == '.')
                    {
                        parsed.Valid = false;
                        return parsed;
                    }
                    ++cursor;
                    continue;
                }

                const std::size_t keyStart = cursor;
                while (cursor < length && Path.Data[cursor] != '.' && Path.Data[cursor] != '[')
                {
                    ++cursor;
                }

                if (keyStart < cursor)
                {
                    if (parsed.Count >= MaxTokens)
                    {
                        parsed.Valid = false;
                        return parsed;
                    }
                    if ((cursor - keyStart) > maxTokenLength)
                    {
                        parsed.Valid = false;
                        return parsed;
                    }
                    parsed.Tokens[parsed.Count++] = RouteToken{
                        .Ptr    = Path.Data + keyStart,
                        .Length = static_cast<std::uint16_t>(cursor - keyStart),
                        .Type   = RouteTokenType::Key,
                        .Index  = 0
                    };
                }

                if (cursor < length && Path.Data[cursor] == '[')
                {
                    ++cursor;
                    const std::size_t indexStart = cursor;
                    while (cursor < length && Path.Data[cursor] != ']')
                    {
                        const char c = Path.Data[cursor];
                        if (c < '0' || c > '9')
                        {
                            parsed.Valid = false;
                            return parsed;
                        }
                        ++cursor;
                    }

                    if (cursor >= length || indexStart == cursor)
                    {
                        parsed.Valid = false;
                        return parsed;
                    }

                    std::uint32_t index = 0;
                    for (std::size_t i = indexStart; i < cursor; ++i)
                    {
                        index = (index * 10u) + static_cast<std::uint32_t>(Path.Data[i] - '0');
                    }

                    if (parsed.Count >= MaxTokens)
                    {
                        parsed.Valid = false;
                        return parsed;
                    }
                    if ((cursor - indexStart) > maxTokenLength)
                    {
                        parsed.Valid = false;
                        return parsed;
                    }

                    parsed.Tokens[parsed.Count++] = RouteToken{
                        .Ptr    = Path.Data + indexStart,
                        .Length = static_cast<std::uint16_t>(cursor - indexStart),
                        .Type   = RouteTokenType::Index,
                        .Index  = index
                    };

                    ++cursor;
                }

                if (cursor < length && Path.Data[cursor] == '.')
                {
                    if ((cursor + 1) < length && Path.Data[cursor + 1] == '.')
                    {
                        parsed.Valid = false;
                        return parsed;
                    }
                    ++cursor;
                }
            }

            return parsed;
        }

        inline std::size_t EstimateTokenCapacity(std::string_view path) noexcept
        {
            if (path.empty())
            {
                return 0;
            }

            std::size_t capacity = 1;
            for (const char c : path)
            {
                if (c == '.' || c == '[')
                {
                    ++capacity;
                }
            }
            return capacity;
        }

        inline bool ParseDynamicRoute(std::string_view path, std::pmr::vector<RouteToken>& outTokens) noexcept
        {
            outTokens.clear();
            constexpr std::size_t maxTokenLength = 65535;
            std::size_t cursor = 0;
            while (cursor < path.size())
            {
                if (path[cursor] == '.')
                {
                    if ((cursor + 1) < path.size() && path[cursor + 1] == '.')
                    {
                        outTokens.clear();
                        return false;
                    }
                    ++cursor;
                    continue;
                }

                const std::size_t keyStart = cursor;
                while (cursor < path.size() && path[cursor] != '.' && path[cursor] != '[')
                {
                    ++cursor;
                }

                if (keyStart < cursor)
                {
                    if ((cursor - keyStart) > maxTokenLength)
                    {
                        outTokens.clear();
                        return false;
                    }
                    outTokens.push_back(RouteToken{
                        .Ptr    = path.data() + keyStart,
                        .Length = static_cast<std::uint16_t>(cursor - keyStart),
                        .Type   = RouteTokenType::Key,
                        .Index  = 0
                    });
                }

                if (cursor < path.size() && path[cursor] == '[')
                {
                    ++cursor;
                    const std::size_t indexStart = cursor;
                    while (cursor < path.size() && path[cursor] != ']')
                    {
                        const char c = path[cursor];
                        if (c < '0' || c > '9')
                        {
                            outTokens.clear();
                            return false;
                        }
                        ++cursor;
                    }

                    if (cursor >= path.size() || indexStart == cursor)
                    {
                        outTokens.clear();
                        return false;
                    }

                    std::uint32_t index = 0;
                    for (std::size_t i = indexStart; i < cursor; ++i)
                    {
                        index = (index * 10u) + static_cast<std::uint32_t>(path[i] - '0');
                    }
                    if ((cursor - indexStart) > maxTokenLength)
                    {
                        outTokens.clear();
                        return false;
                    }

                    outTokens.push_back(RouteToken{
                        .Ptr    = path.data() + indexStart,
                        .Length = static_cast<std::uint16_t>(cursor - indexStart),
                        .Type   = RouteTokenType::Index,
                        .Index  = index
                    });

                    ++cursor;
                }

                if (cursor < path.size() && path[cursor] == '.')
                {
                    if ((cursor + 1) < path.size() && path[cursor + 1] == '.')
                    {
                        outTokens.clear();
                        return false;
                    }
                    ++cursor;
                }
            }

            return true;
        }
    } // namespace detail

    class DynamicRoute
    {
    public:
        DynamicRoute()
            : DynamicRoute(std::string_view{})
        {
        }

        explicit DynamicRoute(
            std::string_view path,
            std::pmr::memory_resource* upstream = std::pmr::get_default_resource())
            : Resource(InlineBuffer.data(), InlineBuffer.size(), upstream)
            , Path(path, &Resource)
            , Tokens(&Resource)
        {
            Tokens.reserve(detail::EstimateTokenCapacity(Path));
            Parse();
        }

        explicit DynamicRoute(
            std::string path,
            std::pmr::memory_resource* upstream = std::pmr::get_default_resource())
            : Resource(InlineBuffer.data(), InlineBuffer.size(), upstream)
            , Path(path, &Resource)
            , Tokens(&Resource)
        {
            Tokens.reserve(detail::EstimateTokenCapacity(Path));
            Parse();
        }

        DynamicRoute(const DynamicRoute& other)
            : Resource(InlineBuffer.data(), InlineBuffer.size(), other.Resource.upstream_resource())
            , Path(other.Path, &Resource)
            , Tokens(&Resource)
        {
            Tokens.reserve(other.Tokens.size());
            Parse();
        }

        DynamicRoute(DynamicRoute&& other) noexcept
            : Resource(InlineBuffer.data(), InlineBuffer.size(), other.Resource.upstream_resource())
            , Path(other.Path, &Resource)
            , Tokens(&Resource)
        {
            Tokens.reserve(other.Tokens.size());
            Parse();
        }

        DynamicRoute& operator=(const DynamicRoute& other)
        {
            if (this == &other)
            {
                return *this;
            }

            Resource.release();
            Path = other.Path;
            Tokens.reserve(other.Tokens.size());
            Parse();
            return *this;
        }

        DynamicRoute& operator=(DynamicRoute&& other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }

            Resource.release();
            Path = other.Path;
            Tokens.reserve(other.Tokens.size());
            Parse();
            return *this;
        }

        [[nodiscard]] std::string_view GetPathString() const noexcept { return Path; }
        [[nodiscard]] std::span<const RouteToken> GetTokens() const noexcept { return Tokens; }
        [[nodiscard]] std::size_t GetTokenCount() const noexcept { return Tokens.size(); }
        [[nodiscard]] bool IsValid() const noexcept { return Valid; }
        [[nodiscard]] std::pmr::memory_resource* GetMemoryResource() const noexcept
        {
            return Resource.upstream_resource();
        }

    private:
        void Parse() { Valid = detail::ParseDynamicRoute(Path, Tokens); }

        static constexpr std::size_t InlineBufferSize = 256;
        std::array<std::byte, InlineBufferSize> InlineBuffer{};
        std::pmr::monotonic_buffer_resource     Resource;
        std::pmr::string                        Path;
        std::pmr::vector<RouteToken>            Tokens;
        bool                                    Valid{ true };
    };

    template <StringLiteral Path, std::size_t MaxTokens = (Path.Size() > 1 ? (Path.Size() - 1) : 1)>
    class StaticRoute
    {
    public:
        static_assert(MaxTokens > 0, "MaxTokens must be greater than 0.");
        static constexpr auto Parsed = detail::ParseStaticRoute<Path, MaxTokens>();
        static_assert(Parsed.Valid, "Invalid route literal.");

        [[nodiscard]] static constexpr std::string_view GetPathString() noexcept
        {
            return std::string_view(Path.Data, Path.Size() - 1);
        }

        [[nodiscard]] static constexpr std::span<const RouteToken> GetTokens() noexcept
        {
            return std::span<const RouteToken>(Parsed.Tokens.data(), Parsed.Count);
        }

        [[nodiscard]] static consteval std::size_t GetTokenCount() noexcept { return Parsed.Count; }
        [[nodiscard]] static consteval bool IsValid() noexcept { return Parsed.Valid; }
    };

    namespace concepts
    {
        template <typename T>
        struct IsStaticRoute : std::false_type
        {
        };

        template <StringLiteral Path, std::size_t MaxTokens>
        struct IsStaticRoute<StaticRoute<Path, MaxTokens>> : std::true_type
        {
        };

        template <typename T>
        concept Route =
            (std::same_as<std::remove_cvref_t<T>, DynamicRoute> ||
             IsStaticRoute<std::remove_cvref_t<T>>::value) &&
            requires(const std::remove_cvref_t<T>& route)
            {
                { route.GetPathString() } -> std::convertible_to<std::string_view>;
                { route.GetTokens() } -> std::convertible_to<std::span<const RouteToken>>;
            };
    } // namespace concepts
}