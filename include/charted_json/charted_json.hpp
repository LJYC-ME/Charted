#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../charted/charted.hpp"
#include "nlohmann/json.hpp"

namespace charted
{
    class Json
    {
        using NativeJson = nlohmann::json;

    public:
        Json() = default;

        explicit Json(NativeJson value) noexcept
            : Root(std::move(value))
        {
        }

        [[nodiscard]] static std::optional<Json> Parse(std::string_view jsonText) noexcept
        {
            try
            {
                return Json(NativeJson::parse(jsonText));
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        [[nodiscard]] bool IsNull() const noexcept { return Root.is_null(); }
        [[nodiscard]] bool IsDiscarded() const noexcept { return Root.is_discarded(); }
        [[nodiscard]] bool Contains(std::string_view key) const noexcept { return Root.contains(key); }

        void Clear() noexcept { Root = NativeJson{}; }

        [[nodiscard]] std::string Dump(bool pretty = true) const
        {
            return pretty ? Root.dump(4) : Root.dump();
        }

        template <typename T>
        Json& Set(std::string_view key, T&& value)
        {
            Root[std::string(key)] = ToNative(std::forward<T>(value));
            return *this;
        }

        template <concepts::Route TRoute, typename T>
        Json& Set(const TRoute& routeValue, T&& value)
        {
            if (!IsRouteValid(routeValue))
            {
                return *this;
            }
            return SetPathValue(routeValue, ToNative(std::forward<T>(value)));
        }

        template <typename T>
        [[nodiscard]] std::optional<T> TryGet(std::string_view key) const noexcept
        {
            try
            {
                const auto it = Root.find(key);
                if (it == Root.end())
                {
                    return std::nullopt;
                }
                return FromNative<T>(*it);
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        template <typename T>
        [[nodiscard]] T Get(std::string_view key, T defaultValue = T{}) const
        {
            auto value = TryGet<T>(key);
            return value.has_value() ? std::move(value.value()) : std::move(defaultValue);
        }

        template <typename T, concepts::Route TRoute>
        [[nodiscard]] std::optional<T> TryGet(const TRoute& routeValue) const noexcept
        {
            if (!IsRouteValid(routeValue))
            {
                return std::nullopt;
            }

            try
            {
                const NativeJson* found = FindPath(Root, routeValue);
                if (found == nullptr)
                {
                    return std::nullopt;
                }
                return FromNative<T>(*found);
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        template <typename T, concepts::Route TRoute>
        [[nodiscard]] T Get(const TRoute& routeValue, T defaultValue = T{}) const
        {
            auto value = TryGet<T>(routeValue);
            return value.has_value() ? std::move(value.value()) : std::move(defaultValue);
        }

        [[nodiscard]] NativeJson& GetNative() noexcept { return Root; }
        [[nodiscard]] const NativeJson& GetNative() const noexcept { return Root; }

    private:
        template <typename T>
        [[nodiscard]] static NativeJson ToNative(T&& value)
        {
            if constexpr (std::same_as<std::remove_cvref_t<T>, Json>)
            {
                return value.GetNative();
            }
            else if constexpr (std::same_as<std::remove_cvref_t<T>, std::string_view>)
            {
                return NativeJson(std::string(value));
            }
            else
            {
                return NativeJson(std::forward<T>(value));
            }
        }

        template <typename T>
        [[nodiscard]] static std::optional<T> FromNative(const NativeJson& value)
        {
            if constexpr (std::same_as<T, Json>)
            {
                return std::optional<T>{ T(value) };
            }
            else
            {
                return std::optional<T>{ value.template get<T>() };
            }
        }

        template <concepts::Route TRoute>
        static bool IsRouteValid(const TRoute& routeValue)
        {
            if constexpr (requires { { routeValue.IsValid() } -> std::convertible_to<bool>; })
            {
                return static_cast<bool>(routeValue.IsValid());
            }
            else
            {
                return true;
            }
        }

        template <concepts::Route TRoute>
        Json& SetPathValue(const TRoute& routeValue, NativeJson value)
        {
            const auto tokens = routeValue.GetTokens();
            if (tokens.empty())
            {
                return *this;
            }

            if (!Root.is_object())
            {
                Root = NativeJson::object();
            }

            NativeJson* current = std::addressof(Root);
            for (std::size_t i = 0; i + 1 < tokens.size(); ++i)
            {
                const RouteToken& token = tokens[i];
                const RouteToken& next  = tokens[i + 1];

                if (token.Type == RouteTokenType::Key)
                {
                    if (!current->is_object())
                    {
                        *current = NativeJson::object();
                    }
                    const std::string key(token.GetString());
                    NativeJson& child = (*current)[key];
                    if (next.Type == RouteTokenType::Key)
                    {
                        if (!child.is_object())
                        {
                            child = NativeJson::object();
                        }
                    }
                    else if (!child.is_array())
                    {
                        child = NativeJson::array();
                    }
                    current = std::addressof(child);
                }
                else
                {
                    if (!current->is_array())
                    {
                        *current = NativeJson::array();
                    }
                    while (token.Index >= current->size())
                    {
                        current->push_back(NativeJson{});
                    }
                    NativeJson& child = (*current)[token.Index];
                    if (next.Type == RouteTokenType::Key)
                    {
                        if (!child.is_object())
                        {
                            child = NativeJson::object();
                        }
                    }
                    else if (!child.is_array())
                    {
                        child = NativeJson::array();
                    }
                    current = std::addressof(child);
                }
            }

            const RouteToken& last = tokens.back();
            if (last.Type == RouteTokenType::Key)
            {
                if (!current->is_object())
                {
                    *current = NativeJson::object();
                }
                const std::string key(last.GetString());
                (*current)[key] = std::move(value);
            }
            else
            {
                if (!current->is_array())
                {
                    *current = NativeJson::array();
                }
                while (last.Index >= current->size())
                {
                    current->push_back(NativeJson{});
                }
                (*current)[last.Index] = std::move(value);
            }

            return *this;
        }

        template <concepts::Route TRoute>
        [[nodiscard]] static const NativeJson* FindPath(const NativeJson& root, const TRoute& routeValue) noexcept
        {
            const auto tokens = routeValue.GetTokens();
            const NativeJson* current = std::addressof(root);

            for (const RouteToken& token : tokens)
            {
                if (token.Type == RouteTokenType::Key)
                {
                    if (!current->is_object())
                    {
                        return nullptr;
                    }
                    const std::string key(token.GetString());
                    const auto it = current->find(key);
                    if (it == current->end())
                    {
                        return nullptr;
                    }
                    current = std::addressof(*it);
                }
                else
                {
                    if (!current->is_array())
                    {
                        return nullptr;
                    }
                    if (token.Index >= current->size())
                    {
                        return nullptr;
                    }
                    current = std::addressof((*current)[token.Index]);
                }
            }

            return current;
        }

        NativeJson Root;
    };
}