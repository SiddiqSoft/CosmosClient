# `CosmosResponseType` & `CosmosIterableResponseType` Reference

Defined in `siddiqsoft/cosmoscl.hpp` inside namespace `siddiqsoft`.

---

## `CosmosResponseType` Struct

Encapsulates the HTTP status and payload returned by Azure Cosmos DB REST API calls.

```cpp
struct CosmosResponseType {
    int statusCode{0};
    nlohmann::json headers{};
    nlohmann::json document{};
    double requestCharge{0.0};
    std::string continuationToken{};
    std::chrono::microseconds duration{0};

    operator bool() const noexcept {
        return statusCode >= 200 && statusCode < 300;
    }
};
```

### Fields

| Field | Type | Description |
| :--- | :--- | :--- |
| `statusCode` | `int` | HTTP status code (200 = OK, 201 = Created, 204 = No Content, 404 = Not Found, etc.). |
| `headers` | `nlohmann::json` | Response headers (e.g. `x-ms-request-charge`, `x-ms-continuation`). |
| `document` | `nlohmann::json` | JSON response payload returned by Cosmos DB. |
| `requestCharge` | `double` | Request Units (RUs) consumed by the operation. |
| `continuationToken` | `std::string` | Token used for query pagination (`x-ms-continuation`). |
| `duration` | `std::chrono::microseconds` | Client wall-clock latency for the operation. |

---

## `CosmosIterableResponseType` Struct

Inherits from `CosmosResponseType` to provide standard iterator interface for paged queries.

```cpp
struct CosmosIterableResponseType : public CosmosResponseType {
    bool hasNext() const;
    CosmosResponseType next();
};
```

### Member Functions

#### `hasNext`
```cpp
bool hasNext() const;
```
Returns `true` if more result pages remain to be fetched.

#### `next`
```cpp
CosmosResponseType next();
```
Fetches the next page of query results using the current `continuationToken`.
