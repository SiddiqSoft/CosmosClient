# API Reference Overview

`CosmosClient` is defined inside namespace `siddiqsoft` in header file `siddiqsoft/cosmoscl.hpp`.

---

## Core Classes & Structs

| Symbol | Category | Description |
| :--- | :--- | :--- |
| [`CosmosClient`](cosmos_client.md) | Class | Primary client object managing Cosmos DB connections, configuration, requests, and CRUD operations. |
| [`CosmosEndpoint`](cosmos_endpoint.md) | Struct | Connection string parser and regional endpoint manager (`BaseUri`, `Key`, `ReadableUris`, `WritableUris`). |
| [`CosmosConnection`](cosmos_endpoint.md) | Struct | Holds connection status and `CosmosEndpoint` configuration. |
| [`CosmosResponseType`](cosmos_response.md) | Struct | Encapsulates REST status code, headers, JSON document payload, request charge (RUs), and continuation tokens. |
| [`CosmosIterableResponseType`](cosmos_response.md) | Struct | Derived response wrapper enabling iterator-style paged query execution. |
| [`CosmosArgumentType`](cosmos_argument.md) | Struct | Parameter structure for operations (database, collection/container, document, query, partitionKey, parameters, etc.). |
| [`CosmosOperation`](cosmos_argument.md) | Enum Class | Enum listing all supported Azure Cosmos DB REST API operations. |

---

## Header File

```cpp
#include <siddiqsoft/cosmoscl.hpp>
```
