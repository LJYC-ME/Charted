# Json

`charted::Json` is an adapter over `nlohmann::json`.

## Basic Set/Get

```cpp
charted::Json json;
json.Set("name", "Charted");
json.Set("version", 1);

std::string name = json.Get<std::string>("name", "unknown");
int version = json.Get<int>("version", 0);
```

## Route-Based Access

```cpp
json.Set(charted::route("A.B[1].C"), 7);
int v1 = json.Get<int>(charted::route("A.B[1].C"), -1);
int v2 = json.Get<int>(charted::route<"A.B[1].C">(), -1);
```

## Parse and TryGet

```cpp
auto parsed = charted::Json::Parse(R"({"pi":3.14})");
if (parsed.has_value())
{
    auto pi = parsed->TryGet<double>("pi");
}
```
