module;
#include <cassert>
export module charted.example;

import charted;
import charted.json;

export int main()
{
    auto dynamic_route = charted::route("A.B[1].C");
    assert(dynamic_route.GetTokenCount() == 4);
    auto static_route  = charted::route<"A.B[1].C">();
    static_assert(charted::route<"A.B[1].C">().GetTokenCount() == 4);

    return 0;
}
