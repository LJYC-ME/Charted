# Overview

![Charted Title](assets/Title-Charted.png)

## Integration Styles

- Header include style (`#include <charted/...>`)
- C++20 module style (`import charted; import charted.json;`)

Both styles share the same public API (`charted::route`, `charted::Json`).

## Header Include Example

```cpp
#include <charted/charted.hpp>
#include <charted_json/charted_json.hpp>

auto dynamic_route = charted::route("A.B[2].C");
auto static_route  = charted::route<"A.B[2].C">();

charted::Json json;
json.Set(dynamic_route, 42);
int value = json.Get<int>(static_route, -1);
```

## C++ Module Example

```cpp
import charted;
import charted.json;

auto dynamic_route = charted::route("A.B[2].C");
auto static_route  = charted::route<"A.B[2].C">();

charted::Json json;
json.Set(dynamic_route, 42);
int value = json.Get<int>(static_route, -1);
```

## CMake Switches

- `CHARTED_ENABLE_MODULES=OFF` (default): header include style only.
- `CHARTED_ENABLE_MODULES=ON`: builds module bindings (`charted`, `charted.json`) on supported toolchains.
- `CHARTED_BUILD_EXAMPLES=ON`: builds example targets.
