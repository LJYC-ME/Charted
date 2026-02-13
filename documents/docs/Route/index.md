# Route

Route expressions support:

- Key segments: `A.B.C`
- Array indices: `A.B[2].C`

## Dynamic Route

Dynamic route is runtime-driven.
Use it when the route comes from config, CLI, files, or user input.

```cpp
auto route = charted::route("A.B[2].C");
if (route.IsValid())
{
    auto tokens = route.GetTokens();
}
```

## Static Route

Static route is compile-time literal-driven.
Use it when the route is fixed in code.

```cpp
auto route = charted::route<"A.B[2].C">();
```

Static routes are parsed at compile time. Invalid literals fail compilation.

## Compile-Time Validation

```cpp
constexpr auto route_ok = charted::route<"Root.Config.Modules[3].Name">();
static_assert(route_ok.IsValid(), "route literal should be valid");
```
