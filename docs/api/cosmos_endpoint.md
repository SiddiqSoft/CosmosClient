# `CosmosEndpoint` & `CosmosConnection` Reference

Defined in `siddiqsoft/cosmoscl.hpp` inside namespace `siddiqsoft`.

---

## `CosmosEndpoint` Struct

```cpp
struct CosmosEndpoint {
    std::string BaseUri;
    std::string EncodedKey;
    std::string Key;
    std::vector<std::string> ReadableUris;
    size_t CurrentReadUriId{0};
    std::vector<std::string> WritableUris;
    size_t CurrentWriteUriId{0};

    CosmosEndpoint() = default;
    CosmosEndpoint(const std::string& connectionString);
};
```

### Fields

| Field | Type | Description |
| :--- | :--- | :--- |
| `BaseUri` | `std::string` | Base HTTPS URI of the Cosmos DB account. |
| `EncodedKey` | `std::string` | Base64-encoded account master key. |
| `Key` | `std::string` | Binary-decoded account key used for HMAC-SHA256 signature. |
| `ReadableUris` | `std::vector<std::string>` | Read-region URIs for failover / geo-replication. |
| `WritableUris` | `std::vector<std::string>` | Write-region URIs. |

---

## `CosmosConnection` Struct

```cpp
struct CosmosConnection {
    CosmosEndpoint endpoint;
    std::vector<std::string> partitionKeyNames;
    bool configured{false};
};
```

### Fields

| Field | Type | Description |
| :--- | :--- | :--- |
| `endpoint` | `CosmosEndpoint` | Active endpoint data. |
| `partitionKeyNames` | `std::vector<std::string>` | Configured partition key path names (e.g. `"/id"`). |
| `configured` | `bool` | True if successfully configured. |
