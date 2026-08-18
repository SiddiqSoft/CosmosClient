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

## Header Organization & Sub-Headers

The library is header-only and exposes a clean public facade header:

```cpp
#include <siddiqsoft/cosmoscl.hpp>
```

Internal implementations are modularized under `siddiqsoft/private/`:

| Module | Location | Purpose |
| :--- | :--- | :--- |
| **Facade** | `include/siddiqsoft/cosmoscl.hpp` | Main entry point; defines `CosmosClient` class interface. |
| **Types & Logging** | `include/siddiqsoft/private/cosmos_types.hpp` | Logger instance (`gCLog`) and `CosmosOperation` enum. |
| **Endpoint** | `include/siddiqsoft/private/cosmos_endpoint.hpp` | `CosmosEndpoint` connection parsing & atomic rotation. |
| **Connection** | `include/siddiqsoft/private/cosmos_connection.hpp` | `CosmosConnection` primary/secondary failover state. |
| **Response** | `include/siddiqsoft/private/cosmos_response.hpp` | `CosmosResponseType` and `CosmosIterableResponseType`. |
| **Argument** | `include/siddiqsoft/private/cosmos_argument.hpp` | `CosmosArgumentType` parameter struct and callback alias. |
| **Operations** | `include/siddiqsoft/private/operations/` | `database_ops.hpp`, `collection_ops.hpp`, `document_ops.hpp`, `query_ops.hpp`, `region_ops.hpp`. |
| **Serializers** | `include/siddiqsoft/private/cosmos_serializers.hpp` | `to_json`, `std::formatter`, and `operator<<` stream helpers. |

