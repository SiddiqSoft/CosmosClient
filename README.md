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
- **Header-Only**: Easy integration with no compilation overhead.
- **Cross-Platform**: Built on top of `restcl`, supporting native `WinHTTP` on Windows and `libcurl` on Unix/Linux/macOS.
- **Full Cosmos SQL API**: Complete support for Databases, Containers (Collections), Documents, SQL Queries, and Stored Procedures.
- **Async Operations**: Built-in non-blocking asynchronous operation dispatch via thread pool.
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

## Architecture

`CosmosClient` is structured into core header components:

- **`CosmosEndpoint`**: Parses Azure connection strings (`AccountEndpoint`, `AccountKey`), calculates HMAC-SHA256 signatures, and routes requests to primary or read-replica endpoints.
- **`CosmosClient`**: Primary client instance handling database, collection, document, query, and stored procedure REST calls.
- **`CosmosResponseType`**: Encloses response HTTP status, response headers, JSON body, request charge (RUs), and pagination tokens.
- **`CosmosIterableResponseType`**: Wrapper for iterating over paged query results seamlessly.

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

---

## Testing

```bash
ctest --preset default
```

---

## Documentation

Full documentation site is built with Material MkDocs and published to GitHub Pages:
- 📖 [**CosmosClient Documentation Site**](https://siddiqsoft.github.io/CosmosClient/)

---

## License

`CosmosClient` is licensed under the [BSD 3-Clause License](LICENSE).
