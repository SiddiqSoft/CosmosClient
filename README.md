# CosmosClient: Azure Cosmos DB REST Client for Modern C++

<!-- badges -->
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.CosmosClient?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionid=15&branchName=main)
![](https://img.shields.io/github/v/tag/SiddiqSoft/CosmosClient)
![](https://img.shields.io/nuget/v/SiddiqSoft.CosmosClient)
![](https://img.shields.io/nuget/dt/SiddiqSoft.CosmosClient)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/15)
![](https://img.shields.io/github/license/siddiqsoft/CosmosClient)
<!-- end badges -->

## Overview

**`CosmosClient`** is a header-only Modern C++23 client library for the Azure Cosmos DB REST API. Designed with `nlohmann::json` as a first-class API metaphor, it abstracts token signature calculation, regional endpoint resolution, payload serialization, and pagination behind a clean, expressive C++ interface.

### Key Features

- **JSON-First API**: `nlohmann::json` is a first-class citizen for documents, configuration, queries, and headers.
- **Modern C++23**: Utilizes C++23 standard features (`std::format`, aggregate initialization, structured bindings, `std::expected`).
- **Thread-Safe Failover & Rotation**: Lock-free atomic connection rotation (`rotate()`, `rotateReadUri()`) safe under concurrent workloads.
- **Header-Only**: Easy integration with no compilation overhead.
- **Cross-Platform**: Built on top of `restcl`, supporting native `WinHTTP` on Windows and `libcurl` on Unix/Linux/macOS.
- **Full Cosmos SQL API**: Complete support for Databases, Containers (Collections), Documents, SQL Queries, and Stored Procedures.
- **Async Operations**: Built-in non-blocking asynchronous operation dispatch via thread pool with worker exception safety.
- **Auto Token Signing**: Automatic HMAC-SHA256 authorization token generation for Azure Cosmos DB REST requests.

## Table of Contents

- [Quick Start](#quick-start)
- [Installation](#installation)
- [Usage Examples](#usage-examples)
- [Architecture](#architecture)
- [Dependencies](#dependencies)
- [Building](#building)
- [Testing](#testing)
- [License](#license)

---

## Quick Start

### Create Document

```cpp
#include "nlohmann/json.hpp"
#include "siddiqsoft/cosmoscl.hpp"

int main() {
    siddiqsoft::CosmosClient client;

    // Configure client with Azure Portal connection string
    client.configure({
        {"connectionStrings", {"AccountEndpoint=https://myaccount.documents.azure.com:443/;AccountKey=dGhpcyBpcyBhIHNhbXBsZSBrZXk=;"}},
        {"partitionKeyNames", {"/id"}}
    });

    // Create a new document
    auto resp = client.createDocument({
        .database     = "mydb",
        .collection   = "items",
        .partitionKey = "item-101",
        .document     = {
            {"id", "item-101"},
            {"name", "Modern C++ Sensor"},
            {"status", "active"}
        }
    });

    if (resp.statusCode == 201) {
        std::cout << "Document created! Request Charge (RUs): " << resp.requestCharge << std::endl;
    }

    return 0;
}
```

### SQL Query with Parameters

```cpp
#include "nlohmann/json.hpp"
#include "siddiqsoft/cosmoscl.hpp"

int main() {
    siddiqsoft::CosmosClient client;
    client.configure({
        {"connectionStrings", {"AccountEndpoint=https://myaccount.documents.azure.com:443/;AccountKey=...;"}},
        {"partitionKeyNames", {"/id"}}
    });

    auto resp = client.queryDocuments({
        .database   = "mydb",
        .collection = "items",
        .query      = "SELECT * FROM c WHERE c.status = @status",
        .parameters = { {{"name", "@status"}, {"value", "active"}} }
    });

    if (resp.statusCode == 200) {
        for (const auto& doc : resp.document["Documents"]) {
            std::cout << "Doc ID: " << doc["id"] << ", Name: " << doc["name"] << std::endl;
        }
    }

    return 0;
}
```

---

## Installation

### Via NuGet (Windows)

> **WARNING**
>
> This package has dependencies that must be satisfied via CPM / CMake when compiling.

```bash
nuget install SiddiqSoft.CosmosClient
```

### Via CMake & CPM (Recommended)

Add to your `CMakeLists.txt`:

```cmake
include(pack/CMakeCommonHelpers.cmake)

CPMAddPackage("gh:SiddiqSoft/CosmosClient#3.2.0")

target_link_libraries(your_target PRIVATE cosmoscl::cosmoscl)
```

---

## Architecture & Modular Header Layout

`CosmosClient` is structured as a header-only library using a main facade header (`siddiqsoft/cosmoscl.hpp`) and single-responsibility sub-headers in `siddiqsoft/private/`:

- **`cosmoscl.hpp`**: Public facade header defining the `CosmosClient` class interface and including internal modules.
- **`private/cosmos_types.hpp`**: Global logging instance (`gCLog`) and `CosmosOperation` enum definitions.
- **`private/cosmos_endpoint.hpp`**: `CosmosEndpoint` connection string parsing and lock-free atomic URI rotation.
- **`private/cosmos_connection.hpp`**: `CosmosConnection` struct and atomic primary/secondary failover state.
- **`private/cosmos_response.hpp`**: `CosmosResponseType` and `CosmosIterableResponseType` response envelopes.
- **`private/cosmos_argument.hpp`**: `CosmosArgumentType` request payload structure and callback type definitions.
- **`private/operations/`**: Single-responsibility operation headers (`database_ops.hpp`, `collection_ops.hpp`, `document_ops.hpp`, `query_ops.hpp`, `region_ops.hpp`).
- **`private/cosmos_serializers.hpp`**: `nlohmann::to_json`, `std::formatter`, and `operator<<` stream helpers.

---

## Dependencies

`CosmosClient` depends on lightweight header-only libraries and platform HTTP components.

See [**`dependencies.md`**](dependencies.md) or [**`docs/integration/dependencies.md`**](docs/integration/dependencies.md) for the full interactive Mermaid diagram and breakdown.

---

## Building

```bash
# Clone repository
git clone https://github.com/SiddiqSoft/CosmosClient.git
cd CosmosClient

# Build with CMake preset
cmake --preset default
cmake --build --preset default
```

### CMake Options

| Option | Default | Description |
| :--- | :--- | :--- |
| `cosmoscl_BUILD_TESTS` | `OFF` | Build unit and integration test suite (`BUILD_TESTS`). |
| `restcl_DEBUG_TRACE` | `OFF` | Enable detailed HTTP header and payload trace logging to `std::cerr`. |

---

## Testing

To run tests against the local Azure Cosmos DB Emulator (using Docker or Podman):

```bash
# 1. Start the Linux vNext Emulator container
./tests/setup-emulator.sh

# 2. Build and run tests using script or CMake
./tests/build_and_test.sh debug macos
```

> **Note on Emulator Protocols (`http://` vs `https://`)**:
> - **vNext Linux Emulator** (`azure-cosmos-emulator:vnext-latest`): Serves unencrypted **HTTP** on port 8081 (`AccountEndpoint=http://localhost:8081/;`).
> - **Windows / Classic Emulator**: Serves **HTTPS** on port 8081 with a self-signed TLS certificate (`AccountEndpoint=https://localhost:8081/;`).

---

## Documentation

Full documentation site is built with Material MkDocs and published to GitHub Pages:
- 📖 [**CosmosClient Documentation Site**](https://siddiqsoft.github.io/CosmosClient/)

---

## License

`CosmosClient` is licensed under the [BSD 3-Clause License](LICENSE).
