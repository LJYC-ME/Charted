# Charted

<p align="center">
  <img src="documents/docs/assets/Title-Charted.png" alt="Charted Titlebar" />
</p>


- **Route language**: dynamic and static route expressions (`A.B[2].C`)
- **JSON adapter**: ergonomic access on top of `nlohmann::json`
- **Two integration styles**: header include and C++20 modules
- **Single CMake target name**: always link `charted::charted`

---

## Why Charted

Charted focuses on route-driven data access with clear trade-offs:

- `DynamicRoute` is ideal when route strings come from runtime inputs.
- `StaticRoute` enables compile-time parsing and route literal validation.
- `Json` gives a consistent API for key-based and route-based access.

Static routes are not guaranteed to be faster in all runtime cases, but they are valuable for **compile-time correctness**.

---

## Quick Start

### Header include style

```cpp
#include <charted/charted.hpp>
#include <charted_json/charted_json.hpp>

auto dynamic_route = charted::route("A.B[2].C");
auto static_route  = charted::route<"A.B[2].C">();

charted::Json json;
json.Set(dynamic_route, 42);

int value = json.Get<int>(static_route, -1);
```

### C++20 module style

```cpp
import charted;
import charted.json;

auto dynamic_route = charted::route("A.B[2].C");
auto static_route  = charted::route<"A.B[2].C">();

charted::Json json;
json.Set(dynamic_route, 42);
int value = json.Get<int>(static_route, -1);
```

---

## Route API

```cpp
auto r1 = charted::route("Root.Config.Modules[3].Name");      // dynamic route
auto r2 = charted::route<"Root.Config.Modules[3].Name">();     // static route
```

Compile-time validation example:

```cpp
constexpr auto route_ok = charted::route<"Root.Config.Modules[3].Name">();
static_assert(route_ok.IsValid(), "route literal should be valid");
```

---

## Json API

```cpp
charted::Json json;

json.Set("name", "Charted");
json.Set(charted::route("A.B[1].C"), 7);

std::string name = json.Get<std::string>("name", "unknown");
int v = json.Get<int>(charted::route<"A.B[1].C">(), -1);

auto try_v = json.TryGet<int>(charted::route("A.B[1].Missing"));
```

---

## Build with CMake

Charted currently does **not** provide `find_package(charted)` config files yet.
Use one of the following integration methods.

### Option 1: `add_subdirectory`

```cmake
add_subdirectory(path/to/Charted)
target_link_libraries(your_target PRIVATE charted::charted)
```

### Option 2: `FetchContent`

```cmake
include(FetchContent)

FetchContent_Declare(
  charted
  GIT_REPOSITORY https://github.com/LJYC-ME/Charted.git
  GIT_TAG main
)
FetchContent_MakeAvailable(charted)

target_link_libraries(your_target PRIVATE charted::charted)
```

Project options:

- `CHARTED_BUILD_EXAMPLES` (default: `OFF`)
- `CHARTED_ENABLE_MODULES` (default: `OFF`, requires CMake >= 3.28)

> **Mode reminder**
> - By default, Charted is used as a **header-only** library.
> - Turn `CHARTED_ENABLE_MODULES=ON` to build and use the **C++20 module bindings**.
> - In both modes, the public CMake target name is still `charted::charted`.

When modules are enabled, the public link target is still `charted::charted`.

---

## Examples

- Runtime/benchmark example: `examples/overview.cpp`
- Module smoke test: `examples/charted_module.cppm`, `examples/charted_module_main.cpp`

---

## Documentation

Official documentation:

- https://charted.ljyc.me

Local docs source is under `documents/` with these sections:

- `Overview`
- `Route`
- `Json`
- `Benchmark`

---

## Author

- LJYC ([LJYC-ME](https://github.com/LJYC-ME))

