# Benchmark

This page records one Release build benchmark run from `examples/overview.cpp`.

## Raw Results (Release)

```text
=== Benchmark (lower is better) ===
Iterations: 1000000
Note: benchmark uses flat key "Flat" to isolate access overhead.
Json::Get(std::string_view): 26.72 ns/op (x1.00)
Json::Get(dynamic route)    : 27.58 ns/op (x1.03)
Json::Get(static route)     : 26.61 ns/op (x1.00)
Native nlohmann find+get    : 21.97 ns/op (x0.82)
Dynamic route compile       : 27.01 ns/op (1000000 iterations, expression -> tokens)

=== Benchmark: Deep path A.B[2].C (lower is better) ===
Json::Get(dynamic route)    : 50.74 ns/op (x1.00)
Json::Get(static route)     : 53.19 ns/op (x1.05)
Native nlohmann chained     : 45.18 ns/op (x0.89)

=== Benchmark: Long route Root.Config.System.Modules[3].Pipelines[2].Stages[4].Name ===
Json::Get(dynamic route)    : 131.40 ns/op (x1.00)
Json::Get(static route)     : 140.88 ns/op (x1.07)
Native nlohmann chained     : 122.30 ns/op (x0.93)
```

## Quick Analysis

- Static route is not guaranteed to be faster in runtime lookup.
- In this run, static route is near dynamic on flat keys, and slightly slower on deeper paths.
- The main value of static route is compile-time validation of route literals.
- Dynamic route still has good ergonomics for runtime-provided paths.

## Compile-Time Validation Value

```cpp
constexpr auto route_ok = charted::route<"Root.Config.Modules[3].Name">();
static_assert(route_ok.IsValid(), "route literal should be valid");
```

Invalid route literals fail at compile time, so broken strings are caught before runtime tests.
