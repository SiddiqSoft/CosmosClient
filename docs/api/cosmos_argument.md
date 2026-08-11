# `CosmosArgumentType` & `CosmosOperation` Reference

Defined in `siddiqsoft/cosmoscl.hpp` inside namespace `siddiqsoft`.

---

## `CosmosArgumentType` Struct

C++ aggregate structure passed to `CosmosClient` operation methods.

```cpp
struct CosmosArgumentType {
    std::string database{};
    std::string collection{};
    std::string id{};
    std::string partitionKey{};
    nlohmann::json document{};
    std::string query{};
    nlohmann::json parameters{};
    std::string continuationToken{};
    int maxItemCount{-1};
    nlohmann::json headers{};
};
```

### Fields

| Field | Type | Description |
| :--- | :--- | :--- |
| `database` | `std::string` | Cosmos DB database name. |
| `collection` | `std::string` | Cosmos DB collection / container name. |
| `id` | `std::string` | Document ID. |
| `partitionKey` | `std::string` | Partition key value. |
| `document` | `nlohmann::json` | JSON document payload for create/replace. |
| `query` | `std::string` | SQL query string (`SELECT * FROM c ...`). |
| `parameters` | `nlohmann::json` | Query parameter bindings array `[{"name":"@param","value":"val"}]`. |
| `continuationToken` | `std::string` | Page continuation token. |
| `maxItemCount` | `int` | Maximum items to return per page (-1 for server default). |
| `headers` | `nlohmann::json` | Custom HTTP headers to pass with request. |

---

## `CosmosOperation` Enum Class

Enum listing supported Cosmos REST API operations:

```cpp
enum class CosmosOperation {
    GetDatabase,
    ListDatabases,
    CreateDatabase,
    DeleteDatabase,
    GetContainer,
    ListContainers,
    CreateContainer,
    DeleteContainer,
    CreateDocument,
    ReadDocument,
    ReplaceDocument,
    DeleteDocument,
    QueryDocuments,
    ExecuteStoredProcedure
};
```
