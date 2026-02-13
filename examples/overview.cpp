#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>
#include "charted/charted.hpp"
#include "charted_json/charted_json.hpp"

int main()
{
    std::cout << "=== Quick Start ===\n";

    auto dynamic_route = charted::route("A.B[2].C");
    const auto& static_route = charted::route<"A.B[2].C">();

    std::cout << "Dynamic path: " << dynamic_route.GetPathString() << '\n';
    std::cout << "Dynamic tokens: " << dynamic_route.GetTokenCount() << '\n';

    std::cout << "Static path: " << static_route.GetPathString() << '\n';
    std::cout << "Static tokens: " << static_route.GetTokenCount() << '\n';

    charted::Json json;
    json.Set(dynamic_route, 42);

    const int value = json.Get(static_route, -1);
    std::cout << "Json value: " << value << '\n';

    std::cout << "\n=== More Usage Cases ===\n";
    json.Set("name", "Charted");
    json.Set("version", 1);
    json.Set("pi", 3.1415926);
    json.Set("enabled", true);
    json.Set(charted::route("config.window.width"), 1920);
    json.Set(charted::route<"config.window.height">(), 1080);

    const std::string name  = json.Get<std::string>("name", "unknown");
    const int width         = json.Get<int>(charted::route("config.window.width"), 0);
    const int height        = json.Get<int>(charted::route<"config.window.height">(), 0);
    const bool enabled      = json.Get<bool>("enabled", false);
    const int missing_with_default = json.Get<int>("missing_key", -404);
    const auto try_pi       = json.TryGet<double>("pi");
    const auto try_missing  = json.TryGet<int>(charted::route("config.window.depth"));

    std::cout << "name: " << name << '\n';
    std::cout << "window: " << width << "x" << height << '\n';
    std::cout << "enabled: " << std::boolalpha << enabled << '\n';
    std::cout << "missing_key with default: " << missing_with_default << '\n';
    std::cout << "TryGet(pi): " << (try_pi.has_value() ? std::to_string(*try_pi) : std::string("nullopt")) << '\n';
    std::cout << "TryGet(missing route): " << (try_missing.has_value() ? "value" : "nullopt") << '\n';

    if (const auto parsed = charted::Json::Parse(R"({"hello":"world","n":7})"); parsed.has_value())
    {
        std::cout << "Parse() demo, hello = " << parsed->Get<std::string>("hello", "none") << '\n';
    }

    auto long_dynamic_route = charted::route("Root.Config.System.Modules[3].Pipelines[2].Stages[4].Name");
    const auto& long_static_route =
        charted::route<"Root.Config.System.Modules[3].Pipelines[2].Stages[4].Name">();
    json.Set(long_dynamic_route, "Stage-Name");

    json.Set("Flat", 123);
    auto dynamic_flat_route = charted::route("Flat");
    const auto& static_flat_route = charted::route<"Flat">();
    std::string flat_key    = "Flat";
    auto flat_key_view      = std::string_view(flat_key);

    using clock = std::chrono::steady_clock;
    constexpr std::size_t iterations = 1'000'000;
    constexpr std::size_t parse_iterations = 100'000;
    volatile int sink = 0;

    auto benchmark_ns_per_op = [&](auto&& fn) -> double
    {
        const auto start = clock::now();
        for (std::size_t i = 0; i < iterations; ++i)
        {
            sink += fn();
        }
        const auto end = clock::now();
        const auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        return static_cast<double>(total_ns) / static_cast<double>(iterations);
    };

    // Warm up hot paths once to reduce one-time noise in the first measured run.
    (void)benchmark_ns_per_op([&]() { return json.Get<int>(flat_key_view, -1); });

    const double ns_get_key           = benchmark_ns_per_op([&]() { return json.Get<int>(flat_key_view, -1); });
    const double ns_get_dynamic_route = benchmark_ns_per_op([&]() { return json.Get<int>(dynamic_flat_route, -1); });
    const double ns_get_static_route  = benchmark_ns_per_op([&]() { return json.Get<int>(static_flat_route, -1); });
    const double ns_native_find_get   = benchmark_ns_per_op([&]()
    {
        const auto& native = json.GetNative();
        const auto it = native.find(flat_key);
        return (it != native.end()) ? it->get<int>() : -1;
    });

    const double ns_deep_dynamic_route = benchmark_ns_per_op([&]() { return json.Get<int>(dynamic_route, -1); });
    const double ns_deep_static_route  = benchmark_ns_per_op([&]() { return json.Get<int>(static_route, -1); });
    const double ns_deep_native_chained = benchmark_ns_per_op([&]()
    {
        const auto& native = json.GetNative();
        return native["A"]["B"][2]["C"].get<int>();
    });
    const double ns_long_dynamic_route = benchmark_ns_per_op([&]()
    {
        return static_cast<int>(json.Get<std::string>(long_dynamic_route, "missing").size());
    });
    const double ns_long_static_route = benchmark_ns_per_op([&]()
    {
        return static_cast<int>(json.Get<std::string>(long_static_route, "missing").size());
    });
    const double ns_long_native_chained = benchmark_ns_per_op([&]()
    {
        const auto& native = json.GetNative();
        return static_cast<int>(
            native["Root"]["Config"]["System"]["Modules"][3]["Pipelines"][2]["Stages"][4]["Name"]
                .get<std::string>()
                .size());
    });

    const auto parse_start = clock::now();
    for (std::size_t i = 0; i < parse_iterations; ++i)
    {
        auto parsed = charted::route("A.B[2].C");
        sink += static_cast<int>(parsed.GetTokenCount());
    }
    const auto parse_end = clock::now();
    const auto parse_total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(parse_end - parse_start).count();
    const double ns_dynamic_parse = static_cast<double>(parse_total_ns) / static_cast<double>(parse_iterations);

    std::cout << "\n=== Benchmark (lower is better) ===\n";
    std::cout << "Iterations: " << iterations << '\n';
    std::cout << "Note: benchmark uses flat key \"Flat\" to isolate access overhead.\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Json::Get(std::string_view): " << ns_get_key << " ns/op (x1.00)\n";
    std::cout << "Json::Get(dynamic route)   : " << ns_get_dynamic_route << " ns/op (x"
              << (ns_get_dynamic_route / ns_get_key) << ")\n";
    std::cout << "Json::Get(static route)    : " << ns_get_static_route << " ns/op (x"
              << (ns_get_static_route / ns_get_key) << ")\n";
    std::cout << "Native nlohmann find+get   : " << ns_native_find_get << " ns/op (x"
              << (ns_native_find_get / ns_get_key) << ")\n";
    std::cout << "Dynamic route compile      : " << ns_dynamic_parse << " ns/op (" << parse_iterations
              << " iterations, expression -> tokens)\n";

    std::cout << "\n=== Benchmark: Deep path A.B[2].C (lower is better) ===\n";
    std::cout << "Json::Get(dynamic route)   : " << ns_deep_dynamic_route << " ns/op (x1.00)\n";
    std::cout << "Json::Get(static route)    : " << ns_deep_static_route << " ns/op (x"
              << (ns_deep_static_route / ns_deep_dynamic_route) << ")\n";
    std::cout << "Native nlohmann chained    : " << ns_deep_native_chained << " ns/op (x"
              << (ns_deep_native_chained / ns_deep_dynamic_route) << ")\n";

    std::cout << "\n=== Benchmark: Long route Root.Config.System.Modules[3].Pipelines[2].Stages[4].Name ===\n";
    std::cout << "Json::Get(dynamic route)   : " << ns_long_dynamic_route << " ns/op (x1.00)\n";
    std::cout << "Json::Get(static route)    : " << ns_long_static_route << " ns/op (x"
              << (ns_long_static_route / ns_long_dynamic_route) << ")\n";
    std::cout << "Native nlohmann chained    : " << ns_long_native_chained << " ns/op (x"
              << (ns_long_native_chained / ns_long_dynamic_route) << ")\n";

    return 0;
}