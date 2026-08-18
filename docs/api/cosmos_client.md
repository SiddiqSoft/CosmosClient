# `CosmosClient` Class Reference

Defined in `siddiqsoft/cosmoscl.hpp` inside namespace `siddiqsoft`.

```cpp
class CosmosClient;
```

---

## Constructors & Rule of 5

```cpp
CosmosClient() = default;
CosmosClient(CosmosClient&& src) noexcept;
CosmosClient& operator=(CosmosClient&& src) noexcept;

CosmosClient(const CosmosClient&) = delete;
CosmosClient& operator=(const CosmosClient&) = delete;
```

`CosmosClient` is move-only, managing internal thread pools and connection state.

---

## Member Functions

### Configuration & State

#### `configure`
```cpp
CosmosClient& configure(const nlohmann::json& src = {}) noexcept(false);
```
Configures the client with connection strings and partition key names.

#### `configuration`
```cpp
const nlohmann::json& configuration() const;
```
Returns the active configuration JSON.

---

## Document Operations

#### `createDocument`
```cpp
CosmosResponseType createDocument(CosmosArgumentType const& ctx);
```
Creates a new document in the specified database and collection.

#### `findDocument`
```cpp
CosmosResponseType findDocument(CosmosArgumentType const& ctx);
```
Retrieves a single document by `id` and `partitionKey`.

#### `updateDocument`
```cpp
CosmosResponseType updateDocument(CosmosArgumentType const& ctx);
```
Updates an existing document.

#### `upsertDocument`
```cpp
CosmosResponseType upsertDocument(CosmosArgumentType const& ctx);
```
Inserts a new document or updates an existing document.

#### `removeDocument`
```cpp
uint32_t removeDocument(CosmosArgumentType const& ctx);
```
Removes a document by `id` and `partitionKey`. Returns the HTTP status code (e.g. `204`).

---

## Query Operations

#### `queryDocuments`
```cpp
CosmosIterableResponseType queryDocuments(CosmosArgumentType const& ctx);
```
Executes a SQL query against the collection, returning paged document arrays and continuation tokens.

#### `listDocuments`
```cpp
CosmosIterableResponseType listDocuments(CosmosArgumentType const& ctx);
```
Lists documents with pagination support.

---

## Database & Collection Operations

#### `createDatabase` / `findDatabase` / `listDatabases` / `deleteDatabase`
```cpp
CosmosResponseType createDatabase(CosmosArgumentType const& ctx);
CosmosResponseType findDatabase(CosmosArgumentType const& ctx);
CosmosResponseType listDatabases();
CosmosResponseType deleteDatabase(CosmosArgumentType const& ctx);
```

#### `createCollection` / `listCollections`
```cpp
CosmosResponseType createCollection(CosmosArgumentType const& ctx);
CosmosResponseType listCollections(CosmosArgumentType const& ctx);
```

#### `discoverRegions`
```cpp
CosmosResponseType discoverRegions();
```
Discovers regional read and write endpoints for failover routing.

---

## Asynchronous Execution

#### `async`
```cpp
void async(CosmosArgumentType&& op) noexcept(false);
```
Dispatches the operation payload to an internal asynchronous thread pool. Upon completion (or exception), the `op.onResponse` callback is safely invoked. Exceptions inside background workers are caught and logged without process termination.

