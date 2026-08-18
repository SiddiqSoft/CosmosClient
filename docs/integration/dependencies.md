# Project Dependencies

This document is automatically generated from `CMakeLists.txt` files for `cosmoscl`.

## Dependency Diagram

```mermaid
graph TD
    cosmoscl["cosmoscl::cosmoscl"]

    subgraph Platform["Platform-Specific HTTP Backend"]
        LIBCURL["libcurl >= 8.7 (Linux / macOS)"]
        ACW32H["acw32h 2.7.4 (Windows / MSVC)"]
    end

    subgraph Core["Core Dependencies (via CPM)"]
        NLOHMANNJSON["nlohmann_json v3.12.0"]
        SPLITURI["SplitUri 3.0.3"]
        STRING2MAP["string2map 2.6.1"]
        GENPROCESSINFO["GenProcessInfo 2.6.1"]
        TIMETHIS["TimeThis 2.4.1"]
        RESTCL["restcl 2.3.9"]
    end

    subgraph Test["Test Dependencies (Optional)"]
        GOOGLETEST["googletest v1.17.0"]
    end

    cosmoscl --> LIBCURL
    cosmoscl --> ACW32H
    cosmoscl --> NLOHMANNJSON
    cosmoscl --> SPLITURI
    cosmoscl --> STRING2MAP
    cosmoscl --> GENPROCESSINFO
    cosmoscl --> TIMETHIS
    cosmoscl --> RESTCL
    cosmoscl -. "BUILD_TESTS=ON" .-> GOOGLETEST
```

## Dependency Breakdown

| Dependency | Repository / Target | Version | Type | Scope / Platform |
| :--- | :--- | :--- | :--- | :--- |
| **libcurl** | `System / CURL` | >= 8.7 | `find_package` | Linux / macOS (GCC, Clang, AppleClang) |
| **acw32h** | [`SiddiqSoft/acw32h`](https://github.com/SiddiqSoft/acw32h) | 2.7.4 | `CPM` | Windows (MSVC) |
| **nlohmann_json** | [`nlohmann/json`](https://github.com/nlohmann/json) | v3.12.0 | `CPM` | All Platforms (`INTERFACE`) |
| **SplitUri** | [`SiddiqSoft/SplitUri`](https://github.com/SiddiqSoft/SplitUri) | 3.0.3 | `CPM` | All Platforms (`INTERFACE`) |
| **string2map** | [`SiddiqSoft/string2map`](https://github.com/SiddiqSoft/string2map) | 2.6.1 | `CPM` | All Platforms (`INTERFACE`) |
| **GenProcessInfo** | [`SiddiqSoft/GenProcessInfo`](https://github.com/SiddiqSoft/GenProcessInfo) | 2.6.1 | `CPM` | All Platforms (`INTERFACE`) |
| **TimeThis** | [`SiddiqSoft/TimeThis`](https://github.com/SiddiqSoft/TimeThis) | 2.4.1 | `CPM` | All Platforms (`INTERFACE`) |
| **restcl** | [`SiddiqSoft/restcl`](https://github.com/SiddiqSoft/restcl) | 2.3.9 | `CPM` | All Platforms (`INTERFACE`) |
| **googletest** | [`google/googletest`](https://github.com/google/googletest) | v1.17.0 | `CPM` | Test Target Only (`BUILD_TESTS=ON`) |
