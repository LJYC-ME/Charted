#pragma once
#include <string_view>
#include "route/charted_route.hpp"

namespace charted
{
    [[nodiscard]] inline DynamicRoute route(std::string_view path)
    {
        return DynamicRoute(path);
    }

    template <StringLiteral Path>
    [[nodiscard]] constexpr auto route()
    {
        return StaticRoute<Path>{};
    }
}