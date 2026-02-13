# Integration

This page describes the recommended CMake integration flow for Charted.

Charted currently does **not** provide `find_package(charted)` config files yet.

## Option 1: add_subdirectory

```cmake
add_subdirectory(path/to/Charted)
target_link_libraries(your_target PRIVATE charted::charted)
```

## Option 2: FetchContent

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

## Project Options

- `CHARTED_BUILD_EXAMPLES` (default: `ON`)
- `CHARTED_ENABLE_MODULES` (default: `OFF`, requires CMake >= 3.28)

## Mode Reminder

- Default mode is **header include** usage.
- Set `CHARTED_ENABLE_MODULES=ON` to build **C++20 module bindings**.
- In both modes, the public CMake target name is still `charted::charted`.
