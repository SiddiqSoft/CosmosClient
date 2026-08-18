# CosmosClient: Azure Cosmos DB REST Client for Modern C++

<div class="badge-container">
  <a href="https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionid=15&branchName=main"><img src="https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.CosmosClient?branchName=main" alt="Build Status"></a>
  <a href="https://github.com/SiddiqSoft/CosmosClient/tags"><img src="https://img.shields.io/github/v/tag/SiddiqSoft/CosmosClient" alt="GitHub Tag"></a>
  <a href="https://www.nuget.org/packages/SiddiqSoft.CosmosClient"><img src="https://img.shields.io/nuget/v/SiddiqSoft.CosmosClient" alt="NuGet Version"></a>
  <a href="https://www.nuget.org/packages/SiddiqSoft.CosmosClient"><img src="https://img.shields.io/nuget/dt/SiddiqSoft.CosmosClient" alt="NuGet Downloads"></a>
  <img src="https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/15" alt="Tests" />
  <img src="https://img.shields.io/github/license/siddiqsoft/CosmosClient" alt="License" />
</div>

**`CosmosClient`** is a header-only Modern C++23 client library for the Azure Cosmos DB REST API. Designed with `nlohmann::json` as a first-class API metaphor, it allows C++ developers to interact with Azure Cosmos DB containers, documents, databases, and stored procedures with native performance, automatic REST token signing, and cross-platform HTTP support.

---

## Design Objectives

* **JSON as First-Class Metaphor**: Standard `nlohmann::json` objects represent connection parameters, documents, queries, and responses for an intuitive, modern C++ developer experience.
* **Modern C++23**: Requires C++23 with support for concepts, initializer lists, structured bindings, and standard string formatting.
* **Header-Only Library**: Simple integration without separate compilation steps.
* **Cross-Platform HTTP**: Built on top of `restcl`, using native `WinHTTP` on Windows and `libcurl` on Linux/macOS.
* **Full Cosmos SQL API**: Support for Database, Container, Document CRUD, SQL Querying with Paged Iterators, and Stored Procedure execution.
* **Async Operations**: Built-in thread-pool asynchronous dispatch for non-blocking database queries and document updates.

---

## Quick Example

=== "Document Creation"

    ```cpp
    #include "nlohmann/json.hpp"
    #include "siddiqsoft/cosmoscl.hpp"

    int main()
    {
        siddiqsoft::CosmosClient client;

        // 1. Configure with Azure Cosmos DB connection string
        client.configure({
            {"connectionStrings", {"AccountEndpoint=https://myaccount.documents.azure.com:443/;AccountKey=...;"}},
            {"partitionKeyNames", {"/id"}}
        });

        // 2. Create a new document in database 'mydb', collection 'items'
        auto resp = client.createDocument({
            .database   = "mydb",
            .collection = "items",
            .document   = {
                {"id", "item-101"},
                {"name", "Modern C++ Cosmos Client"},
                {"tags", {"c++23", "azure", "cosmosdb"}}
            }
        });

        if (resp.statusCode == 201) {
            std::cout << "Document created successfully!" << std::endl;
            std::cout << "Request Charge (RUs): " << resp.requestCharge << std::endl;
        }

        return 0;
    }
    ```

=== "SQL Querying"

    ```cpp
    #include "nlohmann/json.hpp"
    #include "siddiqsoft/cosmoscl.hpp"

    int main()
    {
        siddiqsoft::CosmosClient client;
        client.configure({
            {"connectionStrings", {"AccountEndpoint=https://myaccount.documents.azure.com:443/;AccountKey=...;"}},
            {"partitionKeyNames", {"/id"}}
        });

        // Query documents matching condition
        auto resp = client.queryDocuments({
            .database   = "mydb",
            .collection = "items",
            .query      = "SELECT * FROM c WHERE c.name = @name",
            .parameters = { {{"name", "@name"}, {"value", "Modern C++ Cosmos Client"}} }
        });

        if (resp.statusCode == 200) {
            for (const auto& doc : resp.document["Documents"]) {
                std::cout << "Found doc ID: " << doc["id"] << std::endl;
            }
        }

        return 0;
    }
    ```

=== "Async Execution"

    ```cpp
    #include "nlohmann/json.hpp"
    #include "siddiqsoft/cosmoscl.hpp"

    int main()
    {
        siddiqsoft::CosmosClient client;
        client.configure({
            {"connectionStrings", {"AccountEndpoint=https://myaccount.documents.azure.com:443/;AccountKey=...;"}},
            {"partitionKeyNames", {"/id"}}
        });

        // Dispatch asynchronous document read
        client.sendAsync(
            siddiqsoft::CosmosOperation::ReadDocument,
            {.database = "mydb", .collection = "items", .id = "item-101", .partitionKey = "item-101"},
            [](auto resp) {
                if (resp.statusCode == 200) {
                    std::cout << "Async read doc: " << resp.document.dump() << std::endl;
                }
            }
        );

        return 0;
    }
    ```

---

## Requirements

| Requirement | Details |
| :--- | :--- |
| **Language Standard** | C++23 or higher (`/std:c++latest` on MSVC, `-std=c++23` on Clang/GCC) |
| **Core Dependencies** | [`nlohmann/json`](https://github.com/nlohmann/json), [`restcl`](https://github.com/SiddiqSoft/restcl), [`SplitUri`](https://github.com/SiddiqSoft/SplitUri), [`AzureCppUtils`](https://github.com/SiddiqSoft/AzureCppUtils) |
| **Platform Support** | Windows (MSVC 2019+), Linux (GCC 11+, Clang 13+), macOS (Apple Clang 13+) |

---

## Navigation

- [**Features**](features/index.md): Explore connection management, document operations, SQL queries, and async methods.
- [**Integration**](integration/index.md): Guides for CMake integration, package management, and dependencies diagram.
- [**API Reference**](api/index.md): Complete API documentation for `CosmosClient`, `CosmosEndpoint`, `CosmosResponseType`, and `CosmosArgumentType`.
